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
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    QByteArray source(PresetFileLength, char(0));
    for (int i = 0; i < source.size(); ++i)
        source[i] = char((i * 73 + 19) & 0xff);

    const QByteArray visibleName("P3 BIT PERFECT");
    source.replace(NameOffset, visibleName.size(), visibleName);

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
    }

    Document shortDoc(QByteArray(100, char(0)));
    ok &= expect(!shortDoc.validSize(), "short preset should be rejected");
    ok &= expect(buildDeviceSlotImage(shortDoc.bytes()).isEmpty(), "short preset converted to slot image");

    if (!ok) return 1;
    qInfo() << "P3 bit-perfect .k500 codec self-test PASS";
    return 0;
}
