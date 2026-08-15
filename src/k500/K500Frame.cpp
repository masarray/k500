#include "K500Frame.h"

#include <QStringList>

namespace {
quint8 u8(char value)
{
    return static_cast<quint8>(static_cast<unsigned char>(value));
}
}

namespace K500Frame {

quint8 clampByte(int value)
{
    return static_cast<quint8>(qBound(0, value, 255));
}

quint8 checksumTwosComplement(const QByteArray &body)
{
    quint8 sum = 0;
    for (const char ch : body)
        sum = static_cast<quint8>(sum + u8(ch));
    return static_cast<quint8>(0u - sum);
}

QByteArray build(const QByteArray &bodyWithoutChecksum)
{
    QByteArray frame;
    frame.reserve(bodyWithoutChecksum.size() + 2);
    frame.append(char(0xAA));
    frame.append(bodyWithoutChecksum);
    frame.append(char(checksumTwosComplement(bodyWithoutChecksum)));
    return frame;
}

bool verify(const QByteArray &frame)
{
    if (frame.size() < 4)
        return false;
    const quint8 head = u8(frame.front());
    if (head != 0xAA && head != 0x55)
        return false;

    quint8 sum = 0;
    for (qsizetype i = 1; i < frame.size(); ++i)
        sum = static_cast<quint8>(sum + u8(frame.at(i)));
    return sum == 0;
}

QByteArray toUsbFrame(const QByteArray &btFrame)
{
    if (btFrame.size() < 4 || u8(btFrame.front()) != 0xAA)
        return btFrame;

    const int bodyLength = u8(btFrame.at(1));
    if (bodyLength <= 0 || btFrame.size() < bodyLength + 3)
        return btFrame;

    QByteArray body = btFrame.mid(2, bodyLength);
    // Native USB read-block uses mode 0x00; Bluetooth uses 0x63.
    if (body.size() == 6 && u8(body.at(0)) == 0x40)
        body[5] = char(0x00);

    QByteArray usb;
    usb.reserve(body.size() + 4);
    usb.append(char(0xAA));
    usb.append(char(bodyLength & 0xFF));
    usb.append(char((bodyLength >> 8) & 0xFF));
    usb.append(body);

    quint8 sum = 0;
    for (qsizetype i = 1; i < usb.size(); ++i)
        sum = static_cast<quint8>(sum + u8(usb.at(i)));
    usb.append(char(static_cast<quint8>(0u - sum)));
    return usb;
}

QString hex(const QByteArray &bytes)
{
    QStringList parts;
    parts.reserve(bytes.size());
    for (const char ch : bytes)
        parts.append(QStringLiteral("%1").arg(u8(ch), 2, 16, QLatin1Char('0')).toUpper());
    return parts.join(QLatin1Char(' '));
}

} // namespace K500Frame
