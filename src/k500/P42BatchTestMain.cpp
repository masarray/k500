#include "K500PresetFileBridge.h"
#include "K500PresetCodec.h"

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
    // Deliberately reverse the selection order: mapping must be deterministic
    // by filename, not dependent on FileDialog/platform selection order.
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

    QTextStream(stdout) << "P4.2 donor batch library PASS\n";
    return 0;
}
