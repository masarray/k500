#include "K500PresetFileBridge.h"

#include "K500PresetCodec.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QVector>
#include <algorithm>

QVariantList K500PresetFileBridge::buildMassUploadEntries(const QVariantList &urls,
                                                           int startSlotOneBased)
{
    setError({});
    if (urls.isEmpty()) {
        setError(QStringLiteral("Pilih minimal satu file .k500 untuk Mass Upload."));
        return {};
    }
    if (urls.size() > 10) {
        setError(QStringLiteral("Mass Upload maksimum 10 preset."));
        return {};
    }

    const int startSlot = qBound(1, startSlotOneBased, 10);
    if (startSlot + urls.size() - 1 > 10) {
        setError(QStringLiteral("Batch %1 file tidak muat mulai slot %2; slot maksimum 10.")
                     .arg(urls.size()).arg(startSlot));
        return {};
    }

    struct Source {
        QString path;
        QString fileName;
    };
    QVector<Source> sources;
    sources.reserve(urls.size());
    for (const QVariant &value : urls) {
        const QUrl url = value.toUrl();
        const QString path = localPath(url, false);
        if (path.isEmpty() || !QFileInfo(path).isFile()) {
            setError(QStringLiteral("Mass Upload hanya menerima file lokal .k500 yang valid."));
            return {};
        }
        if (!path.endsWith(QStringLiteral(".k500"), Qt::CaseInsensitive)) {
            setError(QStringLiteral("File batch bukan .k500: %1").arg(QFileInfo(path).fileName()));
            return {};
        }
        sources.push_back({path, QFileInfo(path).fileName()});
    }

    std::sort(sources.begin(), sources.end(), [](const Source &a, const Source &b) {
        const int nameCompare = QString::compare(a.fileName, b.fileName, Qt::CaseInsensitive);
        if (nameCompare != 0) return nameCompare < 0;
        return QString::compare(a.path, b.path, Qt::CaseInsensitive) < 0;
    });

    QVariantList entries;
    entries.reserve(sources.size());
    for (int i = 0; i < sources.size(); ++i) {
        QFile file(sources.at(i).path);
        if (!file.open(QIODevice::ReadOnly)) {
            setError(QStringLiteral("Tidak dapat membuka batch preset: %1").arg(sources.at(i).fileName));
            return {};
        }
        const QByteArray bytes = file.readAll();
        const K500PresetCodec::Document document(bytes);
        if (!document.validSize()) {
            setError(QStringLiteral("%1 bukan file K500 1144-byte.").arg(sources.at(i).fileName));
            return {};
        }
        if (!document.checksumOk()) {
            setError(QStringLiteral("Checksum tidak valid pada %1; seluruh batch dibatalkan.")
                         .arg(sources.at(i).fileName));
            return {};
        }

        QString conversionError;
        const QByteArray image = K500PresetCodec::buildDeviceSlotImage(bytes, &conversionError);
        if (image.size() != K500PresetCodec::DeviceSlotImageLength) {
            setError(QStringLiteral("Konversi %1 gagal: %2")
                         .arg(sources.at(i).fileName,
                              conversionError.isEmpty() ? QStringLiteral("slot image invalid") : conversionError));
            return {};
        }

        QVariantMap entry;
        entry.insert(QStringLiteral("slot"), startSlot + i);
        entry.insert(QStringLiteral("image"), image);
        entry.insert(QStringLiteral("name"), document.name());
        entry.insert(QStringLiteral("path"), sources.at(i).path);
        entries.append(entry);
    }

    return entries;
}

QVariantList K500PresetFileBridge::buildTransferUploadEntries(const QVariantList &paths)
{
    // The right-hand transfer list is authoritative for slot assignment. Never
    // sort it: visible row 1 -> slot 1, row 10 -> slot 10. Sources may be a local
    // user/cache file or a bundled Qt resource; both pass the same codec gate.
    setError({});
    if (paths.isEmpty()) {
        setError(QStringLiteral("Tambahkan minimal satu preset ke daftar Device Slots."));
        return {};
    }
    if (paths.size() > 10) {
        setError(QStringLiteral("Device hanya menerima maksimum 10 preset."));
        return {};
    }

    QVariantList entries;
    entries.reserve(paths.size());

    for (int i = 0; i < paths.size(); ++i) {
        const QVariant value = paths.at(i);
        const QUrl url = value.toUrl();
        QString path = url.isLocalFile() ? url.toLocalFile() : value.toString();
        path = QUrl::fromPercentEncoding(path.toUtf8());

        const bool resourcePath = path.startsWith(QStringLiteral(":/"));
        const QFileInfo info(path);
        const QString fileName = resourcePath
            ? path.section(QLatin1Char('/'), -1)
            : info.fileName();
        if (path.isEmpty()
            || (!resourcePath && !info.isFile())
            || !path.endsWith(QStringLiteral(".k500"), Qt::CaseInsensitive)) {
            setError(QStringLiteral("Slot %1 bukan sumber .k500 yang valid: %2")
                         .arg(i + 1).arg(fileName.isEmpty() ? path : fileName));
            return {};
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            setError(QStringLiteral("Tidak dapat membuka preset slot %1: %2")
                         .arg(i + 1).arg(fileName));
            return {};
        }

        const QByteArray bytes = file.readAll();
        const K500PresetCodec::Document document(bytes);
        if (!document.validSize()) {
            setError(QStringLiteral("%1 bukan file K500 1144-byte.").arg(fileName));
            return {};
        }
        if (!document.checksumOk()) {
            setError(QStringLiteral("Checksum tidak valid pada %1; seluruh batch dibatalkan.")
                         .arg(fileName));
            return {};
        }

        QString conversionError;
        const QByteArray image = K500PresetCodec::buildDeviceSlotImage(bytes, &conversionError);
        if (image.size() != K500PresetCodec::DeviceSlotImageLength) {
            setError(QStringLiteral("Konversi %1 gagal: %2")
                         .arg(fileName,
                              conversionError.isEmpty() ? QStringLiteral("slot image invalid") : conversionError));
            return {};
        }

        QVariantMap entry;
        entry.insert(QStringLiteral("slot"), i + 1);
        entry.insert(QStringLiteral("image"), image);
        entry.insert(QStringLiteral("name"), document.name());
        entry.insert(QStringLiteral("path"), path);
        entries.append(entry);
    }

    return entries;
}
