#include "K500PresetCodec.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QTextStream>
#include <QtEndian>
#include <algorithm>
#include <cstdlib>

namespace {
int fail(const QString &message)
{
    QTextStream(stderr) << "P3.2 corpus failure: " << message << '\n';
    return 1;
}

quint16 u16(const QByteArray &bytes, int offset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(bytes.constData() + offset));
}

qint16 i16(const QByteArray &bytes, int offset)
{
    return static_cast<qint16>(u16(bytes, offset));
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2 && argc != 3)
        return fail(QStringLiteral("expected one .k500 fixture path and optional --slot-sha256"));
    const bool printSlotSha = argc == 3
        && QString::fromLocal8Bit(argv[2]) == QStringLiteral("--slot-sha256");
    if (argc == 3 && !printSlotSha)
        return fail(QStringLiteral("unknown optional argument"));

    QFile file(QString::fromLocal8Bit(argv[1]));
    if (!file.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("cannot open fixture"));
    const QByteArray source = file.readAll();

    K500PresetCodec::Document doc(source);
    if (!doc.validSize())
        return fail(QStringLiteral("fixture is not 0x0478 bytes"));
    if (!doc.checksumOk())
        return fail(QStringLiteral("fixture checksum is invalid"));
    if (doc.name() != QStringLiteral("KARAOKE ARTIST"))
        return fail(QStringLiteral("unexpected donor preset name: %1").arg(doc.name()));
    if (doc.serializeNoop() != source)
        return fail(QStringLiteral("real donor no-op round trip changed bytes"));

    QString error;
    if (!K500PresetCodec::validateDeviceSlotCompatibility(source, &error))
        return fail(QStringLiteral("fixture is not native-slot compatible: %1").arg(error));
    const QByteArray slot = K500PresetCodec::buildDeviceSlotImage(source, &error);
    if (slot.size() != K500PresetCodec::DeviceSlotImageLength)
        return fail(QStringLiteral("slot conversion failed: %1").arg(error));

    // Verify the real donor scalar split around the one-byte file hole 0x0097.
    if (slot.at(0x008e) != source.at(0x0096))
        return fail(QStringLiteral("low scalar +8 mapping regressed"));
    if (slot.at(0x008f) != source.at(0x0098))
        return fail(QStringLiteral("high scalar +9 mapping regressed"));

    // Verify the first Mic A PEQ record is compacted from 8 file bytes to the
    // exact 5-byte native representation, not copied/sliced from the file.
    constexpr int src = 0x00f0 + 2;
    constexpr int dst = 0x00e7;
    const quint16 typeRaw = u16(source, src);
    const quint16 frequency = u16(source, src + 2);
    const quint16 qRaw = u16(source, src + 4);
    const qint16 gainRaw = i16(source, src + 6);
    const quint8 typeNibble = typeRaw == 0x0100 ? 0x10 : typeRaw == 0x0200 ? 0x20 : 0x00;
    if (static_cast<quint8>(slot.at(dst)) != (frequency & 0xff)
        || static_cast<quint8>(slot.at(dst + 1)) != ((frequency >> 8) & 0xff)
        || static_cast<quint8>(slot.at(dst + 2)) != qRaw
        || static_cast<quint8>(slot.at(dst + 3)) != (typeNibble | (gainRaw < 0 ? 0x80 : 0x00))
        || static_cast<quint8>(slot.at(dst + 4)) != std::abs(static_cast<int>(gainRaw)))
        return fail(QStringLiteral("real donor compact PEQ mapping regressed"));

    if (slot.mid(0x027c, 4) != source.mid(0x044c, 4))
        return fail(QStringLiteral("native tail mapping regressed"));
    if (slot.mid(0x0280, 0x10) != source.mid(K500PresetCodec::NameOffset, 0x10))
        return fail(QStringLiteral("native 16-byte name mapping regressed"));
    if (slot == source.left(K500PresetCodec::DeviceSlotImageLength))
        return fail(QStringLiteral("converter degraded to raw .k500 slicing"));

    QString projectionError;
    if (!K500PresetCodec::verifyDeviceSlotProjection(source, slot, &projectionError))
        return fail(QStringLiteral("shadow projection verification failed: %1").arg(projectionError));

    if (printSlotSha) {
        const QByteArray digest = QCryptographicHash::hash(slot, QCryptographicHash::Sha256).toHex();
        QTextStream(stdout) << "SLOT_SHA256=" << digest << '\n';
    }
    QTextStream(stdout) << "P3.2 donor corpus PASS: " << doc.name() << '\n';
    return 0;
}
