#include "K500PresetFileBridge.h"
#include "K500PresetCodec.h"
#include "../StudioEngine.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUrl>
#include <QVariantMap>

namespace {
int fail(const QString &message)
{
    QTextStream(stderr) << "P4.2 batch library failure: " << message << '\n';
    return 1;
}

bool writeFile(const QString &path, const QByteArray &bytes)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
}

void putFixedAscii(QByteArray &memory, int offset, int length, const QByteArray &text)
{
    if (offset < 0 || offset + length > memory.size())
        return;
    for (int i = 0; i < length; ++i)
        memory[offset + i] = char(0);
    const QByteArray clipped = text.left(length);
    for (int i = 0; i < clipped.size(); ++i)
        memory[offset + i] = clipped.at(i);
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2)
        return fail(QStringLiteral("expected donor .k500 fixture path"));

    QFile donor(QString::fromLocal8Bit(argv[1]));
    if (!donor.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("cannot open donor fixture"));
    const QByteArray source = donor.readAll();
    if (!K500PresetCodec::validateChecksum(source))
        return fail(QStringLiteral("donor checksum invalid"));

    QTemporaryDir dir;
    if (!dir.isValid())
        return fail(QStringLiteral("cannot create temp directory"));

    const QString bPath = dir.filePath(QStringLiteral("02_BETA.k500"));
    const QString aPath = dir.filePath(QStringLiteral("01_ALPHA.k500"));
    if (!writeFile(bPath, source) || !writeFile(aPath, source))
        return fail(QStringLiteral("cannot create batch fixtures"));

    K500PresetFileBridge bridge;
    QVariantList urls;
    // Deliberately reverse the selection order: legacy mapping must remain
    // deterministic by filename, not FileDialog/platform selection order.
    urls << QUrl::fromLocalFile(bPath) << QUrl::fromLocalFile(aPath);
    const QVariantList entries = bridge.buildMassUploadEntries(urls, 4);
    if (entries.size() != 2)
        return fail(QStringLiteral("valid two-file batch rejected"));

    const QVariantMap first = entries.at(0).toMap();
    const QVariantMap second = entries.at(1).toMap();
    if (first.value(QStringLiteral("slot")).toInt() != 4
        || second.value(QStringLiteral("slot")).toInt() != 5
        || !first.value(QStringLiteral("path")).toString().endsWith(QStringLiteral("01_ALPHA.k500"))
        || !second.value(QStringLiteral("path")).toString().endsWith(QStringLiteral("02_BETA.k500")))
        return fail(QStringLiteral("filename sort / sequential slot mapping regressed"));
    if (first.value(QStringLiteral("image")).toByteArray().size() != K500PresetCodec::DeviceSlotImageLength
        || second.value(QStringLiteral("image")).toByteArray().size() != K500PresetCodec::DeviceSlotImageLength)
        return fail(QStringLiteral("batch produced non-0x0290 slot image"));

    if (!bridge.buildMassUploadEntries(urls, 10).isEmpty())
        return fail(QStringLiteral("slot overflow was not rejected"));

    // SYSTEM_TRANSFER_LIST_V1: explicit right-list order is authoritative.
    QVariantList transferPaths;
    transferPaths << bPath << aPath;
    const QVariantList transferEntries = bridge.buildTransferUploadEntries(transferPaths);
    if (transferEntries.size() != 2)
        return fail(QStringLiteral("valid transfer-list batch rejected"));
    const QVariantMap transferFirst = transferEntries.at(0).toMap();
    const QVariantMap transferSecond = transferEntries.at(1).toMap();
    if (transferFirst.value(QStringLiteral("slot")).toInt() != 1
        || transferSecond.value(QStringLiteral("slot")).toInt() != 2
        || !transferFirst.value(QStringLiteral("path")).toString().endsWith(QStringLiteral("02_BETA.k500"))
        || !transferSecond.value(QStringLiteral("path")).toString().endsWith(QStringLiteral("01_ALPHA.k500")))
        return fail(QStringLiteral("transfer-list order was sorted or remapped unexpectedly"));

    QVariantList tooMany;
    for (int i = 0; i < 11; ++i)
        tooMany << aPath;
    if (!bridge.buildTransferUploadEntries(tooMany).isEmpty())
        return fail(QStringLiteral("transfer-list accepted more than 10 device slots"));

    // DEVICE_TRUTH_STAGING_V1: loading a PC preset must not hydrate or replace
    // the StudioEngine state representing the actual K500, and subsequent LIVE
    // edits must not silently mutate the staged PC bytes.
    StudioEngine engine;
    QByteArray deviceMemory(0x03AB, char(0));
    const QStringList hardwareNames{
        QStringLiteral("DEVICE MODE 01"), QStringLiteral("DEVICE MODE 02"),
        QStringLiteral("DEVICE MODE 03"), QStringLiteral("DEVICE MODE 04"),
        QStringLiteral("DEVICE MODE 05"), QStringLiteral("DEVICE MODE 06"),
        QStringLiteral("DEVICE MODE 07"), QStringLiteral("DEVICE MODE 08"),
        QStringLiteral("DEVICE MODE 09"), QStringLiteral("DEVICE MODE 10")};
    for (int i = 0; i < hardwareNames.size(); ++i)
        putFixedAscii(deviceMemory, 0x0290 + i * 0x10, 0x10, hardwareNames.at(i).toLatin1());
    putFixedAscii(deviceMemory, 0x02C0, 0x10, QByteArray("DEVICE MODE 04"));
    engine.hydrateFromDeviceMemory(deviceMemory);
    const QVariantMap deviceStateBeforeStage = engine.deviceState();

    bridge.setEngine(&engine);
    bridge.setEditTracking(false);
    if (!bridge.loadFile(QUrl::fromLocalFile(aPath)))
        return fail(QStringLiteral("valid preset could not be staged"));
    if (engine.deviceState() != deviceStateBeforeStage)
        return fail(QStringLiteral("staging PC preset changed StudioEngine/device truth"));

    const QByteArray stagedImageBeforeLiveEdit = bridge.deviceSlotImage();
    engine.setMasterMusic(37.0);
    const QByteArray stagedImageAfterLiveEdit = bridge.deviceSlotImage();
    if (stagedImageBeforeLiveEdit != stagedImageAfterLiveEdit)
        return fail(QStringLiteral("LIVE editor tweak mutated staged PC preset"));

    QByteArray corrupt = source;
    corrupt[0x20] = static_cast<char>(static_cast<unsigned char>(corrupt.at(0x20)) ^ 0x01);
    const QString badPath = dir.filePath(QStringLiteral("03_CORRUPT.k500"));
    if (!writeFile(badPath, corrupt))
        return fail(QStringLiteral("cannot create corrupt fixture"));
    QVariantList badUrls;
    badUrls << QUrl::fromLocalFile(aPath) << QUrl::fromLocalFile(badPath);
    if (!bridge.buildMassUploadEntries(badUrls, 1).isEmpty())
        return fail(QStringLiteral("checksum-invalid member did not abort entire batch"));
    if (!bridge.lastError().contains(QStringLiteral("Checksum"), Qt::CaseInsensitive))
        return fail(QStringLiteral("batch checksum rejection did not surface an error"));

    QTextStream(stdout) << "P4.2 donor batch + transfer staging PASS\n";
    return 0;
}
