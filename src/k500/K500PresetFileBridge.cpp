#include "K500PresetFileBridge.h"

#include "K500PresetCodec.h"
#include "K500PresetEditMapper.h"
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
    return m_engine;
}

void K500PresetFileBridge::setEngine(QObject *engineObject)
{
    StudioEngine *next = qobject_cast<StudioEngine *>(engineObject);
    if (m_engine == next)
        return;

    if (m_engineEditConnection)
        QObject::disconnect(m_engineEditConnection);

    m_engine = next;
    if (m_engine) {
        // P3_4_CONTROLLED_EDIT_PERSISTENCE_V1
        // StudioEngine remains the single canonical edit stream. The file bridge
        // never watches QML controls directly and never invents a second model.
        m_engineEditConnection = QObject::connect(
            m_engine, &StudioEngine::stateEdited,
            this, &K500PresetFileBridge::onEngineEdit);
    } else {
        m_engineEditConnection = {};
    }
    emit engineChanged();
    emit sourceChanged();
}

int K500PresetFileBridge::changedByteCount() const
{
    if (!loaded() || m_savedBytes.size() != m_sourceBytes.size())
        return 0;
    int count = 0;
    for (int i = 0; i < m_sourceBytes.size(); ++i)
        if (m_sourceBytes.at(i) != m_savedBytes.at(i)) ++count;
    return count;
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

void K500PresetFileBridge::refreshDocumentMetadata()
{
    const K500PresetCodec::Document document(m_sourceBytes);
    m_presetName = document.name();
    m_checksumOk = document.validSize() && document.checksumOk();
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

    // Assign the validated source only after all format checks pass. Hydration is
    // contractually read-only; nevertheless the canonical edit callback is safe
    // because StudioEngine hydration emits zero stateEdited events (P0 guard).
    m_sourceBytes = bytes;
    m_savedBytes = bytes;
    m_sourcePath = path;
    m_sourceName = QFileInfo(path).fileName();
    refreshDocumentMetadata();

    // P3_2_OFFLINE_HYDRATION_V1
    QByteArray preview(ActiveMemorySize, char(0));
    std::copy(slot.cbegin(), slot.cend(), preview.begin());
    const QByteArray visibleName = document.name().toLatin1().left(OfflineActiveNameLength);
    for (int i = 0; i < visibleName.size(); ++i)
        preview[OfflineActiveNameOffset + i] = visibleName.at(i);
    m_engine->hydrateFromDeviceMemory(preview);

    emit sourceChanged();
    emit loadedFile(path, m_presetName);
    return true;
}

void K500PresetFileBridge::onEngineEdit(const QString &path, const QVariant &value)
{
    if (!loaded())
        return;

    const auto edit = K500PresetEditMapper::applyEngineEdit(m_sourceBytes, path, value);
    if (!edit.supported)
        return; // intentionally read-only/unverified field; never mutate bytes

    if (!edit.patch.ok) {
        const QString reason = edit.patch.error.isEmpty()
            ? QStringLiteral("Edit tidak lolos whitelist serializer.")
            : edit.patch.error;
        setError(QStringLiteral("%1: %2").arg(path, reason));
        emit persistenceRejected(path, reason);
        return;
    }

    if (edit.patch.bytes == m_sourceBytes)
        return;
    if (!K500PresetCodec::validateChecksum(edit.patch.bytes)) {
        const QString reason = QStringLiteral("Serializer menghasilkan checksum tidak valid; perubahan ditolak.");
        setError(reason);
        emit persistenceRejected(path, reason);
        return;
    }

    m_sourceBytes = edit.patch.bytes;
    refreshDocumentMetadata();
    setError({});
    emit sourceChanged();
    emit persistedEdit(path, edit.patch.changedOffsets.size());
}

bool K500PresetFileBridge::saveFile(const QUrl &url)
{
    setError({});
    if (!loaded() || !K500PresetCodec::validateChecksum(m_sourceBytes)) {
        setError(QStringLiteral("Belum ada working .k500 valid untuk diekspor."));
        return false;
    }

    const QString path = localPath(url, true);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(QStringLiteral("Tidak dapat membuat file preset: %1").arg(path));
        return false;
    }

    // P3_4_EDITED_EXPORT_V1
    // No-op Save As is still byte-identical. Once verified edits exist, only
    // mapper-whitelisted bytes plus checksum differ from the loaded checkpoint.
    if (file.write(m_sourceBytes) != m_sourceBytes.size() || !file.commit()) {
        setError(QStringLiteral("Gagal menyimpan preset secara atomik: %1").arg(path));
        return false;
    }

    m_savedBytes = m_sourceBytes;
    m_sourcePath = path;
    m_sourceName = QFileInfo(path).fileName();
    refreshDocumentMetadata();
    emit sourceChanged();
    emit savedFile(path);
    return true;
}

void K500PresetFileBridge::clear()
{
    if (m_sourceBytes.isEmpty() && m_sourcePath.isEmpty() && m_presetName.isEmpty())
        return;
    m_sourceBytes.clear();
    m_savedBytes.clear();
    m_sourcePath.clear();
    m_sourceName.clear();
    m_presetName.clear();
    m_checksumOk = false;
    setError({});
    emit sourceChanged();
}

QByteArray K500PresetFileBridge::deviceSlotImage() const
{
    if (!loaded() || !K500PresetCodec::validateChecksum(m_sourceBytes))
        return {};
    return K500PresetCodec::buildDeviceSlotImage(m_sourceBytes);
}
