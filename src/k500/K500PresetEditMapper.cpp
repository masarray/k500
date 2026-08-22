#include "K500PresetEditMapper.h"

#include <QRegularExpression>
#include <QtMath>
#include <algorithm>

namespace K500PresetEditMapper {
namespace {
struct EqDescriptor {
    const char *key;
    int fileOffset;
    int bands;
    int hpfScalar;
    int lpfScalar;
};

// Primary editor sections only. *Alt blocks remain read-only until their
// runtime role is independently proven.
constexpr EqDescriptor PrimaryEq[] = {
    {"micA",     0x00F0, 10, 0x0098, 0x009A},
    {"micB",     0x0150, 10, 0x0098, 0x009A},
    {"music",    0x01B0,  7, 0x009C, 0x009E},
    {"main",     0x01F8,  7, 0x00A0, 0x00A4},
    {"surround", 0x0288,  5, 0x00A8, 0x00AC},
    {"center",   0x02F8,  5, 0x00B0, 0x00B4},
    {"sub",      0x0368,  5, 0x00B8, 0x00BC},
    {"reverb",   0x03D8,  5, 0x00C0, 0x00C2},
    {"echo",     0x0410,  5, 0x00C4, 0x00C6},
};

const EqDescriptor *eqDescriptor(const QString &key)
{
    for (const auto &d : PrimaryEq)
        if (key == QLatin1String(d.key)) return &d;
    return nullptr;
}

struct Builder {
    QVector<K500PresetCodec::BytePatch> patches;
    QSet<int> allowed;

    void addBytes(int offset, const QByteArray &bytes)
    {
        patches.push_back({offset, bytes});
        for (int i = 0; i < bytes.size(); ++i) allowed.insert(offset + i);
    }
    void addU8(int offset, int value)
    {
        addBytes(offset, QByteArray(1, static_cast<char>(std::clamp(value, 0, 255))));
    }
    void addU16(int offset, int value)
    {
        const quint16 v = static_cast<quint16>(std::clamp(value, 0, 65535));
        QByteArray bytes(2, char(0));
        bytes[0] = static_cast<char>(v & 0xFF);
        bytes[1] = static_cast<char>((v >> 8) & 0xFF);
        addBytes(offset, bytes);
    }
    void addI16(int offset, int value)
    {
        const qint16 signedValue = static_cast<qint16>(std::clamp(value, -32768, 32767));
        const quint16 v = static_cast<quint16>(signedValue);
        QByteArray bytes(2, char(0));
        bytes[0] = static_cast<char>(v & 0xFF);
        bytes[1] = static_cast<char>((v >> 8) & 0xFF);
        addBytes(offset, bytes);
    }
};

EditResult finish(const QByteArray &source, Builder &&builder)
{
    EditResult result;
    result.supported = true;
    result.patch = K500PresetCodec::applyWhitelistedPatches(
        source, builder.patches, builder.allowed, true);
    return result;
}

int u8Value(const QVariant &value, int bias = 0)
{
    return std::clamp(qRound(value.toDouble()) + bias, 0, 255);
}

int outputRaw(const QVariant &value)
{
    return std::clamp(qRound(value.toDouble() * 2.0 + 75.0), 0, 255);
}

quint16 peqTypeRaw(const QString &label, quint16 currentRaw, bool *ok)
{
    const QString upper = label.trimmed().toUpper();
    if (upper == QStringLiteral("BELL") || upper == QStringLiteral("P") || upper == QStringLiteral("PEAK")) {
        *ok = true;
        // Preserve observed P aliases 0x0000..0x0003 unless the user is
        // explicitly converting a shelf back to Bell.
        return currentRaw <= 0x0003 ? currentRaw : 0x0000;
    }
    if (upper == QStringLiteral("LOW SHELF") || upper == QStringLiteral("LS") || upper == QStringLiteral("LOWSHELF")) {
        *ok = true;
        return 0x0100;
    }
    if (upper == QStringLiteral("HIGH SHELF") || upper == QStringLiteral("HS") || upper == QStringLiteral("HIGHSHELF")) {
        *ok = true;
        return 0x0200;
    }
    *ok = false;
    return currentRaw;
}

quint16 filterTypeRaw(const QString &label, bool hpf, bool *ok)
{
    const QString upper = label.trimmed().toUpper();
    int code = 0;
    if (upper.contains(QStringLiteral("BESSEL")) && upper.contains(QStringLiteral("12"))) code = 0x01;
    else if (upper.contains(QStringLiteral("BUTTER")) && upper.contains(QStringLiteral("12"))) code = 0x02;
    else if (upper.contains(QStringLiteral("BESSEL")) && upper.contains(QStringLiteral("18"))) code = 0x03;
    else if (upper.contains(QStringLiteral("BUTTER")) && upper.contains(QStringLiteral("18"))) code = 0x04;
    else if (upper.contains(QStringLiteral("BESSEL")) && upper.contains(QStringLiteral("24"))) code = 0x05;
    else if (upper.contains(QStringLiteral("BUTTER")) && upper.contains(QStringLiteral("24"))) code = 0x06;
    else if (upper.contains(QStringLiteral("LR")) && upper.contains(QStringLiteral("24"))) code = 0x07;
    *ok = code != 0;
    return static_cast<quint16>((hpf ? 0x0400 : 0x0300) | code);
}

void addCrossoverFrequency(Builder &builder, const EqDescriptor &d, bool hpf, int hz)
{
    const int safeHz = std::clamp(hz, 20, 20000);
    builder.addU16(hpf ? d.hpfScalar : d.lpfScalar, safeHz);
    const int footer = d.fileOffset + 2 + d.bands * 8;
    builder.addU16(footer + (hpf ? 10 : 2), safeHz);
}

void addCrossoverType(Builder &builder, const EqDescriptor &d, bool hpf, quint16 raw)
{
    const int footer = d.fileOffset + 2 + d.bands * 8;
    builder.addU16(footer + (hpf ? 8 : 0), raw);
}

EditResult applyCrossover(const QByteArray &source, const QString &section,
                          const QString &field, const QVariant &value)
{
    QVector<const EqDescriptor *> targets;
    if (section == QStringLiteral("mic") || section == QStringLiteral("micA") || section == QStringLiteral("micB")) {
        targets << eqDescriptor(QStringLiteral("micA")) << eqDescriptor(QStringLiteral("micB"));
    } else if (const auto *d = eqDescriptor(section)) {
        targets << d;
    } else {
        return {};
    }

    Builder builder;
    const bool hpf = field == QStringLiteral("hpfHz") || field == QStringLiteral("hpType");
    if (field == QStringLiteral("hpfHz") || field == QStringLiteral("lpfHz")) {
        const int hz = qRound(value.toDouble());
        for (const auto *d : targets) addCrossoverFrequency(builder, *d, hpf, hz);
        return finish(source, std::move(builder));
    }
    if (field == QStringLiteral("hpType") || field == QStringLiteral("lpType")) {
        bool ok = false;
        const quint16 raw = filterTypeRaw(value.toString(), hpf, &ok);
        if (!ok) {
            EditResult r;
            r.supported = true;
            r.patch.error = QStringLiteral("Unknown verified crossover filter label: %1").arg(value.toString());
            return r;
        }
        for (const auto *d : targets) addCrossoverType(builder, *d, hpf, raw);
        return finish(source, std::move(builder));
    }
    return {};
}

EditResult applyScalar(const QByteArray &source, const QString &path, const QVariant &value)
{
    Builder b;
    if (path == QStringLiteral("system.topMusicVol")) b.addU8(0x0008, u8Value(value));
    else if (path == QStringLiteral("system.topMicVol")) b.addU8(0x0009, u8Value(value));
    else if (path == QStringLiteral("system.topEffectVol")) b.addU8(0x000A, u8Value(value));
    else if (path == QStringLiteral("system.musicInitVol")) b.addU8(0x000B, u8Value(value));
    else if (path == QStringLiteral("system.musicMaxVol")) b.addU8(0x000C, u8Value(value));
    else if (path == QStringLiteral("music.sourceRaw")) b.addU8(0x000E, u8Value(value));
    else if (path == QStringLiteral("music.key")) b.addU8(0x0011, u8Value(value, 7));
    else if (path == QStringLiteral("system.micInitVol")) b.addU8(0x0012, u8Value(value));
    else if (path == QStringLiteral("system.micMaxVol")) b.addU8(0x0013, u8Value(value));
    else if (path == QStringLiteral("mic.micAVol")) b.addU8(0x0014, u8Value(value));
    else if (path == QStringLiteral("mic.micBVol")) b.addU8(0x0015, u8Value(value));
    else if (path == QStringLiteral("mic.noiseGateDb")) b.addU8(0x0016, u8Value(value, 81));
    else if (path == QStringLiteral("mic.compThresholdDb")) b.addU8(0x0017, u8Value(value, 50));
    else if (path == QStringLiteral("mic.compRatio")) b.addU8(0x0018, u8Value(value));
    else if (path == QStringLiteral("mic.attackMs")) b.addU8(0x0019, u8Value(value));
    else if (path == QStringLiteral("mic.releaseSec")) b.addU8(0x001A, std::clamp(qRound(value.toDouble() * 10.0), 0, 255));
    else if (path == QStringLiteral("mic.fbxLevel")) {
        const int raw = std::clamp(qRound(value.toDouble()), 0, 20);
        b.addU8(0x001B, raw);
        b.addU8(0x001C, raw);
    }
    else if (path == QStringLiteral("system.effectInitLevel")) b.addU8(0x001D, u8Value(value));
    else if (path == QStringLiteral("music.input1GainDb")) b.addU8(0x001E, u8Value(value, 12));
    else if (path == QStringLiteral("music.input2GainDb")) b.addU8(0x001F, u8Value(value, 12));
    else if (path == QStringLiteral("music.bluetoothGainDb") || path == QStringLiteral("music.btGainDb")) b.addU8(0x0020, u8Value(value, 12));
    else if (path == QStringLiteral("music.uDiskGainDb")) b.addU8(0x0021, u8Value(value, 12));
    else if (path == QStringLiteral("music.digitalGainDb")) b.addU8(0x0022, u8Value(value, 12));
    else if (path == QStringLiteral("mic.eqLink")) b.addU8(0x0092, value.toBool() ? 1 : 0);
    else if (path == QStringLiteral("system.uDiskRecordVol")) b.addU8(0x0095, u8Value(value, -1));
    else if (path == QStringLiteral("system.usbRecordVol")) b.addU8(0x0096, u8Value(value, -1));
    else return {};
    return finish(source, std::move(b));
}

EditResult applyOutput(const QByteArray &source, const QString &section,
                       const QString &field, const QVariant &value)
{
    Builder b;
    int base = -1;
    bool stereo = false;
    if (section == QStringLiteral("main")) { base = 0x0024; stereo = true; }
    else if (section == QStringLiteral("surround")) { base = 0x0038; stereo = true; }
    else if (section == QStringLiteral("center")) base = 0x004C;
    else if (section == QStringLiteral("sub")) base = 0x0060;
    else return {};

    if (stereo && field == QStringLiteral("lVolDb")) b.addU8(base + 0x00, outputRaw(value));
    else if (stereo && field == QStringLiteral("rVolDb")) b.addU8(base + 0x02, outputRaw(value));
    else if (!stereo && field == QStringLiteral("outputVolDb")) b.addU8(base + 0x00, outputRaw(value));
    else if (field == QStringLiteral("micDirect")) b.addU8(base + 0x04, u8Value(value));
    else if (field == QStringLiteral("musicLevel")) b.addU8(base + 0x06, u8Value(value));
    else if (field == QStringLiteral("reverbLevel")) b.addU8(base + 0x08, u8Value(value));
    else if (field == QStringLiteral("echoLevel")) b.addU8(base + 0x0A, u8Value(value));
    else if (field == QStringLiteral("compThresholdDb")) b.addU8(base + 0x0C, u8Value(value, 50));
    else if (field == QStringLiteral("compRatio")) b.addU8(base + 0x0D, u8Value(value));
    else if (field == QStringLiteral("attackMs")) b.addU8(base + 0x0E, u8Value(value));
    else if (field == QStringLiteral("releaseSec")) b.addU8(base + 0x0F, std::clamp(qRound(value.toDouble() * 10.0), 0, 255));
    else if (section == QStringLiteral("surround") && field == QStringLiteral("lDelayMs")) b.addU16(0x00D8, qRound(value.toDouble()));
    else if (section == QStringLiteral("surround") && field == QStringLiteral("rDelayMs")) b.addU16(0x00DA, qRound(value.toDouble()));
    else return {};
    return finish(source, std::move(b));
}

EditResult applyEffect(const QByteArray &source, const QString &section,
                       const QString &field, const QVariant &value)
{
    Builder b;
    if (section == QStringLiteral("reverb")) {
        if (field == QStringLiteral("level")) b.addU8(0x0074, u8Value(value));
        else if (field == QStringLiteral("decayMs")) b.addU16(0x00C8, qRound(value.toDouble()));
        else if (field == QStringLiteral("predelayMs")) b.addU16(0x00CA, qRound(value.toDouble()));
        else return {};
    } else if (section == QStringLiteral("echo")) {
        if (field == QStringLiteral("level")) b.addU8(0x007B, u8Value(value));
        else if (field == QStringLiteral("repeat")) b.addU8(0x007C, u8Value(value));
        else if (field == QStringLiteral("leftDelayMs")) b.addU16(0x00CC, qRound(value.toDouble()));
        else return {};
    } else return {};
    return finish(source, std::move(b));
}
} // namespace

EditResult applyEngineEdit(const QByteArray &source,
                           const QString &path,
                           const QVariant &value)
{
    if (source.size() != K500PresetCodec::PresetFileLength) {
        EditResult r;
        r.supported = true;
        r.patch.error = QStringLiteral("Edit persistence requires a loaded 0x0478-byte .k500 source.");
        return r;
    }

    static const QRegularExpression bandRe(QStringLiteral(R"(^eq\.([^.]+)\.bands\.(\d+)$)"));
    static const QRegularExpression crossoverRe(QStringLiteral(R"(^eq\.([^.]+)\.crossover\.(hpfHz|lpfHz|hpType|lpType)$)"));
    static const QRegularExpression outputRe(QStringLiteral(R"(^outputs\.(main|surround|center|sub)\.([^.]+)$)"));
    static const QRegularExpression effectRe(QStringLiteral(R"(^effects\.(reverb|echo)\.([^.]+)$)"));

    if (const auto match = bandRe.match(path); match.hasMatch()) {
        const QString section = match.captured(1);
        const auto *d = eqDescriptor(section);
        if (!d) return {}; // includes intentionally forbidden *Alt blocks
        const int index = match.captured(2).toInt();
        if (index < 0 || index >= d->bands) {
            EditResult r; r.supported = true;
            r.patch.error = QStringLiteral("EQ band index out of range for %1: %2").arg(section).arg(index);
            return r;
        }
        const int offset = d->fileOffset + 2 + index * 8;
        const K500PresetCodec::Document doc(source);
        const QVariantMap map = value.toMap();
        const int frequency = std::clamp(qRound(map.value(QStringLiteral("frequency"), doc.u16(offset + 2)).toDouble()), 20, 20000);
        const int qRaw = std::clamp(qRound(map.value(QStringLiteral("q"), doc.u16(offset + 4) / 10.0).toDouble() * 10.0), 1, 300);
        const int gainRaw = std::clamp(qRound(map.value(QStringLiteral("gain"), doc.i16(offset + 6) / 10.0).toDouble() * 10.0), -240, 240);
        bool typeOk = false;
        const quint16 typeRaw = peqTypeRaw(map.value(QStringLiteral("type"), QStringLiteral("BELL")).toString(), doc.u16(offset), &typeOk);
        if (!typeOk) {
            EditResult r; r.supported = true;
            r.patch.error = QStringLiteral("Unknown verified PEQ type: %1").arg(map.value(QStringLiteral("type")).toString());
            return r;
        }
        Builder b;
        b.addU16(offset, typeRaw);
        b.addU16(offset + 2, frequency);
        b.addU16(offset + 4, qRaw);
        b.addI16(offset + 6, gainRaw);
        return finish(source, std::move(b));
    }

    if (const auto match = crossoverRe.match(path); match.hasMatch())
        return applyCrossover(source, match.captured(1), match.captured(2), value);

    if (path == QStringLiteral("mic.hpfHz")) return applyCrossover(source, QStringLiteral("mic"), QStringLiteral("hpfHz"), value);
    if (path == QStringLiteral("mic.lpfHz")) return applyCrossover(source, QStringLiteral("mic"), QStringLiteral("lpfHz"), value);
    if (path == QStringLiteral("outputs.sub.hpfHz")) return applyCrossover(source, QStringLiteral("sub"), QStringLiteral("hpfHz"), value);
    if (path == QStringLiteral("outputs.sub.lpfHz")) return applyCrossover(source, QStringLiteral("sub"), QStringLiteral("lpfHz"), value);
    if (path == QStringLiteral("effects.reverb.hpfHz")) return applyCrossover(source, QStringLiteral("reverb"), QStringLiteral("hpfHz"), value);
    if (path == QStringLiteral("effects.reverb.lpfHz")) return applyCrossover(source, QStringLiteral("reverb"), QStringLiteral("lpfHz"), value);
    if (path == QStringLiteral("effects.echo.hpfHz")) return applyCrossover(source, QStringLiteral("echo"), QStringLiteral("hpfHz"), value);
    if (path == QStringLiteral("effects.echo.lpfHz")) return applyCrossover(source, QStringLiteral("echo"), QStringLiteral("lpfHz"), value);

    if (const auto match = outputRe.match(path); match.hasMatch())
        return applyOutput(source, match.captured(1), match.captured(2), value);
    if (const auto match = effectRe.match(path); match.hasMatch())
        return applyEffect(source, match.captured(1), match.captured(2), value);

    return applyScalar(source, path, value);
}

} // namespace K500PresetEditMapper
