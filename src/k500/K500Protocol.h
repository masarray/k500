#pragma once

#include <QByteArray>
#include <QString>

struct K500EqBand
{
    QString type = QStringLiteral("BELL");
    double frequencyHz = 1000.0;
    double q = 1.0;
    double gainDb = 0.0;
};

struct K500MusicBlockState
{
    int topMusicVol = 35;
    int musicInitVol = 25;
    int sourceRaw = 2; // Bluetooth
    double input1GainDb = -3.0;
    double input2GainDb = -3.0;
    double bluetoothGainDb = -3.0;
    double uDiskGainDb = -4.0;
    double digitalGainDb = -4.0;
    int key = 0;
};

namespace K500Protocol {

constexpr int TopVolumeMax = 84;

QByteArray heartbeat();
QByteArray handshake();
QByteArray mute(bool enabled);
QByteArray playerCommand(const QString &command);
QByteArray readBlock(quint16 offset, quint16 length);
QByteArray eqWrite(const QString &section, int bandIndexZeroBased, const K500EqBand &band);
QByteArray crossoverWrite(const QString &section,
                          const QString &kind,
                          double frequencyHz,
                          const QString &filterLabel,
                          quint8 musicStateByte = 0x32);
QByteArray topMusicBlock(const K500MusicBlockState &state, const QByteArray &deviceScalars);

quint8 crossoverFilterCode(const QString &label);
bool selfTest(QString *error = nullptr);

} // namespace K500Protocol
