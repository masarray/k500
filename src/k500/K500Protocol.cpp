#include "K500Protocol.h"

#include "K500Frame.h"

#include <QHash>
#include <QtMath>

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

QByteArray expected(std::initializer_list<int> values)
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
    return K500Frame::build(QByteArray{char(0x01), char(0x1C)});
}

QByteArray handshake()
{
    return K500Frame::build(QByteArray{char(0x01), char(0x3F)});
}

QByteArray mute(bool enabled)
{
    return K500Frame::build(QByteArray{char(0x03), char(0x15), char(enabled ? 0x01 : 0x00), char(0x00)});
}

QByteArray playerCommand(const QString &command)
{
    int action = 0x02;
    if (command.compare(QStringLiteral("rewind"), Qt::CaseInsensitive) == 0)
        action = 0x00;
    else if (command.compare(QStringLiteral("forward"), Qt::CaseInsensitive) == 0)
        action = 0x01;
    return K500Frame::build(QByteArray{char(0x03), char(0x06), char(action), char(0x05)});
}

QByteArray readBlock(quint16 offset, quint16 length)
{
    QByteArray body;
    body.reserve(7);
    body.append(char(0x06));
    body.append(char(0x40));
    appendU16Le(body, offset);
    appendU16Le(body, length);
    body.append(char(0x63));
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
    const auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };

    if (heartbeat() != expected({0xAA, 0x01, 0x1C, 0xE3}))
        return fail(QStringLiteral("heartbeat frame mismatch"));
    if (K500Frame::toUsbFrame(heartbeat()) != expected({0xAA, 0x01, 0x00, 0x1C, 0xE3}))
        return fail(QStringLiteral("USB heartbeat framing mismatch"));

    K500EqBand band;
    band.frequencyHz = 355.0;
    band.gainDb = -11.1;
    band.q = 1.0;
    if (eqWrite(QStringLiteral("music"), 2, band)
        != expected({0xAA, 0x09, 0x03, 0x02, 0x02, 0x63, 0x01, 0x0A, 0x80, 0x6F, 0x60, 0x33}))
        return fail(QStringLiteral("music EQ frame mismatch"));

    if (crossoverWrite(QStringLiteral("music"), QStringLiteral("hpf"), 95.0,
                       QStringLiteral("HP Butter 12"), 0x32)
        != expected({0xAA, 0x06, 0x11, 0x02, 0x02, 0x5F, 0x00, 0x32, 0x54}))
        return fail(QStringLiteral("music crossover frame mismatch"));

    K500MusicBlockState music;
    if (topMusicBlock(music, {})
        != expected({0xAA, 0x0D, 0x02, 0x23, 0x19, 0x54, 0x02, 0x09, 0x09, 0x09,
                     0x08, 0x08, 0x07, 0x00, 0x02, 0x2B}))
        return fail(QStringLiteral("top music block mismatch"));

    return true;
}

} // namespace K500Protocol
