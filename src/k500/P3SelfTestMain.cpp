#include "K500PresetCodec.h"

#include <QCoreApplication>
#include <QDebug>

using namespace K500PresetCodec;

namespace {
bool expect(bool condition, const char *message)
{
    if (!condition) qCritical() << "P3 FAIL:" << message;
    return condition;
}

void putU16(QByteArray &b, int off, quint16 v)
{
    b[off] = char(v & 0xff);
    b[off + 1] = char((v >> 8) & 0xff);
}

struct EqFixture {
    int fileOffset;
    int bands;
};

constexpr EqFixture EqFixtures[] = {
    {0x00f0, 10}, {0x0150, 10}, {0x01b0, 7}, {0x01f8, 7},
    {0x0240, 7}, {0x0288, 5}, {0x02c0, 5}, {0x02f8, 5},
    {0x0330, 5}, {0x0368, 5}, {0x03a0, 5}, {0x03d8, 5},
    {0x0410, 5},
};

void seedRepresentableEq(QByteArray &bytes)
{
    int bandOrdinal = 0;
    for (const auto &section : EqFixtures) {
        for (int i = 0; i < section.bands; ++i, ++bandOrdinal) {
            const int off = section.fileOffset + 2 + i * 8;
            putU16(bytes, off, 0x0000);
            putU16(bytes, off + 2, static_cast<quint16>(100 + (bandOrdinal * 173) % 18000));
            putU16(bytes, off + 4, 10);
            putU16(bytes, off + 6, 0);
        }
    }
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    QByteArray source(PresetFileLength, char(0));
    for (int i = 0; i < source.size(); ++i)
        source[i] = char((i * 73 + 19) & 0xff);

    const QByteArray visibleName("P3 BIT PERFECT");
    source.replace(NameOffset, NameLength, QByteArray(NameLength, char(0)));
    source.replace(NameOffset, visibleName.size(), visibleName);

    source[K500FieldContract::Field::MicHpType.fileOffset] = char(7);
    source[K500FieldContract::Field::MicLpType.fileOffset] = char(7);
    source[K500FieldContract::Field::MicFbe.fileOffset] = char(2);
    source[K500FieldContract::Field::AdjMannerVrOff.fileOffset] = char(1);
    seedRepresentableEq(source);

    // Seed the first Mic A EQ record with values that verify alias/sign handling.
    putU16(source, 0x00f2, 0x0003); // P alias must remain raw 0x0003 in .k500
    putU16(source, 0x00f4, 1234);
    putU16(source, 0x00f6, 17);
    putU16(source, 0x00f8, quint16(qint16(-45)));
    source = updateChecksum(source);

    Document doc(source);
    ok &= expect(doc.validSize(), "valid 0x0478 size rejected");
    ok &= expect(doc.checksumOk(), "checksum should validate");
    ok &= expect(doc.serializeNoop() == source, "no-op parse/serialize changed bytes");
    ok &= expect(doc.name().startsWith(QStringLiteral("P3 BIT PERFECT")), "name parser mismatch");

    const auto sections = doc.eqSections();
    ok &= expect(sections.size() == 13, "expected all 13 known EQ sections");
    if (!sections.isEmpty()) {
        ok &= expect(sections[0].key == QStringLiteral("micA"), "first EQ section should be micA");
        ok &= expect(sections[0].bands.size() == 10, "Mic A should contain 10 bands");
        ok &= expect(sections[0].bands[0].typeRaw == 0x0003, "P alias raw value was normalized");
        ok &= expect(sections[0].bands[0].gainRaw == -45, "signed EQ gain parse mismatch");
    }

    // Explicit whitelist patch: only byte 0x0008 plus checksum may change.
    const QByteArray beforeUnknown = source.mid(0x0240, 0x48); // mainAlt block
    const auto patch = applyWhitelistedPatches(source,
                                               {BytePatch{0x0008, QByteArray(1, char(88))}},
                                               QSet<int>{0x0008});
    ok &= expect(patch.ok, "whitelisted patch rejected");
    ok &= expect(validateChecksum(patch.bytes), "patched checksum invalid");
    ok &= expect(patch.bytes.mid(0x0240, 0x48) == beforeUnknown, "unknown/Alt bytes changed unexpectedly");
    ok &= expect(patch.changedOffsets.contains(0x0008), "intended scalar was not changed");
    ok &= expect(patch.changedOffsets.size() <= 2, "patch changed bytes outside scalar+checksum");

    const auto denied = applyWhitelistedPatches(source,
                                                {BytePatch{0x0010, QByteArray(1, char(1))}},
                                                QSet<int>{0x0008});
    ok &= expect(!denied.ok, "non-whitelisted byte patch was accepted");

    QString slotError;
    ok &= expect(validateDeviceSlotCompatibility(source, &slotError),
                 "representable preset failed strict compatibility validation");
    ok &= expect(slotError.isEmpty(), "strict compatibility validation returned an error");
    const QByteArray slot = buildDeviceSlotImage(source, &slotError);
    ok &= expect(slotError.isEmpty(), "slot conversion reported an error");
    ok &= expect(slot.size() == DeviceSlotImageLength, "slot image is not 0x0290 bytes");
    if (slot.size() == DeviceSlotImageLength) {
        ok &= expect(quint8(slot[0]) == quint8(source[0x0008]), "low scalar +8 mapping mismatch");
        ok &= expect(quint8(slot[0x008e]) == quint8(source[0x0096]), "last low scalar mapping mismatch");
        ok &= expect(quint8(slot[0x008f]) == quint8(source[0x0098]), "high scalar +9 mapping mismatch");

        const int eq = 0x00e7;
        ok &= expect(quint8(slot[eq]) == (1234 & 0xff), "compact EQ frequency low byte mismatch");
        ok &= expect(quint8(slot[eq + 1]) == ((1234 >> 8) & 0xff), "compact EQ frequency high byte mismatch");
        ok &= expect(quint8(slot[eq + 2]) == 17, "compact EQ Q mismatch");
        ok &= expect((quint8(slot[eq + 3]) & 0x80) != 0, "compact EQ negative sign missing");
        ok &= expect((quint8(slot[eq + 3]) & 0x70) == 0, "P alias must compact to bell type");
        ok &= expect(quint8(slot[eq + 4]) == 45, "compact EQ gain magnitude mismatch");
        ok &= expect(slot.mid(0x027c, 4) == source.mid(0x044c, 4), "slot tail mapping mismatch");
        ok &= expect(slot.mid(0x0280, 0x10) == source.mid(NameOffset, 0x10), "slot name mapping mismatch");
        ok &= expect(slot != source.left(DeviceSlotImageLength), "converter accidentally degraded to raw file slicing");
        QString verifyError;
        ok &= expect(verifyDeviceSlotProjection(source, slot, &verifyError),
                     "shadow verification rejected converter output");
        QByteArray damagedSlot = slot;
        damagedSlot[0x00e7 + 2] = char(quint8(damagedSlot[0x00e7 + 2]) ^ 0x01);
        ok &= expect(!verifyDeviceSlotProjection(source, damagedSlot, &verifyError),
                     "shadow verification accepted a semantically damaged slot image");
    }

    auto expectSemanticReject = [&ok, &source](QByteArray candidate,
                                               const char *message) {
        candidate = updateChecksum(std::move(candidate));
        QString error;
        ok &= expect(!validateDeviceSlotCompatibility(candidate, &error), message);
        ok &= expect(!error.isEmpty(), "semantic rejection did not explain the failure");
        ok &= expect(buildDeviceSlotImage(candidate).isEmpty(),
                     "semantic-invalid preset still produced a native slot");
    };

    QByteArray badType = source;
    putU16(badType, 0x00f2, 0x0300);
    expectSemanticReject(badType, "unsupported PEQ type was accepted");

    QByteArray badFrequency = source;
    putU16(badFrequency, 0x00f4, 20001);
    expectSemanticReject(badFrequency, "out-of-range PEQ frequency was accepted");

    QByteArray badQ = source;
    putU16(badQ, 0x00f6, 251);
    expectSemanticReject(badQ, "PEQ Q requiring native clamp was accepted");

    QByteArray badGain = source;
    putU16(badGain, 0x00f8, quint16(qint16(241)));
    expectSemanticReject(badGain, "PEQ gain requiring native clamp was accepted");

    QByteArray badFbe = source;
    badFbe[K500FieldContract::Field::MicFbe.fileOffset] = char(5);
    expectSemanticReject(badFbe, "out-of-domain FBE scalar was accepted");

    QByteArray badChecksum = source;
    badChecksum[0x20] = char(quint8(badChecksum[0x20]) ^ 0x01);
    QString checksumError;
    ok &= expect(!validateDeviceSlotCompatibility(badChecksum, &checksumError),
                 "checksum-invalid preset passed strict compatibility");
    ok &= expect(buildDeviceSlotImage(badChecksum).isEmpty(),
                 "checksum-invalid preset converted to a native slot");

    Document shortDoc(QByteArray(100, char(0)));
    ok &= expect(!shortDoc.validSize(), "short preset should be rejected");
    ok &= expect(buildDeviceSlotImage(shortDoc.bytes()).isEmpty(), "short preset converted to slot image");

    if (!ok) return 1;
    qInfo() << "P3 bit-perfect .k500 codec self-test PASS";
    return 0;
}
