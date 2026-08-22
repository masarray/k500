#include "K500PresetFileBridge.h"

#include "K500PresetCodec.h"
#include "../StudioEngine.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <algorithm>

namespace {
constexpr int ActiveMemorySize = 0x03AB;
constexpr int OfflineActiveNameOffset = 0x02C0;
constexpr int OfflineActiveNameLength = 0x10;
}

K500PresetFileBridge::K500PresetFileBridge(QObject *parent)
    : QObject(parent)
{
}

QObject *K500PresetFileBridge::engine() const
{
    return m_engine.data();
}

void K500PresetFileBridge::setEngine(QObject *engineObject)
{
    StudioEngine *next = qobject_cast<StudioEngine *>(engineObject);
    if (m_engine == next)
        return;
    m_engine = next;
    emit engineChanged();
}

void K500PresetFileBridge::setError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit errorChanged();
}

QString K500PresetFileBridge::localPath(const QUrl &url, bool appendExtension) const
{
    QString path = url.isLocalFile() ? url.toLocalFile() : url.toString(QUrl::PreferLocalFile);
    path = QUrl::fromPercentEncoding(path.toUtf8());
    if (appendExtension && !path.endsWith(QStringLiteral(".k500"), Qt::CaseInsensitive))
        path += QStringLiteral(".k500");
    return path;
}

bool K500PresetFileBridge::loadFile(const QUrl &url)
{
    setError({});
    if (!m_engine) {
        setError(QStringLiteral("Preset file bridge belum memiliki StudioEngine."));
        return false;
    }

    const QString path = localPath(url, false);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("Tidak dapat membuka preset: %1").arg(path));
        return false;
    }

    const QByteArray bytes = file.readAll();
    const K500PresetCodec::Document document(bytes);
    if (!document.validSize()) {
        setError(QStringLiteral("File .k500 harus tepat 0x0478 (1144) byte; file ini %1 byte.").arg(bytes.size()));
        return false;
    }
    if (!document.checksumOk()) {
        setError(QStringLiteral("Checksum .k500 tidak valid; import dibatalkan untuk menjaga byte/device safety."));
        return false;
    }

    QString slotError;
    const QByteArray slot = K500PresetCodec::buildDeviceSlotImage(bytes, &slotError);
    if (slot.size() != K500PresetCodec::DeviceSlotImageLength) {
        setError(slotError.isEmpty() ? QStringLiteral("Gagal membentuk native 0x0290 slot image.") : slotError);
        return false;
    }

    // P3_2_OFFLINE_HYDRATION_V1
    // The native 0x0290 slot image is the authoritative device-shaped subset.
    // Hydrate through the same 939-byte StudioEngine decoder used by CONNECT so
    // offline import cannot invent a second mapping. Tail metadata unavailable
    // in a PC preset remains zero; the visible preset name is injected only into
    // the fixed-width active-mode label used by the decoder.
    QByteArray preview(ActiveMemorySize, char(0));
    std::copy(slot.cbegin(), slot.cend(), preview.begin());
    const QByteArray visibleName = document.name().toLatin1().left(OfflineActiveNameLength);
    for (int i = 0; i < visibleName.size(); ++i)
        preview[OfflineActiveNameOffset + i] = visibleName.at(i);
    m_engine->hydrateFromDeviceMemory(preview);

    m_sourceBytes = bytes;
    m_sourcePath = path;
    m_sourceName = QFileInfo(path).fileName();
    m_presetName = document.name();
    m_checksumOk = true;
    emit sourceChanged();
    emit loadedFile(path, m_presetName);
    return true;
}

bool K500PresetFileBridge::saveFile(const QUrl &url)
{
    setError({});
    if (m_sourceBytes.size() != K500PresetCodec::PresetFileLength) {
        setError(QStringLiteral("Belum ada source .k500 valid untuk diekspor."));
        return false;
    }

    const QString path = localPath(url, true);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(QStringLiteral("Tidak dapat membuat file preset: %1").arg(path));
        return false;
    }

    // P3_2_NOOP_EXPORT_V1 — until an editor patch is explicitly committed to
    // the whitelist codec, Save As writes the exact imported source bytes.
    if (file.write(m_sourceBytes) != m_sourceBytes.size() || !file.commit()) {
        setError(QStringLiteral("Gagal menyimpan preset secara atomik: %1").arg(path));
        return false;
    }

    emit savedFile(path);
    return true;
}

void K500PresetFileBridge::clear()
{
    if (m_sourceBytes.isEmpty() && m_sourcePath.isEmpty() && m_presetName.isEmpty())
        return;
    m_sourceBytes.clear();
    m_sourcePath.clear();
    m_sourceName.clear();
    m_presetName.clear();
    m_checksumOk = false;
    setError({});
    emit sourceChanged();
}

QByteArray K500PresetFileBridge::deviceSlotImage() const
{
    if (m_sourceBytes.size() != K500PresetCodec::PresetFileLength)
        return {};
    return K500PresetCodec::buildDeviceSlotImage(m_sourceBytes);
}
