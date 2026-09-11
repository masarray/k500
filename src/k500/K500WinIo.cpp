#include "K500WinIo.h"

#include "K500Frame.h"

#include <QMetaObject>
#include <QPointer>
#include <QQueue>
#include <QTimer>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <QWinEventNotifier>

#include <algorithm>
#include <cstring>
#include <vector>
#endif

class K500WinIo::Impl final : public QObject
{
public:
    explicit Impl(K500WinIo *owner)
        : q(owner)
    {
    }

    ~Impl() override
    {
        closeTransport();
    }

    K500WinIo *q = nullptr;
    Kind kind = Kind::None;
    QString label;

#ifdef Q_OS_WIN
    HANDLE handle = INVALID_HANDLE_VALUE;
    bool closing = false;

    QTimer *serialPoll = nullptr;

    HANDLE hidReadEvent = nullptr;
    OVERLAPPED hidReadOverlapped{};
    QWinEventNotifier *hidReadNotifier = nullptr;
    QByteArray hidReadBuffer;
    int hidInputReportLength = 65;
    bool hidReadPending = false;

    HANDLE hidWriteEvent = nullptr;
    OVERLAPPED hidWriteOverlapped{};
    QWinEventNotifier *hidWriteNotifier = nullptr;
    QByteArray hidWriteBuffer;
    QQueue<QByteArray> hidWriteQueue;
    int hidOutputReportLength = 65;
    bool hidWritePending = false;

    void publishBytes(QByteArray bytes)
    {
        QPointer<K500WinIo> owner(q);
        QMetaObject::invokeMethod(q, [owner, bytes = std::move(bytes)]() mutable {
            if (owner)
                emit owner->bytesReceived(bytes);
        }, Qt::QueuedConnection);
    }

    void publishError(const QString &message)
    {
        if (closing)
            return;
        QPointer<K500WinIo> owner(q);
        QMetaObject::invokeMethod(q, [owner, message] {
            if (owner)
                emit owner->errorOccurred(message);
        }, Qt::QueuedConnection);
    }

    QString winErrorText(DWORD code) const
    {
        wchar_t *message = nullptr;
        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                          | FORMAT_MESSAGE_FROM_SYSTEM
                          | FORMAT_MESSAGE_IGNORE_INSERTS;
        FormatMessageW(flags, nullptr, code, 0,
                       reinterpret_cast<wchar_t *>(&message), 0, nullptr);
        const QString detail = message
            ? QString::fromWCharArray(message).trimmed()
            : QStringLiteral("Windows error %1").arg(code);
        if (message)
            LocalFree(message);
        return detail;
    }

    void emitWinError(const QString &prefix, DWORD code = GetLastError())
    {
        publishError(QStringLiteral("%1: %2").arg(prefix, winErrorText(code)));
    }

    void pollSerial()
    {
        if (closing || kind != Kind::Serial || handle == INVALID_HANDLE_VALUE)
            return;

        DWORD errors = 0;
        COMSTAT stat{};
        if (!ClearCommError(handle, &errors, &stat)) {
            emitWinError(QStringLiteral("Serial status failed"));
            return;
        }
        if (stat.cbInQue == 0)
            return;

        QByteArray data(static_cast<int>(qMin<DWORD>(stat.cbInQue, 4096)), Qt::Uninitialized);
        DWORD read = 0;
        if (!ReadFile(handle, data.data(), static_cast<DWORD>(data.size()), &read, nullptr)) {
            emitWinError(QStringLiteral("Serial read failed"));
            return;
        }
        if (read == 0)
            return;
        data.resize(static_cast<int>(read));
        publishBytes(std::move(data));
    }

    void deliverHidBytes(DWORD bytesRead)
    {
        // Windows HID ReadFile includes report id byte 0 on every report.
        if (bytesRead <= 1)
            return;
        const int payloadSize = static_cast<int>(bytesRead - 1);
        QByteArray payload(payloadSize, Qt::Uninitialized);
        std::memcpy(payload.data(), hidReadBuffer.constData() + 1,
                    static_cast<size_t>(payloadSize));
        publishBytes(std::move(payload));
    }

    void issueHidRead()
    {
        if (closing || kind != Kind::UsbHid || handle == INVALID_HANDLE_VALUE)
            return;

        hidReadBuffer.resize(qMax(2, hidInputReportLength));
        ResetEvent(hidReadEvent);
        ZeroMemory(&hidReadOverlapped, sizeof(hidReadOverlapped));
        hidReadOverlapped.hEvent = hidReadEvent;
        hidReadPending = false;

        DWORD bytesRead = 0;
        const BOOL ok = ReadFile(handle, hidReadBuffer.data(),
                                 static_cast<DWORD>(hidReadBuffer.size()),
                                 &bytesRead, &hidReadOverlapped);
        if (ok) {
            deliverHidBytes(bytesRead);
            QTimer::singleShot(0, this, [this] { issueHidRead(); });
            return;
        }

        const DWORD code = GetLastError();
        if (code == ERROR_IO_PENDING) {
            hidReadPending = true;
            if (hidReadNotifier)
                hidReadNotifier->setEnabled(true);
            return;
        }
        emitWinError(QStringLiteral("USB HID read failed"), code);
    }

    void completeHidRead()
    {
        if (closing || handle == INVALID_HANDLE_VALUE)
            return;
        if (hidReadNotifier)
            hidReadNotifier->setEnabled(false);

        DWORD bytesRead = 0;
        if (!GetOverlappedResult(handle, &hidReadOverlapped, &bytesRead, FALSE)) {
            const DWORD code = GetLastError();
            if (code == ERROR_IO_INCOMPLETE) {
                if (hidReadNotifier)
                    hidReadNotifier->setEnabled(true);
                return;
            }
            hidReadPending = false;
            if (code != ERROR_OPERATION_ABORTED)
                emitWinError(QStringLiteral("USB HID read completion failed"), code);
            return;
        }

        hidReadPending = false;
        deliverHidBytes(bytesRead);
        issueHidRead();
    }

    // P2_NONBLOCKING_HID_WRITE_V1
    // Exactly one HID report is outstanding at a time. The queue therefore
    // preserves K500 command/report ordering without any GUI-thread wait.
    void startNextHidWrite()
    {
        if (closing || kind != Kind::UsbHid || handle == INVALID_HANDLE_VALUE
            || hidWritePending || hidWriteQueue.isEmpty()) {
            return;
        }

        hidWriteBuffer = hidWriteQueue.dequeue();
        ResetEvent(hidWriteEvent);
        ZeroMemory(&hidWriteOverlapped, sizeof(hidWriteOverlapped));
        hidWriteOverlapped.hEvent = hidWriteEvent;

        DWORD written = 0;
        const BOOL ok = WriteFile(handle, hidWriteBuffer.constData(),
                                  static_cast<DWORD>(hidWriteBuffer.size()),
                                  &written, &hidWriteOverlapped);
        if (ok) {
            if (written != static_cast<DWORD>(hidWriteBuffer.size())) {
                publishError(QStringLiteral("USB HID write completed partially (%1/%2 bytes)")
                                 .arg(written).arg(hidWriteBuffer.size()));
                hidWriteQueue.clear();
                hidWriteBuffer.clear();
                return;
            }
            hidWriteBuffer.clear();
            QTimer::singleShot(0, this, [this] { startNextHidWrite(); });
            return;
        }

        const DWORD code = GetLastError();
        if (code == ERROR_IO_PENDING) {
            hidWritePending = true;
            if (hidWriteNotifier)
                hidWriteNotifier->setEnabled(true);
            return;
        }

        hidWriteBuffer.clear();
        hidWriteQueue.clear();
        emitWinError(QStringLiteral("USB HID write failed"), code);
    }

    void completeHidWrite()
    {
        if (closing || handle == INVALID_HANDLE_VALUE)
            return;
        if (hidWriteNotifier)
            hidWriteNotifier->setEnabled(false);

        DWORD written = 0;
        if (!GetOverlappedResult(handle, &hidWriteOverlapped, &written, FALSE)) {
            const DWORD code = GetLastError();
            if (code == ERROR_IO_INCOMPLETE) {
                if (hidWriteNotifier)
                    hidWriteNotifier->setEnabled(true);
                return;
            }
            hidWritePending = false;
            hidWriteBuffer.clear();
            hidWriteQueue.clear();
            if (code != ERROR_OPERATION_ABORTED)
                emitWinError(QStringLiteral("USB HID write completion failed"), code);
            return;
        }

        hidWritePending = false;
        if (written != static_cast<DWORD>(hidWriteBuffer.size())) {
            publishError(QStringLiteral("USB HID write completed partially (%1/%2 bytes)")
                             .arg(written).arg(hidWriteBuffer.size()));
            hidWriteBuffer.clear();
            hidWriteQueue.clear();
            return;
        }

        hidWriteBuffer.clear();
        startNextHidWrite();
    }

    void enqueueProtocolFrame(const QByteArray &btFrame)
    {
        if (closing || handle == INVALID_HANDLE_VALUE || kind == Kind::None) {
            publishError(QStringLiteral("K500 transport is not open"));
            return;
        }

        if (kind == Kind::Serial) {
            DWORD written = 0;
            const BOOL ok = WriteFile(handle, btFrame.constData(),
                                      static_cast<DWORD>(btFrame.size()),
                                      &written, nullptr);
            if (!ok || written != static_cast<DWORD>(btFrame.size())) {
                publishError(QStringLiteral("Serial write failed on %1 (Windows error %2)")
                                 .arg(label).arg(GetLastError()));
            }
            return;
        }

        const QByteArray usbFrame = K500Frame::toUsbFrame(btFrame);
        const int reportLength = qMax(2, hidOutputReportLength);
        const int payloadCapacity = reportLength - 1; // byte 0 = report id 0
        if (payloadCapacity <= 0) {
            publishError(QStringLiteral("Invalid HID output report length"));
            return;
        }

        for (int offset = 0; offset < usbFrame.size(); offset += payloadCapacity) {
            QByteArray report(reportLength, char(0x00));
            const int count = qMin(payloadCapacity, usbFrame.size() - offset);
            std::copy_n(usbFrame.constData() + offset, count, report.data() + 1);
            hidWriteQueue.enqueue(std::move(report));
        }
        startNextHidWrite();
    }

    bool openSerial(const QString &portName, QString *error)
    {
        closeTransport();

        const QString path = QStringLiteral("\\\\.\\%1").arg(portName);
        HANDLE newHandle = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),
                                       GENERIC_READ | GENERIC_WRITE,
                                       0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (newHandle == INVALID_HANDLE_VALUE) {
            if (error)
                *error = QStringLiteral("Cannot open %1 (Windows error %2)")
                             .arg(portName).arg(GetLastError());
            return false;
        }

        DCB dcb{};
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(newHandle, &dcb)) {
            const DWORD code = GetLastError();
            CloseHandle(newHandle);
            if (error)
                *error = QStringLiteral("GetCommState %1 failed (%2)").arg(portName).arg(code);
            return false;
        }

        dcb.BaudRate = CBR_115200;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = TRUE;
        dcb.fParity = FALSE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDsrSensitivity = FALSE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;

        if (!SetCommState(newHandle, &dcb)) {
            const DWORD code = GetLastError();
            CloseHandle(newHandle);
            if (error)
                *error = QStringLiteral("SetCommState %1 failed (%2)").arg(portName).arg(code);
            return false;
        }

        SetupComm(newHandle, 4096, 4096);
        PurgeComm(newHandle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

        COMMTIMEOUTS timeouts{};
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = 0;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 900;
        SetCommTimeouts(newHandle, &timeouts);

        handle = newHandle;
        kind = Kind::Serial;
        label = portName.toUpper();
        closing = false;

        serialPoll = new QTimer(this);
        serialPoll->setInterval(25);
        connect(serialPoll, &QTimer::timeout, this, [this] { pollSerial(); });
        serialPoll->start();
        return true;
    }

    bool openUsbHid(quint16 vendorId, quint16 productId,
                    QString *deviceLabel, QString *error)
    {
        closeTransport();

        GUID hidGuid{};
        HidD_GetHidGuid(&hidGuid);
        HDEVINFO devices = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr,
                                                DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (devices == INVALID_HANDLE_VALUE) {
            if (error)
                *error = QStringLiteral("Cannot enumerate HID devices");
            return false;
        }

        QString matchedPath;
        for (DWORD index = 0;; ++index) {
            SP_DEVICE_INTERFACE_DATA iface{};
            iface.cbSize = sizeof(iface);
            if (!SetupDiEnumDeviceInterfaces(devices, nullptr, &hidGuid, index, &iface)) {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
                continue;
            }

            DWORD required = 0;
            SetupDiGetDeviceInterfaceDetailW(devices, &iface, nullptr, 0, &required, nullptr);
            if (required == 0)
                continue;

            std::vector<unsigned char> storage(required);
            auto *detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(storage.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(devices, &iface, detail, required,
                                                  nullptr, nullptr)) {
                continue;
            }

            HANDLE probe = CreateFileW(detail->DevicePath, 0,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       nullptr, OPEN_EXISTING, 0, nullptr);
            if (probe == INVALID_HANDLE_VALUE)
                continue;

            HIDD_ATTRIBUTES attributes{};
            attributes.Size = sizeof(attributes);
            const bool match = HidD_GetAttributes(probe, &attributes)
                            && attributes.VendorID == vendorId
                            && attributes.ProductID == productId;
            CloseHandle(probe);
            if (match) {
                matchedPath = QString::fromWCharArray(detail->DevicePath);
                break;
            }
        }
        SetupDiDestroyDeviceInfoList(devices);

        if (matchedPath.isEmpty()) {
            if (error)
                *error = QStringLiteral("USB HID DSP AUDIO %1:%2 not found")
                             .arg(vendorId, 4, 16, QLatin1Char('0'))
                             .arg(productId, 4, 16, QLatin1Char('0')).toUpper();
            return false;
        }

        HANDLE newHandle = CreateFileW(reinterpret_cast<LPCWSTR>(matchedPath.utf16()),
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        if (newHandle == INVALID_HANDLE_VALUE) {
            if (error)
                *error = QStringLiteral("K500 USB HID is present but cannot be opened. Close the original K500 app and retry. Windows error %1")
                             .arg(GetLastError());
            return false;
        }

        QString detectedLabel = QStringLiteral("USB HID DSP AUDIO");
        wchar_t product[256]{};
        if (HidD_GetProductString(newHandle, product, sizeof(product))) {
            const QString detected = QString::fromWCharArray(product).trimmed();
            if (!detected.isEmpty())
                detectedLabel = detected;
        }

        hidInputReportLength = 65;
        hidOutputReportLength = 65;
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        HIDP_CAPS caps{};
        if (HidD_GetPreparsedData(newHandle, &preparsed)) {
            if (HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS) {
                hidInputReportLength = qMax<int>(2, caps.InputReportByteLength);
                hidOutputReportLength = qMax<int>(2, caps.OutputReportByteLength);
            }
            HidD_FreePreparsedData(preparsed);
        }

        handle = newHandle;
        kind = Kind::UsbHid;
        label = detectedLabel;
        closing = false;
        HidD_SetNumInputBuffers(handle, 64);

        hidReadEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        hidWriteEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!hidReadEvent || !hidWriteEvent) {
            const DWORD code = GetLastError();
            closeTransport();
            if (error)
                *error = QStringLiteral("Cannot create HID I/O event (%1)").arg(code);
            return false;
        }

        hidReadNotifier = new QWinEventNotifier(hidReadEvent, this);
        hidReadNotifier->setEnabled(false);
        connect(hidReadNotifier, &QWinEventNotifier::activated,
                this, [this] { completeHidRead(); });

        hidWriteNotifier = new QWinEventNotifier(hidWriteEvent, this);
        hidWriteNotifier->setEnabled(false);
        connect(hidWriteNotifier, &QWinEventNotifier::activated,
                this, [this] { completeHidWrite(); });

        issueHidRead();
        if (deviceLabel)
            *deviceLabel = detectedLabel;
        return true;
    }

    void closeTransport()
    {
        closing = true;

        if (serialPoll) {
            serialPoll->stop();
            delete serialPoll;
            serialPoll = nullptr;
        }
        if (hidReadNotifier) {
            hidReadNotifier->setEnabled(false);
            delete hidReadNotifier;
            hidReadNotifier = nullptr;
        }
        if (hidWriteNotifier) {
            hidWriteNotifier->setEnabled(false);
            delete hidWriteNotifier;
            hidWriteNotifier = nullptr;
        }

        hidWriteQueue.clear();

        if (handle != INVALID_HANDLE_VALUE) {
            if (kind == Kind::UsbHid) {
                if (hidReadPending)
                    CancelIoEx(handle, &hidReadOverlapped);
                if (hidWritePending)
                    CancelIoEx(handle, &hidWriteOverlapped);

                DWORD ignored = 0;
                if (hidReadPending)
                    GetOverlappedResult(handle, &hidReadOverlapped, &ignored, TRUE);
                if (hidWritePending)
                    GetOverlappedResult(handle, &hidWriteOverlapped, &ignored, TRUE);
            }
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }

        hidReadPending = false;
        hidWritePending = false;
        hidReadBuffer.clear();
        hidWriteBuffer.clear();

        if (hidReadEvent) {
            CloseHandle(hidReadEvent);
            hidReadEvent = nullptr;
        }
        if (hidWriteEvent) {
            CloseHandle(hidWriteEvent);
            hidWriteEvent = nullptr;
        }
        ZeroMemory(&hidReadOverlapped, sizeof(hidReadOverlapped));
        ZeroMemory(&hidWriteOverlapped, sizeof(hidWriteOverlapped));

        kind = Kind::None;
        label.clear();
        closing = false;
    }
#endif
};

K500WinIo::K500WinIo(QObject *parent)
    : QObject(parent)
{
    m_workerThread.setObjectName(QStringLiteral("K500TransportWorker"));
    d = new Impl(this);
    d->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, d, &QObject::deleteLater);
    m_workerThread.start();
}

K500WinIo::~K500WinIo()
{
    m_open = false;
    m_kind = Kind::None;
    m_label.clear();

    if (d && m_workerThread.isRunning()) {
        Impl *worker = d;
        QMetaObject::invokeMethod(worker, [worker] {
#ifdef Q_OS_WIN
            worker->closeTransport();
#endif
        }, Qt::BlockingQueuedConnection);
        m_workerThread.quit();
        m_workerThread.wait();
    }
    d = nullptr;
}

QStringList K500WinIo::serialPorts()
{
    QStringList ports;
#ifdef Q_OS_WIN
    std::vector<wchar_t> buffer(65536, L'\0');
    const DWORD length = QueryDosDeviceW(nullptr, buffer.data(),
                                         static_cast<DWORD>(buffer.size()));
    if (length == 0)
        return ports;

    const wchar_t *cursor = buffer.data();
    while (*cursor) {
        const QString name = QString::fromWCharArray(cursor);
        if (name.startsWith(QStringLiteral("COM"), Qt::CaseInsensitive)) {
            bool numeric = false;
            name.mid(3).toInt(&numeric);
            if (numeric)
                ports.append(name.toUpper());
        }
        cursor += wcslen(cursor) + 1;
    }

    std::sort(ports.begin(), ports.end(), [](const QString &a, const QString &b) {
        return a.mid(3).toInt() < b.mid(3).toInt();
    });
#endif
    return ports;
}

bool K500WinIo::openSerial(const QString &portName, QString *error)
{
    close();
#ifdef Q_OS_WIN
    if (!d || !m_workerThread.isRunning()) {
        if (error)
            *error = QStringLiteral("K500 transport worker is not running");
        return false;
    }

    bool ok = false;
    QString workerError;
    QString workerLabel;
    Impl *worker = d;
    QMetaObject::invokeMethod(worker, [&] {
        ok = worker->openSerial(portName, &workerError);
        if (ok)
            workerLabel = worker->label;
    }, Qt::BlockingQueuedConnection);

    if (!ok) {
        if (error)
            *error = workerError;
        return false;
    }

    m_open = true;
    m_kind = Kind::Serial;
    m_label = workerLabel;
    return true;
#else
    Q_UNUSED(portName)
    if (error)
        *error = QStringLiteral("Native K500 transport currently requires Windows");
    return false;
#endif
}

bool K500WinIo::openUsbHid(quint16 vendorId, quint16 productId,
                           QString *deviceLabel, QString *error)
{
    close();
#ifdef Q_OS_WIN
    if (!d || !m_workerThread.isRunning()) {
        if (error)
            *error = QStringLiteral("K500 transport worker is not running");
        return false;
    }

    bool ok = false;
    QString workerError;
    QString workerLabel;
    Impl *worker = d;
    QMetaObject::invokeMethod(worker, [&] {
        ok = worker->openUsbHid(vendorId, productId, &workerLabel, &workerError);
    }, Qt::BlockingQueuedConnection);

    if (!ok) {
        if (error)
            *error = workerError;
        return false;
    }

    m_open = true;
    m_kind = Kind::UsbHid;
    m_label = workerLabel;
    if (deviceLabel)
        *deviceLabel = workerLabel;
    return true;
#else
    Q_UNUSED(vendorId)
    Q_UNUSED(productId)
    Q_UNUSED(deviceLabel)
    if (error)
        *error = QStringLiteral("Native K500 transport currently requires Windows");
    return false;
#endif
}

void K500WinIo::close()
{
    m_open = false;
    m_kind = Kind::None;
    m_label.clear();

#ifdef Q_OS_WIN
    if (!d || !m_workerThread.isRunning())
        return;
    Impl *worker = d;
    QMetaObject::invokeMethod(worker, [worker] {
        worker->closeTransport();
    }, Qt::QueuedConnection);
#endif
}

bool K500WinIo::isOpen() const
{
    return m_open;
}

K500WinIo::Kind K500WinIo::kind() const
{
    return m_kind;
}

QString K500WinIo::label() const
{
    return m_label;
}

bool K500WinIo::writeProtocolFrame(const QByteArray &btFrame, QString *error)
{
#ifdef Q_OS_WIN
    if (!m_open || m_kind == Kind::None || !d || !m_workerThread.isRunning()) {
        if (error)
            *error = QStringLiteral("K500 transport is not open");
        return false;
    }
    if (btFrame.isEmpty())
        return true;

    // P2_ASYNC_TRANSPORT_WORKER_V1: this method never waits for a HID driver.
    // The worker serializes protocol frames and HID report chunks in FIFO order.
    Impl *worker = d;
    const QByteArray frame = btFrame;
    QMetaObject::invokeMethod(worker, [worker, frame] {
        worker->enqueueProtocolFrame(frame);
    }, Qt::QueuedConnection);
    return true;
#else
    Q_UNUSED(btFrame)
    if (error)
        *error = QStringLiteral("Native K500 transport currently requires Windows");
    return false;
#endif
}
