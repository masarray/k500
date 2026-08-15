#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace K500Frame {

quint8 clampByte(int value);
quint8 checksumTwosComplement(const QByteArray &body);
QByteArray build(const QByteArray &bodyWithoutChecksum);
bool verify(const QByteArray &frame);
QByteArray toUsbFrame(const QByteArray &btFrame);
QString hex(const QByteArray &bytes);

} // namespace K500Frame
