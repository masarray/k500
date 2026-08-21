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

int crossoverSelector(const QString &section, const QString &kind)
{
    static const QHash<QString, int> hpf{
        {QStringLiteral("mic"), 0x00}, {QStringLiteral("micA"), 0x00}, {QStringLiteral("micB"), 0x00},
        {QStringLiteral("music"), 0x02}, {QStringLiteral("main"), 0x04}, {QStringLiteral("reverb"), 0x06},
        {QStringLiteral("surround"), 0x08}, {QStringLiteral("echo"), 0x0A}, {QStringLiteral("center"), 0x0C},
        {QStringLiteral("sub"), 0x0E},
    };
    static const QHash<QString, int> lpf{
        {QStringLiteral("mic"), 0x01}, {QStringLiteral("micA"), 0x01}, {QStringLiteral("micB"), 0x01},
        {QStringLiteral("music"), 0x03}, {QStringLiteral("main"), 0x05}, {QStringLiteral("reverb"), 0x07},
        {QStringLiteral("surround"), 0x09}, {QStringLiteral("echo"), 0x0B}, {QStringLiteral("center"), 0x0D},
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
    const double gain = qBound(-24.0, band.gainDb, 24.0);
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
    body.append(char(K500Frame::clampByte(qBound(0, state.sourceRaw, 4))));
    body.append(char(K500Frame::clampByte(qRound(state.input1GainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.input2GainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.bluetoothGainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.uDiskGainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qRound(state.digitalGainDb + 12.0))));
    body.append(char(K500Frame::clampByte(qBound(-7, state.key, 7) + 7)));
    body.append(char(mirrored(0x1B, 0x00)));
    body.append(char(mirrored(0x07, 0x02)));
    return K500Frame::build(body);
}

bool selfTest(QString *error)
{
    // P0_PROTOCOL_GOLDEN_VECTORS_V1
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

    if (!expect(heartbeat(), {0xAA, 0x01, 0x1C, 0xE3}, QStringLiteral("heartbeat")))
        return false;
    if (!expect(K500Frame::toUsbFrame(heartbeat()),
                {0xAA, 0x01, 0x00, 0x1C, 0xE3}, QStringLiteral("USB heartbeat")))
        return false;
    if (!expect(handshake(), {0xAA, 0x01, 0x3F, 0xC0}, QStringLiteral("handshake")))
        return false;

    if (!expect(mute(false), {0xAA, 0x03, 0x15, 0x00, 0x00, 0xE8}, QStringLiteral("mute off")))
        return false;
    if (!expect(mute(true), {0xAA, 0x03, 0x15, 0x01, 0x00, 0xE7}, QStringLiteral("mute on")))
        return false;
    if (!expect(playerCommand(QStringLiteral("rewind")),
                {0xAA, 0x03, 0x06, 0x00, 0x05, 0xF2}, QStringLiteral("rewind")))
        return false;
    if (!expect(playerCommand(QStringLiteral("forward")),
                {0xAA, 0x03, 0x06, 0x01, 0x05, 0xF1}, QStringLiteral("forward")))
        return false;
    if (!expect(playerCommand(QStringLiteral("playPause")),
                {0xAA, 0x03, 0x06, 0x02, 0x05, 0xF0}, QStringLiteral("play/pause")))
        return false;

    if (!expect(readBlock(0x0000, 0x003A, 0x63),
                {0xAA, 0x06, 0x40, 0x00, 0x00, 0x3A, 0x00, 0x63, 0x1D},
                QStringLiteral("Bluetooth read-block")))
        return false;
    if (!expect(readBlock(0x0000, 0x003A, 0x00),
                {0xAA, 0x06, 0x40, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x80},
                QStringLiteral("USB read-block")))
        return false;
    // 0x03AB bytes = sixteen 0x3A-byte reads followed by 0x0B bytes at 0x03A0.
    if (!expect(readBlock(0x03A0, 0x000B, 0x63),
                {0xAA, 0x06, 0x40, 0xA0, 0x03, 0x0B, 0x00, 0x63, 0xA9},
                QStringLiteral("Bluetooth final read-block")))
        return false;
    if (!expect(readBlock(0x03A0, 0x000B, 0x00),
                {0xAA, 0x06, 0x40, 0xA0, 0x03, 0x0B, 0x00, 0x00, 0x0C},
                QStringLiteral("USB final read-block")))
        return false;

    K500EqBand band;
    band.frequencyHz = 355.0;
    band.gainDb = -11.1;
    band.q = 1.0;
    if (!expect(eqWrite(QStringLiteral("music"), 2, band),
                {0xAA, 0x09, 0x03, 0x02, 0x02, 0x63, 0x01, 0x0A, 0x80, 0x6F, 0x60, 0x33},
                QStringLiteral("music EQ")))
        return false;
    if (!expect(eqWrite(QStringLiteral("micA"), 2, band),
                {0xAA, 0x09, 0x03, 0x00, 0x02, 0x63, 0x01, 0x0A, 0x80, 0x6F, 0x00, 0x95},
                QStringLiteral("mic A EQ")))
        return false;
    if (!expect(eqWrite(QStringLiteral("sub"), 2, band),
                {0xAA, 0x09, 0x03, 0x08, 0x02, 0x63, 0x01, 0x0A, 0x80, 0x6F, 0x00, 0x8D},
                QStringLiteral("sub EQ")))
        return false;
    if (!eqWrite(QStringLiteral("unknown"), 0, band).isEmpty())
        return fail(QStringLiteral("unsupported EQ section must not produce a frame"));

    if (!expect(crossoverWrite(QStringLiteral("music"), QStringLiteral("hpf"), 95.0,
                               QStringLiteral("HP Butter 12"), 0x32),
                {0xAA, 0x06, 0x11, 0x02, 0x02, 0x5F, 0x00, 0x32, 0x54},
                QStringLiteral("music crossover")))
        return false;
    // Directly captured selector values from the donor protocol documentation.
    if (!expect(crossoverWrite(QStringLiteral("main"), QStringLiteral("lpf"), 1000.0,
                               QStringLiteral("LP Butter 12")),
                {0xAA, 0x06, 0x11, 0x05, 0x02, 0xE8, 0x03, 0x00, 0xF7},
                QStringLiteral("main LPF selector")))
        return false;
    if (!expect(crossoverWrite(QStringLiteral("surround"), QStringLiteral("lpf"), 1000.0,
                               QStringLiteral("LP Butter 12")),
                {0xAA, 0x06, 0x11, 0x09, 0x02, 0xE8, 0x03, 0x00, 0xF3},
                QStringLiteral("surround LPF selector")))
        return false;
    if (!expect(crossoverWrite(QStringLiteral("center"), QStringLiteral("lpf"), 1000.0,
                               QStringLiteral("LP Butter 12")),
                {0xAA, 0x06, 0x11, 0x0D, 0x02, 0xE8, 0x03, 0x00, 0xEF},
                QStringLiteral("center LPF selector")))
        return false;
    if (!expect(crossoverWrite(QStringLiteral("sub"), QStringLiteral("lpf"), 1000.0,
                               QStringLiteral("LP Butter 12")),
                {0xAA, 0x06, 0x11, 0x0F, 0x02, 0xE8, 0x03, 0x00, 0xED},
                QStringLiteral("sub LPF selector")))
        return false;
    if (!crossoverWrite(QStringLiteral("unknown"), QStringLiteral("hpf"), 1000.0,
                        QStringLiteral("HP Butter 12")).isEmpty())
        return fail(QStringLiteral("unsupported crossover section must not produce a frame"));

    K500MusicBlockState music;
    if (!expect(topMusicBlock(music, {}),
                {0xAA, 0x0D, 0x02, 0x23, 0x19, 0x54, 0x02, 0x09, 0x09, 0x09,
                 0x08, 0x08, 0x07, 0x00, 0x02, 0x2B},
                QStringLiteral("top music default")))
        return false;

    // The block must preserve device-mirrored neighbours instead of sending stale UI defaults.
    QByteArray scalars(0x40, char(0));
    scalars[0x03] = char(0x31); // music init
    scalars[0x04] = char(0x52); // music max
    scalars[0x1B] = char(0x0B); // gate/state neighbour
    scalars[0x07] = char(0x06); // device filter/type neighbour
    music.topMusicVol = 70;
    music.sourceRaw = 4;
    music.input1GainDb = 3.0;
    music.input2GainDb = -1.0;
    music.bluetoothGainDb = 5.0;
    music.uDiskGainDb = -3.0;
    music.digitalGainDb = -4.0;
    music.key = 3;
    if (!expect(topMusicBlock(music, scalars),
                {0xAA, 0x0D, 0x02, 0x46, 0x31, 0x52, 0x04, 0x0F, 0x0B, 0x11,
                 0x09, 0x08, 0x0A, 0x0B, 0x06, 0xCD},
                QStringLiteral("top music mirrored scalar")))
        return false;

    return true;
}

} // namespace K500Protocol
