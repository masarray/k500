#include "K500PresetCodec.h"
#include "K500PresetEditMapper.h"

#include <QCoreApplication>
#include <QFile>
#include <QSet>
#include <QTextStream>
#include <QtEndian>

namespace {
int fail(const QString &message)
{
    QTextStream(stderr) << "P3.4 edit persistence failure: " << message << '\n';
    return 1;
}

quint8 u8(const QByteArray &bytes, int offset)
{
    return static_cast<quint8>(static_cast<unsigned char>(bytes.at(offset)));
}

quint16 u16(const QByteArray &bytes, int offset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(bytes.constData() + offset));
}

void putU16(QByteArray &bytes, int offset, quint16 value)
{
    qToLittleEndian<quint16>(value, reinterpret_cast<uchar *>(bytes.data() + offset));
}

QSet<int> changedSet(const K500PresetCodec::PatchResult &patch)
{
    return QSet<int>(patch.changedOffsets.cbegin(), patch.changedOffsets.cend());
}

bool onlyChanged(const K500PresetCodec::PatchResult &patch, const QSet<int> &expected)
{
    return changedSet(patch) == expected;
}

bool accepted(const K500PresetEditMapper::EditResult &edit)
{
    return edit.supported && edit.patch.ok
        && K500PresetCodec::validateChecksum(edit.patch.bytes);
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2)
        return fail(QStringLiteral("expected one donor .k500 fixture path"));

    QFile file(QString::fromLocal8Bit(argv[1]));
    if (!file.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("cannot open donor fixture"));
    const QByteArray source = file.readAll();
    if (!K500PresetCodec::validateChecksum(source))
        return fail(QStringLiteral("donor fixture checksum invalid"));

    // Scalar: exactly one data byte plus checksum.
    auto edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("system.topMusicVol"), 44);
    if (!accepted(edit) || u8(edit.patch.bytes, 0x0008) != 44
        || !onlyChanged(edit.patch, QSet<int>{0x0008, K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("topMusic whitelist/checksum mapping regressed"));

    // Shared FBX UI intentionally writes both independently stored raw bytes.
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("mic.fbxLevel"), 9);
    if (!accepted(edit) || u8(edit.patch.bytes, 0x001B) != 9 || u8(edit.patch.bytes, 0x001C) != 9
        || !onlyChanged(edit.patch, QSet<int>{0x001B, 0x001C, K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("FBX shared-edit whitelist mapping regressed"));

    // PEQ Bell aliases 0x0000..0x0003 must survive an ordinary band edit.
    QByteArray aliasSource = source;
    constexpr int micAFirstBand = 0x00F0 + 2;
    putU16(aliasSource, micAFirstBand, 0x0003);
    aliasSource = K500PresetCodec::updateChecksum(aliasSource);
    QVariantMap band;
    band.insert(QStringLiteral("frequency"), 137.0);
    band.insert(QStringLiteral("gain"), -2.3);
    band.insert(QStringLiteral("q"), 1.7);
    band.insert(QStringLiteral("type"), QStringLiteral("BELL"));
    edit = K500PresetEditMapper::applyEngineEdit(aliasSource, QStringLiteral("eq.micA.bands.0"), band);
    if (!accepted(edit) || u16(edit.patch.bytes, micAFirstBand) != 0x0003
        || u16(edit.patch.bytes, micAFirstBand + 2) != 137
        || u16(edit.patch.bytes, micAFirstBand + 4) != 17
        || static_cast<qint16>(u16(edit.patch.bytes, micAFirstBand + 6)) != -23)
        return fail(QStringLiteral("PEQ raw Bell alias preservation regressed"));
    if (changedSet(edit.patch).contains(micAFirstBand) || changedSet(edit.patch).contains(micAFirstBand + 1))
        return fail(QStringLiteral("PEQ alias bytes changed despite Bell->Bell edit"));

    // Music HPF must patch both proven scalar and section-footer mirror. The
    // real donor stores 20 Hz as 14 00; changing to 91 Hz (5B 00) therefore
    // changes only each low byte plus checksum. High bytes remaining 00 are
    // deliberately absent from changedOffsets although they are whitelisted.
    constexpr int musicFooter = 0x01B0 + 2 + 7 * 8;
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("eq.music.crossover.hpfHz"), 91);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x009C) != 91 || u16(edit.patch.bytes, musicFooter + 10) != 91)
        return fail(QStringLiteral("Music crossover scalar/footer mirror regressed"));
    if (!onlyChanged(edit.patch, QSet<int>{0x009C,musicFooter+10,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Music crossover changed unexpected bytes"));

    // Mic shared crossover must update one scalar plus both proven EQ footer copies.
    constexpr int micAFooter = 0x00F0 + 2 + 10 * 8;
    constexpr int micBFooter = 0x0150 + 2 + 10 * 8;
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("mic.lpfHz"), 15500);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x009A) != 15500
        || u16(edit.patch.bytes, micAFooter + 2) != 15500
        || u16(edit.patch.bytes, micBFooter + 2) != 15500)
        return fail(QStringLiteral("shared Mic crossover mirror regressed"));

    // Sub alias path must update scalar plus sub footer LP frequency.
    constexpr int subFooter = 0x0368 + 2 + 5 * 8;
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("outputs.sub.lpfHz"), 115);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00BC) != 115 || u16(edit.patch.bytes, subFooter + 2) != 115)
        return fail(QStringLiteral("Sub LPF alias persistence regressed"));

    // Output dB encoding: raw = round(db*2 + 75).
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("outputs.main.lVolDb"), 6.0);
    if (!accepted(edit) || u8(edit.patch.bytes, 0x0024) != 87
        || !onlyChanged(edit.patch, QSet<int>{0x0024,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Main L output dB persistence regressed"));

    // Verified file-only effect detail can persist even though native LIVE write stays unsupported.
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("effects.reverb.decayMs"), 1900);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00C8) != 1900)
        return fail(QStringLiteral("Reverb decay persistence regressed"));

    // Unverified/virtual controls remain non-destructive.
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("music.bassDb"), 3.0);
    if (edit.supported || edit.patch.ok || !edit.patch.bytes.isEmpty())
        return fail(QStringLiteral("unsupported path became destructive"));

    QTextStream(stdout) << "P3.4 donor edit persistence PASS\n";
    return 0;
}
