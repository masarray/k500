#include "K500FieldContract.h"
#include "K500PresetCodec.h"
#include "K500PresetEditMapper.h"

#include <QCoreApplication>
#include <QFile>
#include <QSet>
#include <QTextStream>
#include <QVector>
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

QSet<int> diffSet(const QByteArray &lhs, const QByteArray &rhs)
{
    QSet<int> changed;
    const int common = qMin(lhs.size(), rhs.size());
    for (int i = 0; i < common; ++i)
        if (lhs.at(i) != rhs.at(i)) changed.insert(i);
    for (int i = common; i < lhs.size(); ++i) changed.insert(i);
    for (int i = common; i < rhs.size(); ++i) changed.insert(i);
    return changed;
}

bool accepted(const K500PresetEditMapper::EditResult &edit)
{
    return edit.supported && edit.patch.ok
        && K500PresetCodec::validateChecksum(edit.patch.bytes);
}

bool rejected(const K500PresetEditMapper::EditResult &edit)
{
    return edit.supported && !edit.patch.ok
        && edit.patch.bytes.isEmpty() && !edit.patch.error.isEmpty();
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2 && argc != 7)
        return fail(QStringLiteral(
            "expected donor .k500 fixture path and optional FBE0..FBE4 golden fixture paths"));

    QFile file(QString::fromLocal8Bit(argv[1]));
    if (!file.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("cannot open donor fixture"));
    const QByteArray source = file.readAll();
    if (!K500PresetCodec::validateChecksum(source))
        return fail(QStringLiteral("donor fixture checksum invalid"));

    // CANONICAL_PRESET_FIELD_CONTRACT_V1 — keep scalar projection and
    // evidence-backed Mic/FBE semantics inside the existing fast persistence
    // regression instead of creating another CI/build target.
    using namespace K500FieldContract;
    using namespace K500FieldContract::ScalarGeometry;
    if (activeOffsetForFileScalar(0x0008) != 0x0000
        || activeOffsetForFileScalar(0x0096) != 0x008E
        || activeOffsetForFileScalar(0x0097) != InvalidOffset
        || activeOffsetForFileScalar(0x0098) != 0x008F
        || activeOffsetForFileScalar(0x00EF) != 0x00E6)
        return fail(QStringLiteral("canonical scalar split boundary regressed"));

    for (int active = 0; active < ActiveEndExclusive; ++active) {
        const int fileOffset = fileOffsetForActiveScalar(active);
        if (fileOffset == InvalidOffset || activeOffsetForFileScalar(fileOffset) != active)
            return fail(QStringLiteral("canonical scalar mapping is not bijective"));
    }

    QSet<int> contractFiles;
    QSet<int> contractActive;
    for (const auto &field : EvidenceBackedScalars) {
        if (!geometryMatches(field) || field.rawMin > field.rawMax)
            return fail(QStringLiteral("evidence-backed field contract is inconsistent"));
        if (contractFiles.contains(field.fileOffset) || contractActive.contains(field.activeOffset))
            return fail(QStringLiteral("duplicate evidence-backed field mapping"));
        contractFiles.insert(field.fileOffset);
        contractActive.insert(field.activeOffset);
    }

    if (Field::MicFbe.fileOffset != 0x0023
        || Field::MicFbe.activeOffset != 0x001B
        || Field::MicFbe.rawMin != 0 || Field::MicFbe.rawMax != 4
        || Field::MicFbe.evidence != EvidenceLevel::ProvenRoundTrip
        || Field::MicFbe.fileMutation != FileMutationPolicy::Writable)
        return fail(QStringLiteral("physical FBE contract regressed"));

    if (Field::AdjMannerVrOff.fileOffset != 0x0094
        || Field::AdjMannerVrOff.activeOffset != 0x008C
        || Field::AdjMannerVrOff.rawMin != 0 || Field::AdjMannerVrOff.rawMax != 1
        || Field::AdjMannerVrOff.evidence != EvidenceLevel::ProvenRoundTrip
        || Field::AdjMannerVrOff.fileMutation != FileMutationPolicy::Writable)
        return fail(QStringLiteral("physical Adj Manner VR OFF contract regressed"));

    if (Field::MicHpType.fileOffset != 0x001B
        || Field::MicLpType.fileOffset != 0x001C
        || Field::MicHpType.fileMutation != FileMutationPolicy::PreserveOnly
        || Field::MicLpType.fileMutation != FileMutationPolicy::PreserveOnly)
        return fail(QStringLiteral("Mic HP/LP preservation contract regressed"));

    QVector<QByteArray> fbeGolden;
    if (argc == 7) {
        fbeGolden.reserve(5);
        for (int level = 0; level <= 4; ++level) {
            QFile goldenFile(QString::fromLocal8Bit(argv[level + 2]));
            if (!goldenFile.open(QIODevice::ReadOnly))
                return fail(QStringLiteral("cannot open FBE%1 golden fixture").arg(level));
            const QByteArray bytes = goldenFile.readAll();
            if (bytes.size() != K500PresetCodec::PresetFileLength
                || !K500PresetCodec::validateChecksum(bytes))
                return fail(QStringLiteral("FBE%1 golden fixture invalid").arg(level));
            fbeGolden.push_back(bytes);
        }
    }

    // Scalar: exactly one data byte plus checksum.
    auto edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("system.topMusicVol"), 44);
    if (!accepted(edit) || u8(edit.patch.bytes, 0x0008) != 44
        || !onlyChanged(edit.patch, QSet<int>{0x0008, K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("topMusic whitelist/checksum mapping regressed"));

    // ADJ_MANNER_VR_OFF_FILE_EDIT_V1 — paired reconnect captures prove the
    // .k500 scalar exactly at 0x0094. Donor is ON (1); edit OFF must touch only
    // that byte plus the additive checksum.
    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("system.adjMannerVrOff"), false);
    if (!accepted(edit) || u8(edit.patch.bytes, 0x0094) != 0
        || !onlyChanged(edit.patch, QSet<int>{0x0094, K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Adj Manner VR OFF file mapping regressed"));

    // FBE_FILE_MAPPING_CAPTURED_20261006_V1 — controlled FBE0..FBE4 native
    // exports prove file[0x0023] is the sole FBE/FBX scalar in the 0..4
    // native domain. file[0x001B]/[0x001C] remain Mic HP/LP type bytes.
    const quint8 originalMicHpType = u8(source, 0x001B);
    const quint8 originalMicLpType = u8(source, 0x001C);
    const int targetFbe = u8(source, 0x0023) == 4 ? 0 : 4;
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("mic.fbxLevel"), targetFbe);
    if (!accepted(edit) || u8(edit.patch.bytes, 0x0023) != targetFbe
        || u8(edit.patch.bytes, 0x001B) != originalMicHpType
        || u8(edit.patch.bytes, 0x001C) != originalMicLpType
        || !onlyChanged(edit.patch, QSet<int>{0x0023, K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("FBE file mapping/whitelist regressed"));

    QByteArray fbeRangeSource = source;
    fbeRangeSource[0x0023] = char(2);
    fbeRangeSource = K500PresetCodec::updateChecksum(fbeRangeSource);
    edit = K500PresetEditMapper::applyEngineEdit(
        fbeRangeSource, QStringLiteral("mic.fbxLevel"), 99);
    if (!rejected(edit))
        return fail(QStringLiteral("FBE out-of-range maximum edit was not rejected"));
    edit = K500PresetEditMapper::applyEngineEdit(
        fbeRangeSource, QStringLiteral("mic.fbxLevel"), -99);
    if (!rejected(edit))
        return fail(QStringLiteral("FBE out-of-range minimum edit was not rejected"));

    if (!fbeGolden.isEmpty()) {
        const QByteArray &baseline = fbeGolden.at(0);
        const quint8 capturedMicHpType = u8(baseline, 0x001B);
        const quint8 capturedMicLpType = u8(baseline, 0x001C);
        for (int level = 0; level <= 4; ++level) {
            const QByteArray &fixture = fbeGolden.at(level);
            if (u8(fixture, 0x0023) != level)
                return fail(QStringLiteral("FBE%1 fixture scalar is not level %1").arg(level));
            if (u8(fixture, 0x001B) != capturedMicHpType
                || u8(fixture, 0x001C) != capturedMicLpType)
                return fail(QStringLiteral("FBE%1 fixture changed Mic HP/LP type bytes").arg(level));
            if (level > 0
                && diffSet(fbeGolden.at(level - 1), fixture)
                    != QSet<int>{0x0023, K500PresetCodec::ChecksumOffset})
                return fail(QStringLiteral("FBE%1 pairwise golden diff is not scalar+checksum only").arg(level));

            const auto goldenEdit = K500PresetEditMapper::applyEngineEdit(
                baseline, QStringLiteral("mic.fbxLevel"), level);
            if (!accepted(goldenEdit) || goldenEdit.patch.bytes != fixture)
                return fail(QStringLiteral(
                    "FBE%1 writer output is not byte-identical to physical golden capture").arg(level));
        }
    }

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

    // OUTPUT_DELAY_CMD0E_CAPTURED_V1 — preset persistence keeps semantic
    // channel order even though Surround's native CMD 0x0E wire order is reversed.
    QByteArray delaySource = source;
    for (const int offset : {0x00D4,0x00D5,0x00D6,0x00D7,0x00D8,0x00D9,
                             0x00DA,0x00DB,0x00DC,0x00DD,0x00DE,0x00DF})
        delaySource[offset] = char(0);
    delaySource = K500PresetCodec::updateChecksum(delaySource);

    edit = K500PresetEditMapper::applyEngineEdit(delaySource, QStringLiteral("outputs.main.lDelayMs"), 20);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00D4) != 20
        || !onlyChanged(edit.patch, QSet<int>{0x00D4,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Main L delay persistence regressed"));

    edit = K500PresetEditMapper::applyEngineEdit(delaySource, QStringLiteral("outputs.main.rDelayMs"), 50);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00D6) != 50
        || !onlyChanged(edit.patch, QSet<int>{0x00D6,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Main R delay persistence regressed"));

    edit = K500PresetEditMapper::applyEngineEdit(delaySource, QStringLiteral("outputs.surround.lDelayMs"), 20);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00D8) != 20
        || !onlyChanged(edit.patch, QSet<int>{0x00D8,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Surround L semantic delay persistence regressed"));

    edit = K500PresetEditMapper::applyEngineEdit(delaySource, QStringLiteral("outputs.surround.rDelayMs"), 14);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00DA) != 14
        || !onlyChanged(edit.patch, QSet<int>{0x00DA,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Surround R semantic delay persistence regressed"));

    edit = K500PresetEditMapper::applyEngineEdit(delaySource, QStringLiteral("outputs.center.outputDelayMs"), 30);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00DC) != 30
        || !onlyChanged(edit.patch, QSet<int>{0x00DC,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Center output delay persistence regressed"));

    edit = K500PresetEditMapper::applyEngineEdit(delaySource, QStringLiteral("outputs.sub.outputDelayMs"), 40);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00DE) != 40
        || !onlyChanged(edit.patch, QSet<int>{0x00DE,K500PresetCodec::ChecksumOffset}))
        return fail(QStringLiteral("Subwoofer output delay persistence regressed"));

    // Programmatic persistence must reject out-of-domain writes rather than
    // silently changing the requested semantic value.
    edit = K500PresetEditMapper::applyEngineEdit(
        delaySource, QStringLiteral("outputs.main.lDelayMs"), 999);
    if (!rejected(edit))
        return fail(QStringLiteral("Output delay out-of-range edit was not rejected"));

    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("system.musicInitVol"), 85);
    if (!rejected(edit))
        return fail(QStringLiteral("Music Init out-of-range edit was not rejected"));

    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("system.usbRecordVol"), 0);
    if (!rejected(edit))
        return fail(QStringLiteral("USB Record out-of-range edit was not rejected"));

    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("effects.reverb.decayMs"), 499);
    if (!rejected(edit))
        return fail(QStringLiteral("Reverb decay out-of-range edit was not rejected"));

    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("effects.echo.repeat"), 11);
    if (!rejected(edit))
        return fail(QStringLiteral("Echo repeat out-of-range edit was not rejected"));

    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("effects.reverb.hpfHz"), 1001);
    if (!rejected(edit))
        return fail(QStringLiteral("Reverb HPF out-of-range edit was not rejected"));

    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("effects.echo.lpfHz"), 3999);
    if (!rejected(edit))
        return fail(QStringLiteral("Echo LPF out-of-range edit was not rejected"));

    // Valid proven-range values still persist exactly.
    edit = K500PresetEditMapper::applyEngineEdit(
        source, QStringLiteral("effects.reverb.decayMs"), 1900);
    if (!accepted(edit) || u16(edit.patch.bytes, 0x00C8) != 1900)
        return fail(QStringLiteral("Reverb decay persistence regressed"));

    // Unverified/virtual controls remain non-destructive.
    edit = K500PresetEditMapper::applyEngineEdit(source, QStringLiteral("music.bassDb"), 3.0);
    if (edit.supported || edit.patch.ok || !edit.patch.bytes.isEmpty())
        return fail(QStringLiteral("unsupported path became destructive"));

    QTextStream(stdout) << "P3.4 donor edit persistence PASS\n";
    return 0;
}
