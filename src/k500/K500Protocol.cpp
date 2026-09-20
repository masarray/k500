#include "K500Protocol.h"

#include "K500Frame.h"

#include <QHash>
#include <QtMath>
#include <initializer_list>

namespace {
quint8 byteFromChar(char value)
{
    return static_cast<quint8>(static_cast<unsigned char>(value));
}

void appendU16Le(QByteArray &out, int value)
{
    const quint16 v = static_cast<quint16>(qBound(0, value, 65535));
    out.append(char(v & 0xFF));
    out.append(char((v >> 8) & 0xFF));
}

void writeU16Le(QByteArray &out, int offset, int value)
{
    if (offset < 0 || offset + 1 >= out.size())
        return;
    const quint16 v = static_cast<quint16>(qBound(0, value, 65535));
    out[offset] = char(v & 0xFF);
    out[offset + 1] = char((v >> 8) & 0xFF);
}

quint8 eqTypeNibble(const QString &type)
{
    const QString normalized = type.trimmed().toUpper();
    if (normalized == QStringLiteral("LS") || normalized.contains(QStringLiteral("LOW SHELF")))
        return 0x10;
    if (normalized == QStringLiteral("HS") || normalized.contains(QStringLiteral("HIGH SHELF")))
        return 0x20;
    return 0x00;
}

int eqSectionId(const QString &section)
{
    static const QHash<QString, int> ids{
        {QStringLiteral("micA"), 0x00},
        {QStringLiteral("micB"), 0x01},
        {QStringLiteral("music"), 0x02},
        {QStringLiteral("main"), 0x03},
        {QStringLiteral("surround"), 0x05},
        {QStringLiteral("center"), 0x07},
        {QStringLiteral("sub"), 0x08},
        {QStringLiteral("reverb"), 0x09},
        {QStringLiteral("echo"), 0x0A},
    };
    return ids.value(section, -1);
}

int outputSectionId(const QString &section)
{
    static const QHash<QString, int> ids{
        {QStringLiteral("main"), 0x00},
        {QStringLiteral("surround"), 0x02},
        {QStringLiteral("center"), 0x04},
        {QStringLiteral("sub"), 0x05},
    };
    return ids.value(section, -1);
}

int crossoverSelector(const QString &section, const QString &kind)
{
    static const QHash<QString, int> hpf{
        {QStringLiteral("mic"), 0x00}, {QStringLiteral("micA"), 0x00}, {QStringLiteral("micB"), 0x00},
        {QStringLiteral("music"), 0x02}, {QStringLiteral("main"), 0x04},
        {QStringLiteral("surround"), 0x08}, {QStringLiteral("center"), 0x0C},
        {QStringLiteral("sub"), 0x0E},
    };
    static const QHash<QString, int> lpf{
        {QStringLiteral("mic"), 0x01}, {QStringLiteral("micA"), 0x01}, {QStringLiteral("micB"), 0x01},
        {QStringLiteral("music"), 0x03}, {QStringLiteral("main"), 0x05},
        {QStringLiteral("surround"), 0x09}, {QStringLiteral("center"), 0x0D},
        {QStringLiteral("sub"), 0x0F},
    };
    return kind.compare(QStringLiteral("hpf"), Qt::CaseInsensitive) == 0
        ? hpf.value(section, -1)
        : lpf.value(section, -1);
}

QByteArray bytes(std::initializer_list<int> values)
{
    QByteArray out;
    out.reserve(static_cast<qsizetype>(values.size()));
    for (const int value : values)
        out.append(char(value & 0xFF));
    return out;
}

quint8 outDbToRaw(double db)
{
    return K500Frame::clampByte(qRound(db * 2.0 + 75.0));
}

bool eqBypassSpec(const QString &section, int *byteIndex, quint8 *mask)
{
    const QString key = section.trimmed().toLower();
    if (key == QStringLiteral("mic") || key == QStringLiteral("mica") || key == QStringLiteral("micb")) { *byteIndex = 0; *mask = 0x60; return true; }
    if (key == QStringLiteral("music"))    { *byteIndex = 0; *mask = 0x80; return true; }
    if (key == QStringLiteral("main"))     { *byteIndex = 1; *mask = 0x01; return true; }
    if (key == QStringLiteral("surround")) { *byteIndex = 1; *mask = 0x04; return true; }
    if (key == QStringLiteral("center"))   { *byteIndex = 1; *mask = 0x10; return true; }
    if (key == QStringLiteral("sub") || key == QStringLiteral("subwoofer")) { *byteIndex = 1; *mask = 0x40; return true; }
    if (key == QStringLiteral("reverb"))   { *byteIndex = 2; *mask = 0x01; return true; }
    if (key == QStringLiteral("echo"))     { *byteIndex = 2; *mask = 0x02; return true; }
    return false;
}
}


namespace K500Protocol {

QByteArray heartbeat()
{
    return K500Frame::build(bytes({0x01, 0x1C}));
}

QByteArray handshake()
{
    return K500Frame::build(bytes({0x01, 0x3F}));
}

QByteArray mute(bool enabled)
{
    return K500Frame::build(bytes({0x03, 0x15, enabled ? 0x01 : 0x00, 0x00}));
}

QByteArray playerCommand(const QString &command)
{
    int action = 0x02;
    if (command.compare(QStringLiteral("rewind"), Qt::CaseInsensitive) == 0)
        action = 0x00;
    else if (command.compare(QStringLiteral("forward"), Qt::CaseInsensitive) == 0)
        action = 0x01;
    return K500Frame::build(bytes({0x03, 0x06, action, 0x05}));
}

QByteArray readBlock(quint16 offset, quint16 length, quint8 mode)
{
    QByteArray body;
    body.reserve(7);
    body.append(char(0x06));
    body.append(char(0x40));
    appendU16Le(body, offset);
    appendU16Le(body, length);
    body.append(char(mode));
    return K500Frame::build(body);
}

QByteArray eqWrite(const QString &section, int bandIndexZeroBased, const K500EqBand &band)
{
    const int sectionId = eqSectionId(section);
    if (sectionId < 0)
        return {};

    const int frequency = qBound(20, qRound(band.frequencyHz), 20000);
    const int qValue = qBound(1, qRound(band.q * 10.0), 250);
    const double gain = qBound(NativeRange::EqGainMinDb, band.gainDb, NativeRange::EqGainMaxDb);
    const int gainMagnitude = qBound(0, qRound(qAbs(gain) * 10.0), 240);
    const quint8 typeSign = static_cast<quint8>(eqTypeNibble(band.type) | (gain < 0.0 ? 0x80 : 0x00));

    QByteArray body;
    body.reserve(10);
    body.append(char(0x09));
    body.append(char(0x03));
    body.append(char(sectionId));
    body.append(char(K500Frame::clampByte(bandIndexZeroBased)));
    appendU16Le(body, frequency);
    body.append(char(qValue));
    body.append(char(typeSign));
    body.append(char(gainMagnitude));
    body.append(char(section == QStringLiteral("music") ? 0x60 : 0x00));
    return K500Frame::build(body);
}

quint8 crossoverFilterCode(const QString &label)
{
    const QString normalized = label.trimmed().toUpper();
    // CROSSOVER_NATIVE_BYPASS_TYPE0_V1
    // Native K500 keeps the cutoff anchor unchanged while the type dropdown is
    // set to bypass. The preset format uses the same 0x03xx/0x04xx type family,
    // with active filters occupying contiguous codes 0x01..0x07; code 0x00 is
    // therefore the native bypass member of that enum. Keep this behind the
    // physical-device acceptance gate just like every newly replayed command.
    if (normalized == QStringLiteral("BYPASS")) return 0x00;
    if (normalized.contains(QStringLiteral("BESSEL 12"))) return 0x01;
    if (normalized.contains(QStringLiteral("BUTTER 12"))) return 0x02;
    if (normalized.contains(QStringLiteral("BESSEL 18"))) return 0x03;
    if (normalized.contains(QStringLiteral("BUTTER 18"))) return 0x04;
    if (normalized.contains(QStringLiteral("BESSEL 24"))) return 0x05;
    if (normalized.contains(QStringLiteral("BUTTER 24"))) return 0x06;
    if (normalized.contains(QStringLiteral("LR 24"))) return 0x07;
    return 0x02;
}

QByteArray crossoverWrite(const QString &section,
                          const QString &kind,
                          double frequencyHz,
                          const QString &filterLabel,
                          quint8 musicStateByte)
{
    const int selector = crossoverSelector(section, kind);
    if (selector < 0)
        return {};

    const int frequency = qBound(20, qRound(frequencyHz), 20000);
    QByteArray body;
    body.reserve(7);
    body.append(char(0x06));
    body.append(char(0x11));
    body.append(char(selector));
    body.append(char(crossoverFilterCode(filterLabel)));
    appendU16Le(body, frequency);
    body.append(char(section == QStringLiteral("music") ? musicStateByte : 0x00));
    return K500Frame::build(body);
}

quint8 musicNoiseGateRaw(double gateDb)
{
    // MUSIC_TONE_CAPTURED_V1
    // Native UI domain: OFF, then -90..-50 dB. Raw 0 is the dedicated OFF
    // sentinel; raw 1..41 maps linearly to -90..-50 dB.
    if (gateDb <= NativeRange::MusicNoiseGateOffDb)
        return 0;
    const int db = qBound(NativeRange::MusicNoiseGateMinDb, qRound(gateDb),
                          NativeRange::MusicNoiseGateMaxDb);
    return K500Frame::clampByte(db + 91);
}

QByteArray musicBass(double bassDb)
{
    // MUSIC_TONE_CAPTURED_V1 — CMD 0x0C selector 0x02, 0.1 dB encoding.
    const double clamped = qBound(NativeRange::MusicBassMinDb, bassDb,
                                  NativeRange::MusicBassMaxDb);
    const quint16 raw = static_cast<quint16>(qRound((clamped + 12.0) * 10.0));
    return K500Frame::build(bytes({
        0x06, 0x0C, 0x02, 0x00,
        raw & 0xFF, (raw >> 8) & 0xFF, 0x09,
    }));
}

QByteArray topMusicBlock(const K500MusicBlockState &state, const QByteArray &deviceScalars)
{
    const auto mirrored = [&deviceScalars](int offset, int fallback) -> quint8 {
        if (offset >= 0 && offset < deviceScalars.size())
            return byteFromChar(deviceScalars.at(offset));
        return K500Frame::clampByte(fallback);
    };

    QByteArray body;
    body.reserve(14);
    body.append(char(0x0D));
    body.append(char(0x02));
    body.append(char(K500Frame::clampByte(qBound(0, state.topMusicVol, TopVolumeMax))));
    body.append(char(mirrored(0x03, state.musicInitVol)));
    body.append(char(mirrored(0x04, TopVolumeMax)));
    // MUSIC_SOURCE_SIX_WAY_V1 — INPUT1, INPUT2, BT, UDISK, OPTIC, UAUDIO.
    body.append(char(K500Frame::clampByte(qBound(0, state.sourceRaw, 5))));
    body.append(char(K500Frame::clampByte(qRound(state.input1GainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.input2GainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.bluetoothGainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.uDiskGainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.digitalGainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qBound(-7, state.key, 7) + 7)));
    body.append(char(state.noiseGateRaw >= 0
                         ? K500Frame::clampByte(qBound(0, state.noiseGateRaw, 41))
                         : mirrored(0x1B, 0x00)));
    body.append(char(mirrored(0x07, 0x02)));
    return K500Frame::build(body);
}

QByteArray topMicBlock(const K500MicBlockState &state, const QByteArray &deviceScalars)
{
    // FBX_NATIVE_0_4_CAPTURED_V1 + P1_TOP_MIC_VERIFIED_V1
    // Paired physical captures prove CMD 0x05 carries FBX as one direct byte
    // in the 0..4 domain. The following byte is a fixed 0x00 in every captured
    // level and is NOT active-memory neighbour 0x1C.
    const auto mirrored = [&deviceScalars](int offset, int fallback) -> quint8 {
        if (offset >= 0 && offset < deviceScalars.size())
            return byteFromChar(deviceScalars.at(offset));
        return K500Frame::clampByte(fallback);
    };

    QByteArray body;
    body.reserve(15);
    body.append(char(0x0E));
    body.append(char(0x05));
    body.append(char(K500Frame::clampByte(qBound(0, state.topMicVol, TopVolumeMax))));
    body.append(char(mirrored(0x0A, state.micInitVol)));
    body.append(char(mirrored(0x0B, TopVolumeMax)));
    body.append(char(mirrored(0x0E, 0x0B)));
    body.append(char(K500Frame::clampByte(
        qBound(NativeRange::MicFbxMinLevel, state.fbxLevel, NativeRange::MicFbxMaxLevel))));
    body.append(char(0x00));
    body.append(char(K500Frame::clampByte(state.micAVol)));
    body.append(char(K500Frame::clampByte(state.micBVol)));
    body.append(char(K500Frame::clampByte(state.compThresholdDb + 50)));
    body.append(char(K500Frame::clampByte(state.compRatio)));
    body.append(char(K500Frame::clampByte(state.attackMs)));
    body.append(char(K500Frame::clampByte(qRound(state.releaseSec * 10.0))));
    body.append(char(0x00));
    return K500Frame::build(body);
}

QByteArray topEffectBlock(const K500EffectBlockState &state, const QByteArray &deviceScalars)
{
    // P1_TOP_EFFECT_VERIFIED_V1 — exact donor CMD 0x09 layout.
    quint8 init = K500Frame::clampByte(state.effectInitLevel);
    if (deviceScalars.size() > 0x15)
        init = byteFromChar(deviceScalars.at(0x15));
    return K500Frame::build(bytes({
        0x03, 0x09,
        K500Frame::clampByte(qBound(0, state.topEffectVol, TopVolumeMax)),
        init,
    }));
}

QByteArray reverbBlock(const K500ReverbBlockState &state, const QByteArray &deviceData)
{
    // REVERB_CMD0B_CAPTURED_V1 — HHD native KTV delta captures prove that
    // Level, Direct, HPF, LPF, Decay and Predelay are one full CMD 0x0B block.
    // Preserve every unproven neighbour from the current device readback seed.
    QByteArray data = deviceData.left(ReverbDataLength);
    while (data.size() < ReverbDataLength)
        data.append(char(0));

    data[0] = char(K500Frame::clampByte(qBound(NativeRange::ReverbLevelMin, state.level,
                                              NativeRange::ReverbLevelMax)));
    data[2] = char(K500Frame::clampByte(qBound(NativeRange::ReverbDirectMin, state.direct,
                                              NativeRange::ReverbDirectMax)));
    writeU16Le(data, 6, qBound(NativeRange::FxHpfMinHz, state.hpfHz, NativeRange::FxHpfMaxHz));
    writeU16Le(data, 8, qBound(NativeRange::FxLpfMinHz, state.lpfHz, NativeRange::FxLpfMaxHz));
    writeU16Le(data, 10, qBound(NativeRange::ReverbDecayMinMs, state.decayMs,
                                NativeRange::ReverbDecayMaxMs));
    writeU16Le(data, 12, qBound(NativeRange::ReverbPredelayMinMs, state.predelayMs,
                                NativeRange::ReverbPredelayMaxMs));

    QByteArray body;
    body.reserve(17);
    body.append(char(0x10));
    body.append(char(0x0B));
    body.append(data);
    return K500Frame::build(body);
}

QByteArray echoBlock(const K500EchoBlockState &state, const QByteArray &deviceData)
{
    // ECHO_CMD0D_CAPTURED_V1 — native HHD delta captures prove that the Echo
    // section is one 22-byte full image. Patch only byte-verified controls and
    // preserve every unproven neighbour from device readback.
    QByteArray data = deviceData.left(EchoDataLength);
    while (data.size() < EchoDataLength)
        data.append(char(0));

    data[1] = char(K500Frame::clampByte(qBound(NativeRange::EchoLevelMin, state.level,
                                              NativeRange::EchoLevelMax)));
    data[2] = char(K500Frame::clampByte(qBound(NativeRange::EchoRepeatMin, state.repeat,
                                              NativeRange::EchoRepeatMax)));
    data[6] = char(K500Frame::clampByte(qBound(NativeRange::EchoDirectMin, state.direct,
                                              NativeRange::EchoDirectMax)));
    data[7] = char(K500Frame::clampByte(qBound(-50, state.rightDelayPercent, 50) + 50));
    data[8] = char(K500Frame::clampByte(qBound(-50, state.rightPredelayPercent, 50) + 50));
    writeU16Le(data, 9, qBound(NativeRange::FxHpfMinHz, state.hpfHz, NativeRange::FxHpfMaxHz));
    writeU16Le(data, 11, qBound(NativeRange::FxLpfMinHz, state.lpfHz, NativeRange::FxLpfMaxHz));
    writeU16Le(data, 13, qBound(NativeRange::EchoDelayMinMs, state.leftDelayMs,
                                NativeRange::EchoDelayMaxMs));
    writeU16Le(data, 15, qBound(0, state.leftPredelayMs, 65535));

    QByteArray body;
    body.reserve(24);
    body.append(char(0x17));
    body.append(char(0x0D));
    body.append(data);
    return K500Frame::build(body);
}

bool setEqBypass(K500EqBypassImage &image, const QString &section, bool enabled)
{
    int byteIndex = -1;
    quint8 mask = 0;
    if (!eqBypassSpec(section, &byteIndex, &mask))
        return false;
    quint8 *target = byteIndex == 0 ? &image.m0 : (byteIndex == 1 ? &image.m1 : &image.m2);
    *target = enabled ? static_cast<quint8>(*target | mask)
                      : static_cast<quint8>(*target & static_cast<quint8>(~mask));
    return true;
}

bool eqBypassEnabled(const K500EqBypassImage &image, const QString &section)
{
    int byteIndex = -1;
    quint8 mask = 0;
    if (!eqBypassSpec(section, &byteIndex, &mask))
        return false;
    const quint8 value = byteIndex == 0 ? image.m0 : (byteIndex == 1 ? image.m1 : image.m2);
    return (value & mask) == mask;
}

QByteArray eqBypassWrite(const K500EqBypassImage &image)
{
    // EQ_BYPASS_24BIT_CAPTURED_V2 — native KTV writes the complete shared
    // 3-byte bypass image. Preserve every unrelated/reserved bit via RMW.
    return K500Frame::build(bytes({0x06, 0x0F, 0x40, image.m0, image.m1, image.m2, 0x02}));
}

QByteArray outputBlock(const QString &section,
                       const K500OutputBlockState &state,
                       const QByteArray &deviceData)
{
    // P1_OUTPUT_BLOCK_VERIFIED_V1 — preserve all unknown/reserved bytes from
    // device truth, then patch only donor-verified positions.
    const int sectionId = outputSectionId(section);
    if (sectionId < 0)
        return {};

    QByteArray data = deviceData.left(OutputDataLength);
    while (data.size() < OutputDataLength)
        data.append(char(0));

    if (section == QStringLiteral("main")) {
        data[0] = char(outDbToRaw(state.lVolDb));
        data[2] = char(outDbToRaw(state.rVolDb));
        data[4] = char(K500Frame::clampByte(state.micDirect));
        data[6] = char(K500Frame::clampByte(state.musicLevel));
        data[8] = char(K500Frame::clampByte(state.reverbLevel));
        data[10] = char(K500Frame::clampByte(state.echoLevel));
    } else if (section == QStringLiteral("surround")) {
        data[0] = char(outDbToRaw(state.lVolDb));
        data[2] = char(outDbToRaw(state.rVolDb));
        data[4] = char(K500Frame::clampByte(state.micDirect));
        data[6] = char(K500Frame::clampByte(state.musicLevel));
        data[8] = char(K500Frame::clampByte(state.reverbLevel));
        data[10] = char(K500Frame::clampByte(state.echoLevel));
        writeU16Le(data, 16, state.lDelayMs);
        writeU16Le(data, 18, state.rDelayMs);
    } else {
        data[0] = char(outDbToRaw(state.outputVolDb));
        data[4] = char(K500Frame::clampByte(state.micDirect));
        data[6] = char(K500Frame::clampByte(state.musicLevel));
        data[8] = char(K500Frame::clampByte(state.reverbLevel));
        data[10] = char(K500Frame::clampByte(state.echoLevel));
    }

    data[12] = char(K500Frame::clampByte(state.compThresholdDb + 50));
    data[13] = char(K500Frame::clampByte(state.compRatio));
    data[14] = char(K500Frame::clampByte(state.attackMs));
    data[15] = char(K500Frame::clampByte(qRound(state.releaseSec * 10.0)));

    QByteArray body;
    body.reserve(38);
    body.append(char(0x25));
    body.append(char(0x0E));
    body.append(char(sectionId));
    body.append(data);
    return K500Frame::build(body);
}

QByteArray micEqLink(bool enabled)
{
    // P1_MIC_EQ_LINK_VERIFIED_V1 — exact capture including native tail bytes.
    return K500Frame::build(enabled
        ? bytes({0x04, 0x3C, 0x01, 0x01, 0x9E})
        : bytes({0x04, 0x3C, 0x00, 0x00, 0xC4}));
}

bool selfTest(QString *error)
{
    // P0_PROTOCOL_GOLDEN_VECTORS_V1
    // P1_PROTOCOL_GOLDEN_VECTORS_V1
    // REVERB_CMD0B_CAPTURED_V1
    // ECHO_CMD0D_CAPTURED_V1
    const auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    const auto expect = [&fail](const QByteArray &actual, std::initializer_list<int> expected,
                                const QString &label) {
        if (actual != bytes(expected))
            return fail(label + QStringLiteral(" frame mismatch"));
        return true;
    };

    if (!expect(heartbeat(), {0xAA, 0x01, 0x1C, 0xE3}, QStringLiteral("heartbeat"))) return false;
    if (!expect(K500Frame::toUsbFrame(heartbeat()), {0xAA, 0x01, 0x00, 0x1C, 0xE3}, QStringLiteral("USB heartbeat"))) return false;
    if (!expect(handshake(), {0xAA, 0x01, 0x3F, 0xC0}, QStringLiteral("handshake"))) return false;

    if (!expect(mute(false), {0xAA, 0x03, 0x15, 0x00, 0x00, 0xE8}, QStringLiteral("mute off"))) return false;
    if (!expect(mute(true), {0xAA, 0x03, 0x15, 0x01, 0x00, 0xE7}, QStringLiteral("mute on"))) return false;
    if (!expect(playerCommand(QStringLiteral("rewind")), {0xAA, 0x03, 0x06, 0x00, 0x05, 0xF2}, QStringLiteral("rewind"))) return false;
    if (!expect(playerCommand(QStringLiteral("forward")), {0xAA, 0x03, 0x06, 0x01, 0x05, 0xF1}, QStringLiteral("forward"))) return false;
    if (!expect(playerCommand(QStringLiteral("playPause")), {0xAA, 0x03, 0x06, 0x02, 0x05, 0xF0}, QStringLiteral("play/pause"))) return false;

    if (!expect(readBlock(0x0000, 0x003A, 0x63), {0xAA, 0x06, 0x40, 0x00, 0x00, 0x3A, 0x00, 0x63, 0x1D}, QStringLiteral("Bluetooth read-block"))) return false;
    if (!expect(readBlock(0x0000, 0x003A, 0x02), {0xAA, 0x06, 0x40, 0x00, 0x00, 0x3A, 0x00, 0x02, 0x7E}, QStringLiteral("USB read-block captured mode 0x02"))) return false;
    if (!expect(readBlock(0x03A0, 0x000B, 0x63), {0xAA, 0x06, 0x40, 0xA0, 0x03, 0x0B, 0x00, 0x63, 0xA9}, QStringLiteral("Bluetooth final read-block"))) return false;
    if (!expect(readBlock(0x03A0, 0x000B, 0x02), {0xAA, 0x06, 0x40, 0xA0, 0x03, 0x0B, 0x00, 0x02, 0x0A}, QStringLiteral("USB final read-block captured mode 0x02"))) return false;

    K500EqBand band;
    band.frequencyHz = 355.0;
    band.gainDb = -11.1;
    band.q = 1.0;
    if (!expect(eqWrite(QStringLiteral("music"), 2, band), {0xAA, 0x09, 0x03, 0x02, 0x02, 0x63, 0x01, 0x0A, 0x80, 0x6F, 0x60, 0x33}, QStringLiteral("music EQ"))) return false;
    if (!expect(eqWrite(QStringLiteral("micA"), 2, band), {0xAA, 0x09, 0x03, 0x00, 0x02, 0x63, 0x01, 0x0A, 0x80, 0x6F, 0x00, 0x95}, QStringLiteral("mic A EQ"))) return false;
    if (!expect(eqWrite(QStringLiteral("sub"), 2, band), {0xAA, 0x09, 0x03, 0x08, 0x02, 0x63, 0x01, 0x0A, 0x80, 0x6F, 0x00, 0x8D}, QStringLiteral("sub EQ"))) return false;
    if (!eqWrite(QStringLiteral("unknown"), 0, band).isEmpty()) return fail(QStringLiteral("unsupported EQ section must not produce a frame"));

    if (!expect(crossoverWrite(QStringLiteral("music"), QStringLiteral("hpf"), 95.0, QStringLiteral("HP Butter 12"), 0x32), {0xAA, 0x06, 0x11, 0x02, 0x02, 0x5F, 0x00, 0x32, 0x54}, QStringLiteral("music crossover"))) return false;
    if (!expect(crossoverWrite(QStringLiteral("mic"), QStringLiteral("hpf"), 1000.0, QStringLiteral("HP Butter 12")), {0xAA, 0x06, 0x11, 0x00, 0x02, 0xE8, 0x03, 0x00, 0xFC}, QStringLiteral("mic HPF selector"))) return false;
    if (!expect(crossoverWrite(QStringLiteral("main"), QStringLiteral("lpf"), 1000.0, QStringLiteral("LP Butter 12")), {0xAA, 0x06, 0x11, 0x05, 0x02, 0xE8, 0x03, 0x00, 0xF7}, QStringLiteral("main LPF selector"))) return false;
    if (!crossoverWrite(QStringLiteral("reverb"), QStringLiteral("hpf"), 1000.0, QStringLiteral("HP Butter 12")).isEmpty()) return fail(QStringLiteral("Reverb crossover must use captured CMD 0x0B block"));
    if (!expect(crossoverWrite(QStringLiteral("surround"), QStringLiteral("lpf"), 1000.0, QStringLiteral("LP Butter 12")), {0xAA, 0x06, 0x11, 0x09, 0x02, 0xE8, 0x03, 0x00, 0xF3}, QStringLiteral("surround LPF selector"))) return false;
    if (!crossoverWrite(QStringLiteral("echo"), QStringLiteral("hpf"), 1000.0, QStringLiteral("HP Butter 12")).isEmpty()) return fail(QStringLiteral("Echo crossover must use captured CMD 0x0D block"));
    if (!expect(crossoverWrite(QStringLiteral("center"), QStringLiteral("lpf"), 1000.0, QStringLiteral("LP Butter 12")), {0xAA, 0x06, 0x11, 0x0D, 0x02, 0xE8, 0x03, 0x00, 0xEF}, QStringLiteral("center LPF selector"))) return false;
    if (!expect(crossoverWrite(QStringLiteral("sub"), QStringLiteral("lpf"), 1000.0, QStringLiteral("LP Butter 12")), {0xAA, 0x06, 0x11, 0x0F, 0x02, 0xE8, 0x03, 0x00, 0xED}, QStringLiteral("sub LPF selector"))) return false;
    if (!expect(crossoverWrite(QStringLiteral("center"), QStringLiteral("lpf"), 1474.0, QStringLiteral("Bypass")), {0xAA, 0x06, 0x11, 0x0D, 0x00, 0xC2, 0x05, 0x00, 0x15}, QStringLiteral("center LPF bypass preserves cutoff"))) return false;
    if (!crossoverWrite(QStringLiteral("unknown"), QStringLiteral("hpf"), 1000.0, QStringLiteral("HP Butter 12")).isEmpty()) return fail(QStringLiteral("unsupported crossover section must not produce a frame"));

    K500MusicBlockState music;
    if (!expect(topMusicBlock(music, {}), {0xAA, 0x0D, 0x02, 0x23, 0x19, 0x54, 0x02, 0x09, 0x09, 0x09, 0x08, 0x08, 0x07, 0x00, 0x02, 0x2B}, QStringLiteral("top music default"))) return false;

    K500MusicBlockState gateCapture;
    gateCapture.topMusicVol = 25;
    gateCapture.musicInitVol = 25;
    gateCapture.sourceRaw = 2;
    gateCapture.input1GainDb = -3.0;
    gateCapture.input2GainDb = -3.0;
    gateCapture.bluetoothGainDb = -3.0;
    gateCapture.uDiskGainDb = -4.0;
    gateCapture.digitalGainDb = -4.0;
    gateCapture.key = 0;
    QByteArray gateScalars(0x40, char(0));
    gateScalars[0x03] = char(0x19);
    gateScalars[0x04] = char(0x54);
    gateScalars[0x07] = char(0x13);
    gateCapture.noiseGateRaw = musicNoiseGateRaw(NativeRange::MusicNoiseGateOffDb);
    if (!expect(topMusicBlock(gateCapture, gateScalars),
                {0xAA,0x0D,0x02,0x19,0x19,0x54,0x02,0x09,0x09,0x09,0x08,0x08,0x07,0x00,0x13,0x24},
                QStringLiteral("Music Noise Gate OFF capture"))) return false;
    gateCapture.noiseGateRaw = musicNoiseGateRaw(-90);
    if (!expect(topMusicBlock(gateCapture, gateScalars),
                {0xAA,0x0D,0x02,0x19,0x19,0x54,0x02,0x09,0x09,0x09,0x08,0x08,0x07,0x01,0x13,0x23},
                QStringLiteral("Music Noise Gate -90 capture"))) return false;
    gateCapture.noiseGateRaw = musicNoiseGateRaw(-50);
    if (!expect(topMusicBlock(gateCapture, gateScalars),
                {0xAA,0x0D,0x02,0x19,0x19,0x54,0x02,0x09,0x09,0x09,0x08,0x08,0x07,0x29,0x13,0xFB},
                QStringLiteral("Music Noise Gate -50 capture"))) return false;

    if (!expect(musicBass(-12.0), {0xAA,0x06,0x0C,0x02,0x00,0x00,0x00,0x09,0xE3},
                QStringLiteral("Music Bass -12 capture"))) return false;
    if (!expect(musicBass(0.0), {0xAA,0x06,0x0C,0x02,0x00,0x78,0x00,0x09,0x6B},
                QStringLiteral("Music Bass 0 capture"))) return false;
    if (!expect(musicBass(9.0), {0xAA,0x06,0x0C,0x02,0x00,0xD2,0x00,0x09,0x11},
                QStringLiteral("Music Bass +9 capture"))) return false;
    QByteArray scalars(0x40, char(0));
    scalars[0x03] = char(0x31); scalars[0x04] = char(0x52); scalars[0x1B] = char(0x0B); scalars[0x07] = char(0x06);
    music.topMusicVol = 70; music.sourceRaw = 4; music.input1GainDb = 3.0; music.input2GainDb = -1.0; music.bluetoothGainDb = 5.0; music.uDiskGainDb = -3.0; music.digitalGainDb = -4.0; music.key = 3;
    if (!expect(topMusicBlock(music, scalars), {0xAA, 0x0D, 0x02, 0x46, 0x31, 0x52, 0x04, 0x0F, 0x0B, 0x11, 0x09, 0x08, 0x0A, 0x0B, 0x06, 0xCD}, QStringLiteral("top music mirrored scalar"))) return false;

    K500MicBlockState mic;
    if (!expect(topMicBlock(mic, {}), {0xAA, 0x0E, 0x05, 0x23, 0x19, 0x54, 0x0B, 0x00, 0x00, 0x60, 0x60, 0x26, 0x03, 0x0A, 0x02, 0x00, 0x5D}, QStringLiteral("top mic default"))) return false;

    QByteArray micScalars(0x40, char(0));
    micScalars[0x0A] = char(0x19); micScalars[0x0B] = char(0x54); micScalars[0x0E] = char(0x0B);
    // Deliberately poison neighbour 0x1C: captured FBX write must never replay it.
    micScalars[0x1B] = char(0x03); micScalars[0x1C] = char(0x7F);
    mic.topMicVol = 30; mic.fbxLevel = 19; mic.micAVol = 100; mic.micBVol = 100;
    mic.compThresholdDb = 0; mic.compRatio = 2; mic.attackMs = 1; mic.releaseSec = 1.2;
    if (!expect(topMicBlock(mic, micScalars), {0xAA, 0x0E, 0x05, 0x1E, 0x19, 0x54, 0x0B, 0x04, 0x00, 0x64, 0x64, 0x32, 0x02, 0x01, 0x0C, 0x00, 0x4A}, QStringLiteral("top mic FBX clamps to captured level 4"))) return false;

    K500MicBlockState fbxCapture;
    fbxCapture.topMicVol = 25;
    fbxCapture.micInitVol = 25;
    fbxCapture.micAVol = 96;
    fbxCapture.micBVol = 96;
    fbxCapture.compThresholdDb = -11;
    fbxCapture.compRatio = 3;
    fbxCapture.attackMs = 10;
    fbxCapture.releaseSec = 0.2;
    QByteArray fbxScalars(0x40, char(0));
    fbxScalars[0x0A] = char(0x19);
    fbxScalars[0x0B] = char(0x54);
    fbxScalars[0x0E] = char(0x0B);
    const QList<QByteArray> expectedFbx{
        bytes({0xAA,0x0E,0x05,0x19,0x19,0x54,0x0B,0x00,0x00,0x60,0x60,0x27,0x03,0x0A,0x02,0x00,0x66}),
        bytes({0xAA,0x0E,0x05,0x19,0x19,0x54,0x0B,0x01,0x00,0x60,0x60,0x27,0x03,0x0A,0x02,0x00,0x65}),
        bytes({0xAA,0x0E,0x05,0x19,0x19,0x54,0x0B,0x02,0x00,0x60,0x60,0x27,0x03,0x0A,0x02,0x00,0x64}),
        bytes({0xAA,0x0E,0x05,0x19,0x19,0x54,0x0B,0x03,0x00,0x60,0x60,0x27,0x03,0x0A,0x02,0x00,0x63}),
        bytes({0xAA,0x0E,0x05,0x19,0x19,0x54,0x0B,0x04,0x00,0x60,0x60,0x27,0x03,0x0A,0x02,0x00,0x62}),
    };
    for (int level = 0; level <= 4; ++level) {
        fbxCapture.fbxLevel = level;
        if (topMicBlock(fbxCapture, fbxScalars) != expectedFbx.at(level))
            return fail(QStringLiteral("captured FBX level %1 frame mismatch").arg(level));
    }

    K500EffectBlockState effect;
    effect.topEffectVol = 49;
    if (!expect(topEffectBlock(effect, {}), {0xAA, 0x03, 0x09, 0x31, 0x19, 0xAA}, QStringLiteral("top effect"))) return false;

    QByteArray reverbSeed = bytes({0x5F,0x01,0x64,0x32,0x32,0x55,0xDC,0x00,0xB8,0x3D,0x90,0x06,0x2A,0x00,0x00});
    K500ReverbBlockState reverb;
    reverb.level = 99; reverb.direct = 100; reverb.hpfHz = 220; reverb.lpfHz = 15800; reverb.decayMs = 1680; reverb.predelayMs = 42;
    if (!expect(K500Frame::toUsbFrame(reverbBlock(reverb, reverbSeed)), {0xAA,0x10,0x00,0x0B,0x63,0x01,0x64,0x32,0x32,0x55,0xDC,0x00,0xB8,0x3D,0x90,0x06,0x2A,0x00,0x00,0xD3}, QStringLiteral("Reverb level 99 USB capture"))) return false;
    reverb.level = 95; reverb.direct = 99; reverb.decayMs = 1685; reverb.predelayMs = 50;
    if (!expect(K500Frame::toUsbFrame(reverbBlock(reverb, reverbSeed)), {0xAA,0x10,0x00,0x0B,0x5F,0x01,0x63,0x32,0x32,0x55,0xDC,0x00,0xB8,0x3D,0x95,0x06,0x32,0x00,0x00,0xCB}, QStringLiteral("Reverb direct 99 USB capture"))) return false;
    reverb.direct = 95; reverb.hpfHz = 221;
    if (!expect(K500Frame::toUsbFrame(reverbBlock(reverb, reverbSeed)), {0xAA,0x10,0x00,0x0B,0x5F,0x01,0x5F,0x32,0x32,0x55,0xDD,0x00,0xB8,0x3D,0x95,0x06,0x32,0x00,0x00,0xCE}, QStringLiteral("Reverb HPF 221 USB capture"))) return false;
    reverb.hpfHz = 225; reverb.lpfHz = 16000;
    if (!expect(K500Frame::toUsbFrame(reverbBlock(reverb, reverbSeed)), {0xAA,0x10,0x00,0x0B,0x5F,0x01,0x5F,0x32,0x32,0x55,0xE1,0x00,0x80,0x3E,0x95,0x06,0x32,0x00,0x00,0x01}, QStringLiteral("Reverb LPF 16000 USB capture"))) return false;
    QByteArray preserveReverb(ReverbDataLength, char(0x5A));
    const QByteArray preservedReverbFrame = reverbBlock(reverb, preserveReverb);
    if (preservedReverbFrame.size() < 19 || byteFromChar(preservedReverbFrame.at(4)) != 0x5A
        || byteFromChar(preservedReverbFrame.at(6)) != 0x5A || byteFromChar(preservedReverbFrame.at(17)) != 0x5A)
        return fail(QStringLiteral("Reverb block must preserve unknown device bytes"));

    // K500_NATIVE_VALUE_CONTRACT_V1 — transport clamping must be identical to
    // the manufacturer UI domain even when called programmatically.
    K500ReverbBlockState reverbLow = reverb;
    reverbLow.level = -99; reverbLow.direct = -99; reverbLow.hpfHz = -1;
    reverbLow.lpfHz = 1; reverbLow.decayMs = 1; reverbLow.predelayMs = -1;
    K500ReverbBlockState reverbMin = reverbLow;
    reverbMin.level = NativeRange::ReverbLevelMin;
    reverbMin.direct = NativeRange::ReverbDirectMin;
    reverbMin.hpfHz = NativeRange::FxHpfMinHz;
    reverbMin.lpfHz = NativeRange::FxLpfMinHz;
    reverbMin.decayMs = NativeRange::ReverbDecayMinMs;
    reverbMin.predelayMs = NativeRange::ReverbPredelayMinMs;
    if (reverbBlock(reverbLow, reverbSeed) != reverbBlock(reverbMin, reverbSeed))
        return fail(QStringLiteral("Reverb native minimum clamp mismatch"));

    K500ReverbBlockState reverbHigh = reverb;
    reverbHigh.level = 999; reverbHigh.direct = 999; reverbHigh.hpfHz = 99999;
    reverbHigh.lpfHz = 99999; reverbHigh.decayMs = 99999; reverbHigh.predelayMs = 99999;
    K500ReverbBlockState reverbMax = reverbHigh;
    reverbMax.level = NativeRange::ReverbLevelMax;
    reverbMax.direct = NativeRange::ReverbDirectMax;
    reverbMax.hpfHz = NativeRange::FxHpfMaxHz;
    reverbMax.lpfHz = NativeRange::FxLpfMaxHz;
    reverbMax.decayMs = NativeRange::ReverbDecayMaxMs;
    reverbMax.predelayMs = NativeRange::ReverbPredelayMaxMs;
    if (reverbBlock(reverbHigh, reverbSeed) != reverbBlock(reverbMax, reverbSeed))
        return fail(QStringLiteral("Reverb native maximum clamp mismatch"));

    QByteArray echoSeed = bytes({0x01,0x5A,0x02,0x64,0x02,0x40,0x64,0x3C,0x3C,0x26,0x02,0x68,0x10,0x2C,0x01,0x64,0x00,0xC8,0x00,0x00,0x00,0x00});
    K500EchoBlockState echo;
    echo.level = 90; echo.repeat = 2; echo.direct = 99; echo.rightDelayPercent = 10; echo.rightPredelayPercent = 10;
    echo.hpfHz = 550; echo.lpfHz = 4200; echo.leftDelayMs = 300; echo.leftPredelayMs = 100;
    if (!expect(K500Frame::toUsbFrame(echoBlock(echo, echoSeed)), {0xAA,0x17,0x00,0x0D,0x01,0x5A,0x02,0x64,0x02,0x40,0x63,0x3C,0x3C,0x26,0x02,0x68,0x10,0x2C,0x01,0x64,0x00,0xC8,0x00,0x00,0x00,0x00,0x05}, QStringLiteral("Echo direct 99 USB capture"))) return false;
    echo.repeat = 10; echo.direct = 90; echo.rightDelayPercent = 20; echo.rightPredelayPercent = -10;
    echo.hpfHz = 560; echo.lpfHz = 4210; echo.leftDelayMs = 310; echo.leftPredelayMs = 90;
    if (!expect(K500Frame::toUsbFrame(echoBlock(echo, echoSeed)), {0xAA,0x17,0x00,0x0D,0x01,0x5A,0x0A,0x64,0x02,0x40,0x5A,0x46,0x28,0x30,0x02,0x72,0x10,0x36,0x01,0x5A,0x00,0xC8,0x00,0x00,0x00,0x00,0xFC}, QStringLiteral("Echo full captured field map"))) return false;
    QByteArray preserveEcho(EchoDataLength, char(0x5A));
    const QByteArray preservedEchoFrame = echoBlock(echo, preserveEcho);
    if (preservedEchoFrame.size() < 26 || byteFromChar(preservedEchoFrame.at(4)) != 0x5A
        || byteFromChar(preservedEchoFrame.at(7)) != 0x5A || byteFromChar(preservedEchoFrame.at(21)) != 0x5A
        || byteFromChar(preservedEchoFrame.at(24)) != 0x5A)
        return fail(QStringLiteral("Echo block must preserve unknown device bytes"));

    K500EchoBlockState echoLow = echo;
    echoLow.level = -1; echoLow.repeat = -1; echoLow.direct = -1;
    echoLow.hpfHz = -1; echoLow.lpfHz = 1; echoLow.leftDelayMs = -1;
    K500EchoBlockState echoMin = echoLow;
    echoMin.level = NativeRange::EchoLevelMin;
    echoMin.repeat = NativeRange::EchoRepeatMin;
    echoMin.direct = NativeRange::EchoDirectMin;
    echoMin.hpfHz = NativeRange::FxHpfMinHz;
    echoMin.lpfHz = NativeRange::FxLpfMinHz;
    echoMin.leftDelayMs = NativeRange::EchoDelayMinMs;
    if (echoBlock(echoLow, echoSeed) != echoBlock(echoMin, echoSeed))
        return fail(QStringLiteral("Echo native minimum clamp mismatch"));

    K500EchoBlockState echoHigh = echo;
    echoHigh.level = 999; echoHigh.repeat = 999; echoHigh.direct = 999;
    echoHigh.hpfHz = 99999; echoHigh.lpfHz = 99999; echoHigh.leftDelayMs = 99999;
    K500EchoBlockState echoMax = echoHigh;
    echoMax.level = NativeRange::EchoLevelMax;
    echoMax.repeat = NativeRange::EchoRepeatMax;
    echoMax.direct = NativeRange::EchoDirectMax;
    echoMax.hpfHz = NativeRange::FxHpfMaxHz;
    echoMax.lpfHz = NativeRange::FxLpfMaxHz;
    echoMax.leftDelayMs = NativeRange::EchoDelayMaxMs;
    if (echoBlock(echoHigh, echoSeed) != echoBlock(echoMax, echoSeed))
        return fail(QStringLiteral("Echo native maximum clamp mismatch"));

    // EQ_BYPASS_24BIT_CAPTURED_V2 — sequential donor vectors prove one shared
    // 24-bit image across Mic/Music/Main/Surround/Center/Sub/Reverb/Echo.
    K500EqBypassImage bypass{0x1D, 0xAA, 0x00};
    if (!expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0x1D,0xAA,0x00,0x02,0xE2}, QStringLiteral("EQ bypass baseline USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("sub"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0x1D,0xEA,0x00,0x02,0xA2}, QStringLiteral("Sub EQ bypass ON USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("center"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0x1D,0xFA,0x00,0x02,0x92}, QStringLiteral("Center EQ bypass ON USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("surround"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0x1D,0xFE,0x00,0x02,0x8E}, QStringLiteral("Surround EQ bypass ON USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("main"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0x1D,0xFF,0x00,0x02,0x8D}, QStringLiteral("Main EQ bypass ON USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("mic"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0x7D,0xFF,0x00,0x02,0x2D}, QStringLiteral("Mic EQ bypass ON USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("music"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0xFD,0xFF,0x00,0x02,0xAD}, QStringLiteral("Music EQ bypass ON USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("reverb"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0xFD,0xFF,0x01,0x02,0xAC}, QStringLiteral("Reverb EQ bypass ON USB capture"))) return false;
    if (!setEqBypass(bypass, QStringLiteral("echo"), true) || !expect(K500Frame::toUsbFrame(eqBypassWrite(bypass)), {0xAA,0x06,0x00,0x0F,0x40,0xFD,0xFF,0x03,0x02,0xAA}, QStringLiteral("Echo EQ bypass ON USB capture"))) return false;
    if (!eqBypassEnabled(bypass, QStringLiteral("mic")) || !eqBypassEnabled(bypass, QStringLiteral("echo"))) return fail(QStringLiteral("EQ bypass decode mismatch"));

    if (!expect(micEqLink(false), {0xAA, 0x04, 0x3C, 0x00, 0x00, 0xC4, 0xFC}, QStringLiteral("mic EQ link off"))) return false;
    if (!expect(micEqLink(true), {0xAA, 0x04, 0x3C, 0x01, 0x01, 0x9E, 0x20}, QStringLiteral("mic EQ link on"))) return false;

    K500OutputBlockState main;
    main.lVolDb = 12; main.rVolDb = 10; main.micDirect = 91; main.musicLevel = 87; main.reverbLevel = 83; main.echoLevel = 79; main.compThresholdDb = -3; main.compRatio = 18; main.attackMs = 7; main.releaseSec = 0.1;
    if (!expect(outputBlock(QStringLiteral("main"), main, QByteArray(OutputDataLength, char(0))), {0xAA,0x25,0x0E,0x00,0x63,0x00,0x5F,0x00,0x5B,0x00,0x57,0x00,0x53,0x00,0x4F,0x00,0x2F,0x12,0x07,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6E}, QStringLiteral("main output block"))) return false;

    K500OutputBlockState surround;
    surround.lVolDb = 12; surround.rVolDb = 11; surround.micDirect = 87; surround.musicLevel = 85; surround.reverbLevel = 80; surround.echoLevel = 75; surround.compThresholdDb = -20; surround.compRatio = 100; surround.attackMs = 1; surround.releaseSec = 0.1; surround.lDelayMs = 3; surround.rDelayMs = 4;
    if (!expect(outputBlock(QStringLiteral("surround"), surround, QByteArray(OutputDataLength, char(0))), {0xAA,0x25,0x0E,0x02,0x63,0x00,0x61,0x00,0x57,0x00,0x55,0x00,0x50,0x00,0x4B,0x00,0x1E,0x64,0x01,0x01,0x03,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x35}, QStringLiteral("surround output block"))) return false;

    K500OutputBlockState center;
    center.outputVolDb = 12; center.micDirect = 88; center.musicLevel = 86; center.reverbLevel = 84; center.echoLevel = 82; center.compThresholdDb = -4; center.compRatio = 10; center.attackMs = 5; center.releaseSec = 0.2;
    if (!expect(outputBlock(QStringLiteral("center"), center, QByteArray(OutputDataLength, char(0))), {0xAA,0x25,0x0E,0x04,0x63,0x00,0x00,0x00,0x58,0x00,0x56,0x00,0x54,0x00,0x52,0x00,0x2E,0x0A,0x05,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xD3}, QStringLiteral("center output block"))) return false;

    K500OutputBlockState sub;
    sub.outputVolDb = 9; sub.micDirect = 70; sub.musicLevel = 90; sub.reverbLevel = 60; sub.echoLevel = 50; sub.compThresholdDb = -10; sub.compRatio = 8; sub.attackMs = 4; sub.releaseSec = 0.3;
    if (!expect(outputBlock(QStringLiteral("sub"), sub, QByteArray(OutputDataLength, char(0))), {0xAA,0x25,0x0E,0x05,0x5D,0x00,0x00,0x00,0x46,0x00,0x5A,0x00,0x3C,0x00,0x32,0x00,0x28,0x08,0x04,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x26}, QStringLiteral("sub output block"))) return false;

    QByteArray preserve(OutputDataLength, char(0x5A));
    const QByteArray preservedFrame = outputBlock(QStringLiteral("main"), main, preserve);
    if (preservedFrame.size() < 39 || byteFromChar(preservedFrame.at(21)) != 0x5A)
        return fail(QStringLiteral("output block must preserve unknown device bytes"));
    if (!outputBlock(QStringLiteral("unknown"), main, preserve).isEmpty())
        return fail(QStringLiteral("unsupported output section must not produce a frame"));

    return true;
}

} // namespace K500Protocol
