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
    // P4_2_PRESET_BATCH_LIBRARY_V1
    // Batch construction is deliberately side-effect free with respect to the
    // currently loaded/edited working document. Every selected file must pass
    // the same strict P3 size/checksum/conversion contract independently.
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

    // File-dialog selection order is platform dependent. Sort by filename so
    // slot assignment is deterministic and reviewable before touching hardware.
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
