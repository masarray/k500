// POSIX_K500_TRANSPORT_V1 — real USB HID + serial transport on macOS/Linux.
// The existing Win32 worker implementation remains unchanged on Windows.
// Blocking native I/O runs only on this dedicated worker, never the QML thread.
#include "K500WinIo.h"
#include "K500Frame.h"
#include <QMetaObject>
#include <QPointer>
#include <QQueue>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QList>
#include <QtGlobal>
#include <hidapi/hidapi.h>
#include <algorithm>
#include <utility>

class K500WinIo::Impl final : public QObject
{
public:
    explicit Impl(K500WinIo *owner) : q(owner) {}
    ~Impl() override { closeTransport(); }

    K500WinIo *q = nullptr;
    Kind kind = Kind::None;
    QString label;
    bool closing = false;
    QSerialPort *serial = nullptr;
    hid_device *hid = nullptr;
    QTimer *poll = nullptr;
    QQueue<QByteArray> hidWrites;
    QList<quint64> pendingDrains;

    void publishBytes(const QByteArray &bytes) {
        QPointer<K500WinIo> owner(q);
        QMetaObject::invokeMethod(q, [owner, bytes] {
            if (owner) emit owner->bytesReceived(bytes);
        }, Qt::QueuedConnection);
    }

    void publishError(const QString &message) {
        if (closing) return;
        QPointer<K500WinIo> owner(q);
        QMetaObject::invokeMethod(q, [owner, message] {
            if (owner) emit owner->errorOccurred(message);
        }, Qt::QueuedConnection);
    }

    void emitDrainedWhenIdle() {
        if (kind == Kind::None || !hidWrites.isEmpty() || pendingDrains.isEmpty())
            return;
        if (kind == Kind::Serial && serial && serial->bytesToWrite() > 0)
            return;
        const auto tokens = std::exchange(pendingDrains, QList<quint64>{});
        QPointer<K500WinIo> owner(q);
        QMetaObject::invokeMethod(q, [owner, tokens] {
            if (!owner) return;
            for (const quint64 token : tokens) emit owner->writesDrained(token);
        }, Qt::QueuedConnection);
    }

    void tick() {
        if (closing) return;
        if (kind == Kind::UsbHid && hid) {
            // hid_read_timeout(0) returns immediately. Report id zero is
            // sometimes included (65 bytes) but not on all POSIX drivers.
            unsigned char buffer[512]{};
            for (int attempt = 0; attempt < 12; ++attempt) {
                const int n = hid_read_timeout(hid, buffer, sizeof(buffer), 0);
                if (n < 0) {
                    publishError(QStringLiteral("K500 HID read failed"));
                    closeTransport();
                    return;
                }
                if (n == 0) break;
                const int skip = (n == 65 && buffer[0] == 0) ? 1 : 0;
                if (n > skip) publishBytes(QByteArray(reinterpret_cast<const char *>(buffer + skip), n - skip));
            }
            // Bounded batch to prevent long mass-upload transfers from
            // monopolizing the native event loop.
            for (int sent = 0; sent < 8 && !hidWrites.isEmpty(); ++sent) {
                const QByteArray report = hidWrites.dequeue();
                const int bytes = hid_write(hid,
                    reinterpret_cast<const unsigned char *>(report.constData()),
                    static_cast<size_t>(report.size()));
                if (bytes != report.size()) {
                    hidWrites.clear();
                    pendingDrains.clear(); // Failure is NOT a successful Save barrier.
                    publishError(QStringLiteral("K500 HID write failed or was partial"));
                    closeTransport();
                    return;
                }
            }
        }
        if (kind == Kind::Serial && serial && serial->bytesToWrite() > 0)
            serial->flush();
        emitDrainedWhenIdle();
    }

    void closeTransport() {
        closing = true;
        if (poll) { poll->stop(); delete poll; poll = nullptr; }
        if (serial) {
            serial->disconnect(this);
            serial->close();
            delete serial;
            serial = nullptr;
        }
        if (hid) { hid_close(hid); hid = nullptr; }
        hidWrites.clear();
        pendingDrains.clear();
        kind = Kind::None;
        label.clear();
        closing = false;
    }

    bool openSerial(const QString &portName, QString *error) {
        closeTransport();
        auto *device = new QSerialPort(this);
        device->setPortName(portName);
        device->setBaudRate(QSerialPort::Baud115200);
        device->setDataBits(QSerialPort::Data8);
        device->setParity(QSerialPort::NoParity);
        device->setStopBits(QSerialPort::OneStop);
        device->setFlowControl(QSerialPort::NoFlowControl);
        if (!device->open(QIODevice::ReadWrite)) {
            if (error) *error = QStringLiteral("Serial %1: %2").arg(portName, device->errorString());
            delete device;
            return false;
        }
        serial = device;
        device->setDataTerminalReady(true);
        device->setRequestToSend(true);
        kind = Kind::Serial;
        label = portName;
        connect(device, &QSerialPort::readyRead, this, [this] {
            if (serial && !closing) publishBytes(serial->readAll());
        });
        connect(device, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError e) {
            if (e != QSerialPort::NoError && serial && !closing)
                publishError(QStringLiteral("K500 serial: %1").arg(serial->errorString()));
        });
        poll = new QTimer(this);
        poll->setInterval(12);
        connect(poll, &QTimer::timeout, this, [this] { tick(); });
        poll->start();
        return true;
    }

    bool openUsbHid(quint16 vendorId, quint16 productId,
                    QString *deviceLabel, QString *error) {
        closeTransport();
        if (hid_init() != 0) {
            if (error) *error = QStringLiteral("hidapi initialization failed");
            return false;
        }
        hid = hid_open(vendorId, productId, nullptr);
        if (!hid) {
            if (error) *error = QStringLiteral("K500 USB HID %1:%2 unavailable or permission denied")
                .arg(vendorId, 4, 16, QLatin1Char('0'))
                .arg(productId, 4, 16, QLatin1Char('0'));
            return false;
        }
        if (hid_set_nonblocking(hid, 1) != 0) {
            if (error) *error = QStringLiteral("K500 USB HID cannot enter nonblocking mode");
            closeTransport();
            return false;
        }
        wchar_t product[256]{};
        label = hid_get_product_string(hid, product, 256) == 0
            ? QString::fromWCharArray(product).trimmed()
            : QStringLiteral("USB HID DSP AUDIO");
        if (label.isEmpty()) label = QStringLiteral("USB HID DSP AUDIO");
        kind = Kind::UsbHid;
        poll = new QTimer(this);
        poll->setInterval(8);
        connect(poll, &QTimer::timeout, this, [this] { tick(); });
        poll->start();
        if (deviceLabel) *deviceLabel = label;
        return true;
    }

    void enqueueProtocolFrame(const QByteArray &frame) {
        if (kind == Kind::None || closing) {
            publishError(QStringLiteral("K500 transport is not open"));
            return;
        }
        if (kind == Kind::Serial) {
            if (!serial || serial->write(frame) != frame.size())
                publishError(QStringLiteral("K500 serial write was not accepted"));
            return;
        }
        if (!hid) return;
        const QByteArray usb = K500Frame::toUsbFrame(frame);
        // K500 native USB uses 64-byte payload reports with report-id 0.
        constexpr int payload = 64;
        for (int offset = 0; offset < usb.size(); offset += payload) {
            QByteArray report(65, '\0');
            const int length = static_cast<int>(std::min<qsizetype>(payload, usb.size() - offset));
            std::copy_n(usb.constData() + offset, length, report.data() + 1);
            hidWrites.enqueue(std::move(report));
        }
    }
};

K500WinIo::K500WinIo(QObject *parent) : QObject(parent) {
    m_workerThread.setObjectName(QStringLiteral("K500TransportWorkerPOSIX"));
    d = new Impl(this);
    d->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, d, &QObject::deleteLater);
    m_workerThread.start();
}

K500WinIo::~K500WinIo() { shutdown(); }

QStringList K500WinIo::serialPorts() {
    QStringList ports;
    const auto all = QSerialPortInfo::availablePorts();
    for (const auto &port : all) {
        if (!port.portName().isEmpty()) ports.append(port.portName());
    }
    ports.removeDuplicates();
    return ports;
}

bool K500WinIo::openSerial(const QString &portName, QString *error) {
    close();
    if (m_shutdown || !d || !m_workerThread.isRunning()) {
        if (error) *error = QStringLiteral("K500 transport worker not running");
        return false;
    }
    bool ok = false;
    QString label;
    Impl *worker = d;
    QMetaObject::invokeMethod(worker, [&] {
        ok = worker->openSerial(portName, error);
        if (ok) label = worker->label;
    }, Qt::BlockingQueuedConnection);
    if (ok) { m_open = true; m_kind = Kind::Serial; m_label = label; }
    return ok;
}

bool K500WinIo::openUsbHid(quint16 vendorId, quint16 productId,
                            QString *deviceLabel, QString *error) {
    close();
    if (m_shutdown || !d || !m_workerThread.isRunning()) {
        if (error) *error = QStringLiteral("K500 transport worker not running");
        return false;
    }
    bool ok = false;
    QString label;
    Impl *worker = d;
    QMetaObject::invokeMethod(worker, [&] {
        ok = worker->openUsbHid(vendorId, productId, &label, error);
    }, Qt::BlockingQueuedConnection);
    if (ok) {
        m_open = true;
        m_kind = Kind::UsbHid;
        m_label = label;
        if (deviceLabel) *deviceLabel = label;
    }
    return ok;
}

void K500WinIo::close() {
    m_open = false; m_kind = Kind::None; m_label.clear();
    if (m_shutdown || !d || !m_workerThread.isRunning()) return;
    Impl *worker = d;
    QMetaObject::invokeMethod(worker, [worker] { worker->closeTransport(); }, Qt::QueuedConnection);
}

void K500WinIo::shutdown() {
    if (m_shutdown) return;
    m_shutdown = true;
    m_open = false; m_kind = Kind::None; m_label.clear();
    if (d && m_workerThread.isRunning()) {
        Impl *worker = d;
        QMetaObject::invokeMethod(worker, [worker] { worker->closeTransport(); },
                                  Qt::BlockingQueuedConnection);
        m_workerThread.quit();
        m_workerThread.wait();
    }
    d = nullptr;
}

bool K500WinIo::isOpen() const { return m_open; }
K500WinIo::Kind K500WinIo::kind() const { return m_kind; }
QString K500WinIo::label() const { return m_label; }

bool K500WinIo::requestWriteDrain(quint64 token) {
    if (!token || m_shutdown || !m_open || !d || !m_workerThread.isRunning()) return false;
    Impl *worker = d;
    QMetaObject::invokeMethod(worker, [worker, token] {
        worker->pendingDrains.append(token);
        worker->emitDrainedWhenIdle();
    }, Qt::QueuedConnection);
    return true;
}

bool K500WinIo::writeProtocolFrame(const QByteArray &btFrame, QString *error) {
    if (m_shutdown || !m_open || m_kind == Kind::None || !d || !m_workerThread.isRunning()) {
        if (error) *error = QStringLiteral("K500 transport not connected");
        return false;
    }
    if (btFrame.isEmpty()) return true;
    Impl *worker = d;
    const QByteArray frame = btFrame;
    if (!QMetaObject::invokeMethod(worker, [worker, frame] {
        worker->enqueueProtocolFrame(frame);
    }, Qt::QueuedConnection)) {
        if (error) *error = QStringLiteral("Cannot enqueue K500 frame");
        return false;
    }
    ++m_submittedFrameCount;
    return true;
}
