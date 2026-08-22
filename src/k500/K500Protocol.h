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

struct K500MicBlockState
{
    int topMicVol = 35;
    int micInitVol = 25;
    int fbxLevel = 7;
    int micAVol = 96;
    int micBVol = 96;
    int compThresholdDb = -12;
    int compRatio = 3;
    int attackMs = 10;
    double releaseSec = 0.2;
};

struct K500EffectBlockState
{
    int topEffectVol = 35;
    int effectInitLevel = 25;
};

struct K500OutputBlockState
{
    double lVolDb = 0.0;
    double rVolDb = 0.0;
    double outputVolDb = 0.0;
    int micDirect = 0;
    int musicLevel = 0;
    int reverbLevel = 0;
    int echoLevel = 0;
    int compThresholdDb = -20;
    int compRatio = 1;
    int attackMs = 10;
    double releaseSec = 0.1;
    int lDelayMs = 0;
    int rDelayMs = 0;
};

namespace K500Protocol {

constexpr int TopVolumeMax = 84;
constexpr int OutputDataLength = 35;

QByteArray heartbeat();
QByteArray handshake();
QByteArray mute(bool enabled);
QByteArray playerCommand(const QString &command);
QByteArray readBlock(quint16 offset, quint16 length, quint8 mode = 0x63);
QByteArray eqWrite(const QString &section, int bandIndexZeroBased, const K500EqBand &band);
QByteArray crossoverWrite(const QString &section,
                          const QString &kind,
                          double frequencyHz,
                          const QString &filterLabel,
                          quint8 musicStateByte = 0x32);
QByteArray topMusicBlock(const K500MusicBlockState &state, const QByteArray &deviceScalars);
QByteArray topMicBlock(const K500MicBlockState &state, const QByteArray &deviceScalars);
QByteArray topEffectBlock(const K500EffectBlockState &state, const QByteArray &deviceScalars);
QByteArray outputBlock(const QString &section,
                       const K500OutputBlockState &state,
                       const QByteArray &deviceData);
QByteArray micEqLink(bool enabled);

quint8 crossoverFilterCode(const QString &label);
bool selfTest(QString *error = nullptr);

} // namespace K500Protocol
