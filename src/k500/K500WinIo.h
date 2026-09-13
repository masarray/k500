#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QtGlobal>

class K500WinIo final : public QObject
{
    Q_OBJECT

public:
    enum class Kind {
        None,
        Serial,
        UsbHid,
    };

    explicit K500WinIo(QObject *parent = nullptr);
    ~K500WinIo() override;

    static QStringList serialPorts();

    bool openSerial(const QString &portName, QString *error = nullptr);
    bool openUsbHid(quint16 vendorId, quint16 productId,
                    QString *deviceLabel = nullptr, QString *error = nullptr);
    void close();

    // P3_DETERMINISTIC_SHUTDOWN_V1
    // Permanently stops accepting new work, closes native resources on the
    // transport worker, then joins the worker. Safe to call repeatedly.
    void shutdown();

    bool isOpen() const;
    Kind kind() const;
    QString label() const;

    // Accepts the canonical BT-style K500 frame. USB HID conversion and
    // 64-byte report chunking happen inside the transport worker. A true return
    // means the frame was accepted for ordered asynchronous transmission; any
    // driver failure is reported later through errorOccurred().
    bool writeProtocolFrame(const QByteArray &btFrame, QString *error = nullptr);

signals:
    void bytesReceived(const QByteArray &bytes);
    void errorOccurred(const QString &message);

private:
    // P2_ASYNC_TRANSPORT_WORKER_V1
    // Impl is the sole Windows HANDLE / OVERLAPPED owner and lives on this
    // dedicated thread. The public K500WinIo object remains on the GUI thread.
    class Impl;
    Impl *d = nullptr;
    QThread m_workerThread;

    // GUI-thread mirror only. The worker remains authoritative for native
    // resources; these fields let the existing DeviceManager API stay stable.
    Kind m_kind = Kind::None;
    QString m_label;
    bool m_open = false;
    bool m_shutdown = false;
};
