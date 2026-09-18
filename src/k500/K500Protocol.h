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
    // MUSIC_TONE_CAPTURED_V1 — -1 means preserve device scalar until the user
    // explicitly edits Music Noise Gate in this session.
    int noiseGateRaw = -1;
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

struct K500ReverbBlockState
{
    int level = 100;
    int direct = 100;
    int hpfHz = 220;
    int lpfHz = 15800;
    int decayMs = 1680;
    int predelayMs = 42;
};

struct K500EchoBlockState
{
    int level = 100;
    int repeat = 2;
    int direct = 100;
    int rightDelayPercent = 0;
    int rightPredelayPercent = 0;
    int hpfHz = 550;
    int lpfHz = 4200;
    int leftDelayMs = 300;
    int leftPredelayMs = 100;
};

struct K500EqBypassImage
{
    quint8 m0 = 0;
    quint8 m1 = 0;
    quint8 m2 = 0;
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
constexpr int ReverbDataLength = 15;
constexpr int EchoDataLength = 22;
constexpr int OutputDataLength = 35;

// K500_NATIVE_VALUE_CONTRACT_V1
// These bounds mirror the manufacturer UI / captured native behavior. They are
// protocol constraints, not cosmetic slider limits. Keep UI, controller and
// transport clamping aligned with docs/K500_NATIVE_VALUE_RANGES.md.
namespace NativeRange {
constexpr double EqGainMinDb = -24.0;
constexpr double EqGainMaxDb = 24.0;
constexpr int FxHpfMinHz = 20;
constexpr int FxHpfMaxHz = 1000;
constexpr int FxLpfMinHz = 4000;
constexpr int FxLpfMaxHz = 16000;

constexpr int ReverbLevelMin = 0;
constexpr int ReverbLevelMax = 100;
constexpr int ReverbDirectMin = 0;
constexpr int ReverbDirectMax = 100;
constexpr int ReverbDecayMinMs = 500;
constexpr int ReverbDecayMaxMs = 5000;
constexpr int ReverbPredelayMinMs = 0;
constexpr int ReverbPredelayMaxMs = 100;

constexpr int EchoLevelMin = 0;
constexpr int EchoLevelMax = 100;
constexpr int EchoRepeatMin = 0;
constexpr int EchoRepeatMax = 10;
constexpr int EchoDirectMin = 0;
constexpr int EchoDirectMax = 100;
constexpr int EchoDelayMinMs = 0;
constexpr int EchoDelayMaxMs = 1000;

constexpr int MusicNoiseGateOffDb = -91; // UI sentinel displayed as OFF
constexpr int MusicNoiseGateMinDb = -90;
constexpr int MusicNoiseGateMaxDb = -50;
constexpr double MusicBassMinDb = -12.0;
constexpr double MusicBassMaxDb = 12.0;
} // namespace NativeRange

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
QByteArray musicBass(double bassDb);
quint8 musicNoiseGateRaw(double gateDb);
QByteArray topMicBlock(const K500MicBlockState &state, const QByteArray &deviceScalars);
QByteArray topEffectBlock(const K500EffectBlockState &state, const QByteArray &deviceScalars);
QByteArray reverbBlock(const K500ReverbBlockState &state, const QByteArray &deviceData);
QByteArray echoBlock(const K500EchoBlockState &state, const QByteArray &deviceData);
bool setEqBypass(K500EqBypassImage &image, const QString &section, bool enabled);
bool eqBypassEnabled(const K500EqBypassImage &image, const QString &section);
QByteArray eqBypassWrite(const K500EqBypassImage &image);
QByteArray outputBlock(const QString &section,
                       const K500OutputBlockState &state,
                       const QByteArray &deviceData);
QByteArray micEqLink(bool enabled);

quint8 crossoverFilterCode(const QString &label);
bool selfTest(QString *error = nullptr);

} // namespace K500Protocol
