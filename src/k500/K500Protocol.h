#pragma once

#include "K500FieldContract.h"

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
    // MUSIC_MAX_NATIVE_CEILING_V1 — physical System/Music Max captures prove
    // this scalar is part of Top Music CMD 0x02 and hard-limits topMusicVol.
    int musicMaxVol = 84;
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
    // MIC_MAX_NATIVE_CEILING_V1 — 2026-10-04 physical sweep proves this
    // scalar shares CMD 0x05 and clamps topMicVol when lowered below it.
    int micMaxVol = 84;
    int fbxLevel = 0;
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
    int outputDelayMs = 0;
};

namespace K500Protocol {

constexpr int TopVolumeMax = 84;
constexpr int ReverbDataLength = 15;
constexpr int EchoDataLength = 22;
constexpr int OutputDataLength = 35;
// Canonical manufacturer active-memory geometry. Keep transport/readback
// consumers on these shared constants instead of duplicating magic numbers.
constexpr int ActiveMemorySize = 0x03AB;       // 939 bytes, 0x0000..0x03AA
constexpr int ActiveMemoryBlockSize = 0x003A;  // 58-byte CMD 0x40 chunks

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
// ECHO_TIMING_ENDPOINTS_20261004_V1 — physical native UI endpoint sweeps.
constexpr int EchoRightDelayMinPercent = -50;
constexpr int EchoRightDelayMaxPercent = 50;
constexpr int EchoRightPredelayMinPercent = -50;
constexpr int EchoRightPredelayMaxPercent = 50;
constexpr int EchoLeftPredelayMinMs = 0;
constexpr int EchoLeftPredelayMaxMs = 100;

constexpr int MusicNoiseGateOffDb = -91; // UI sentinel displayed as OFF
constexpr int MusicNoiseGateMinDb = -90;
constexpr int MusicNoiseGateMaxDb = -50;
constexpr double MusicBassMinDb = -12.0;
constexpr double MusicBassMaxDb = 12.0;

constexpr int MicFbxMinLevel = K500FieldContract::Field::MicFbe.rawMin;
constexpr int MicFbxMaxLevel = K500FieldContract::Field::MicFbe.rawMax;

constexpr int StartupLevelMin = 0;
constexpr int StartupLevelMax = TopVolumeMax;
constexpr int UsbRecordVolMin = 1;
constexpr int UsbRecordVolMax = 6;
constexpr int UDiskRecordVolMin = 1;
constexpr int UDiskRecordVolMax = 6;
constexpr int DanceMicThresholdMinDb = -60;
constexpr int DanceMicThresholdMaxDb = 0;
constexpr int DanceMicHoldMinSec = 1;
constexpr int DanceMicHoldMaxSec = 30;
constexpr int OutputDelayMinMs = 0;
constexpr int OutputDelayMaxMs = 50;
constexpr int BtNameMaxLength = 8;
} // namespace NativeRange

// CAPTURED_ACTIVE_MEMORY_OFFSETS_20261004_V1
// These are direct indices into the 939-byte active-memory image returned by
// CMD 0x40. They are NOT .k500/file scalar offsets and must never pass through
// fileU8()/fileU16() translation.
namespace ReadbackOffset {
constexpr int MusicNoiseGate = 0x0005;
constexpr int MicHpType = 0x0013;
constexpr int MicLpType = 0x0014;
constexpr int MicFbx = K500FieldContract::Field::MicFbe.activeOffset;
constexpr int MainHpType = 0x002C;
constexpr int MainLpType = 0x002E;
constexpr int SurroundHpType = 0x0040;
constexpr int SurroundLpType = 0x0042;
constexpr int CenterHpType = 0x0054;
constexpr int CenterLpType = 0x0056;
constexpr int SubHpType = 0x0068;
constexpr int SubLpType = 0x006A;
constexpr int MainLDelay = 0x00CB;
constexpr int MainRDelay = 0x00CD;
constexpr int SurroundLDelay = 0x00CF;
constexpr int SurroundRDelay = 0x00D1;
constexpr int CenterDelay = 0x00D3;
constexpr int SubDelay = 0x00D5;
// ADJ_MANNER_VR_OFF_READBACK_20261004_V1 — paired reconnect captures.
constexpr int AdjMannerVrOff = 0x008C;
// MIC_CROSSOVER_TAIL_DONOR_20261004_V1 — physical donor-isolation capture
// proves Mic CMD 0x11 final data byte mirrors Music Input1 Gain raw.
constexpr int MusicInput1Gain = 0x0016;
constexpr int MusicBass = 0x00DF;
static_assert(MicHpType == K500FieldContract::Field::MicHpType.activeOffset);
static_assert(MicLpType == K500FieldContract::Field::MicLpType.activeOffset);
static_assert(AdjMannerVrOff == K500FieldContract::Field::AdjMannerVrOff.activeOffset);
static_assert(MusicInput1Gain
              == K500FieldContract::ScalarGeometry::activeOffsetForFileScalar(
                     K500FieldContract::FileOffset::MusicInput1Gain));
} // namespace ReadbackOffset

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
                          quint8 preservedStateByte = 0x00);
QByteArray topMusicBlock(const K500MusicBlockState &state, const QByteArray &deviceScalars);
QByteArray musicBass(double bassDb);
quint8 musicNoiseGateRaw(double gateDb);
bool musicNoiseGateRawValid(quint8 raw);
int musicNoiseGateDbFromRaw(quint8 raw);
bool musicBassRawValid(quint8 raw);
double musicBassDbFromRaw(quint8 raw);
QByteArray topMicBlock(const K500MicBlockState &state, const QByteArray &deviceScalars);
QByteArray topEffectBlock(const K500EffectBlockState &state, const QByteArray &deviceScalars);
QByteArray effectInitLevel(int initLevel, int topEffectVol);
QByteArray usbRecordVolume(int levelOneBased);
QByteArray uDiskRecordVolume(int levelOneBased);
QByteArray danceMicTrigger(int thresholdDb, int holdSeconds);
QByteArray btNameSet(const QString &name);
QByteArray btNameReset();
QByteArray adjMannerVrOff(bool enabled);
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
bool crossoverFilterCodeValid(quint8 code);
// MUSIC_CROSSOVER_TYPE_READBACK_V1 — manufacturer reconnect captures map
// Music HP Type to activeMemory[0x0007] and LP Type to activeMemory[0x0008].
// Decode the shared native 0..7 filter enum into the UI labels used by K500.
QString crossoverFilterLabel(quint8 code, bool highPass);
bool selfTest(QString *error = nullptr);

} // namespace K500Protocol
