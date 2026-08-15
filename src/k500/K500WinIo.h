#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <memory>

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

    bool isOpen() const;
    Kind kind() const;
    QString label() const;

    // Accepts the canonical BT-style K500 frame. USB HID conversion and
    // 64-byte report chunking happen inside this transport boundary.
    bool writeProtocolFrame(const QByteArray &btFrame, QString *error = nullptr);

signals:
    void bytesReceived(const QByteArray &bytes);
    void errorOccurred(const QString &message);

private:
    class Impl;
    std::unique_ptr<Impl> d;
};
