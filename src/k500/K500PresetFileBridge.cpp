#include "K500PresetFileBridge.h"

#include "K500PresetCodec.h"
#include "K500PresetEditMapper.h"
#include "../StudioEngine.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <algorithm>

namespace {
constexpr int ActiveMemorySize = 0x03AB;
constexpr int OfflineActiveNameOffset = 0x02C0;
constexpr int OfflineActiveNameLength = 0x10;

struct BuiltInPresetDefinition {
    const char *displayName;
    const char *description;
    const char *fileName;
    const char *resourcePath;
};

constexpr BuiltInPresetDefinition BuiltInPresetDefinitions[] = {
    {"ALL GENRE",       "Universal karaoke flagship",          "01_ALL_GENRE.k500",       ":/presets/01_ALL_GENRE.k500"},
    {"BROADCAST",       "Podcast, radio and MC",               "02_BROADCAST.k500",       ":/presets/02_BROADCAST.k500"},
    {"DANGDUT SUPREME", "Dangdut pitch-lock and cengkok",      "03_DANGDUT_SUPREME.k500", ":/presets/03_DANGDUT_SUPREME.k500"},
    {"ROCK",            "Forward vocal and punch",             "04_ROCK.k500",            ":/presets/04_ROCK.k500"},
    {"POP KENANGAN",    "Smooth nostalgic pop vocal",          "05_POP_KENANGAN.k500",    ":/presets/05_POP_KENANGAN.k500"},
    {"QORI / SHOLAWAT", "Long-phrase spiritual vocal support", "06_QORI_SHOLAWAT.k500",   ":/presets/06_QORI_SHOLAWAT.k500"},
    {"JAZZ",            "Natural dynamic vocal support",       "07_JAZZ.k500",            ":/presets/07_JAZZ.k500"},
    {"BLUES",           "Warm controlled vocal",               "08_BLUES.k500",           ":/presets/08_BLUES.k500"},
    {"ACOUSTIC",        "Intimate vocal and music",            "09_ACOUSTIC.k500",        ":/presets/09_ACOUSTIC.k500"},
    {"REGGAE",          "Relaxed rhythmic vocal support",      "10_REGGAE.k500",          ":/presets/10_REGGAE.k500"},
};
}

K500PresetFileBridge::K500PresetFileBridge(QObject *parent)
    : QObject(parent)
{
    rebuildBuiltInPresets();

    const QString rememberedFolder = QSettings().value(
        QStringLiteral("pcPresetLibrary/folder")).toString();
    if (!rememberedFolder.isEmpty() && QDir(rememberedFolder).exists()) {
        m_presetFolder = QDir::cleanPath(rememberedFolder);
        rebuildFolderPresets();
    }
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

QVariantMap K500PresetFileBridge::describePreset(const QByteArray &bytes,
                                                 const QString &displayName,
                                                 const QString &fileName,
                                                 const QString &path,
                                                 const QString &source,
                                                 int index) const
{
    const K500PresetCodec::Document document(bytes);
    const bool sizeOk = document.validSize();
    const bool checksumOk = sizeOk && document.checksumOk();

    QVariantMap entry;
    entry.insert(QStringLiteral("index"), index);
    entry.insert(QStringLiteral("displayName"), displayName.isEmpty()
        ? (checksumOk && !document.name().isEmpty() ? document.name() : fileName)
        : displayName);
    entry.insert(QStringLiteral("presetName"), checksumOk ? document.name() : QString());
    entry.insert(QStringLiteral("fileName"), fileName);
    entry.insert(QStringLiteral("path"), path);
    entry.insert(QStringLiteral("source"), source);
    entry.insert(QStringLiteral("size"), bytes.size());
    entry.insert(QStringLiteral("sizeOk"), sizeOk);
    entry.insert(QStringLiteral("checksumOk"), checksumOk);
    entry.insert(QStringLiteral("valid"), sizeOk && checksumOk);
    return entry;
}

void K500PresetFileBridge::rebuildBuiltInPresets()
{
    m_builtInPresets.clear();
    int index = 0;
    for (const BuiltInPresetDefinition &definition : BuiltInPresetDefinitions) {
        QFile file(QString::fromLatin1(definition.resourcePath));
        QByteArray bytes;
        if (file.open(QIODevice::ReadOnly))
            bytes = file.readAll();

        QVariantMap entry = describePreset(
            bytes,
            QString::fromLatin1(definition.displayName),
            QString::fromLatin1(definition.fileName),
            QString::fromLatin1(definition.resourcePath),
            QStringLiteral("builtin"),
            index);
        entry.insert(QStringLiteral("description"), QString::fromLatin1(definition.description));
        entry.insert(QStringLiteral("resourcePath"), QString::fromLatin1(definition.resourcePath));
        m_builtInPresets.append(entry);
        ++index;
    }
}

void K500PresetFileBridge::rebuildFolderPresets()
{
    QVariantList next;
    if (!m_presetFolder.isEmpty()) {
        QDir dir(m_presetFolder);
        if (dir.exists()) {
            const QStringList filters{QStringLiteral("*.k500")};
            const QFileInfoList files = dir.entryInfoList(
                filters,
                QDir::Files | QDir::Readable | QDir::NoSymLinks,
                QDir::Name | QDir::IgnoreCase);

            int index = 0;
            for (const QFileInfo &info : files) {
                QFile file(info.absoluteFilePath());
                QByteArray bytes;
                if (file.open(QIODevice::ReadOnly))
                    bytes = file.readAll();

                next.append(describePreset(
                    bytes,
                    QString(),
                    info.fileName(),
                    info.absoluteFilePath(),
                    QStringLiteral("folder"),
                    index));
                ++index;
            }
        }
    }

    m_folderPresets = next;
    emit libraryChanged();
}

bool K500PresetFileBridge::loadValidatedBytes(const QByteArray &bytes,
                                              const QString &sourcePath,
                                              const QString &sourceName)
{
    setError({});
    if (!m_engine) {
        setError(QStringLiteral("Preset file bridge belum memiliki StudioEngine."));
        return false;
    }

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
    m_sourcePath = sourcePath;
    m_sourceName = sourceName;
    refreshDocumentMetadata();

    // P3_2_OFFLINE_HYDRATION_V1
    QByteArray preview(ActiveMemorySize, char(0));
    std::copy(slot.cbegin(), slot.cend(), preview.begin());
    const QByteArray visibleName = document.name().toLatin1().left(OfflineActiveNameLength);
    for (int i = 0; i < visibleName.size(); ++i)
        preview[OfflineActiveNameOffset + i] = visibleName.at(i);
    m_engine->hydrateFromDeviceMemory(preview);

    emit sourceChanged();
    emit loadedFile(sourcePath, m_presetName);
    return true;
}

bool K500PresetFileBridge::loadFile(const QUrl &url)
{
    setError({});
    const QString path = localPath(url, false);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("Tidak dapat membuka preset: %1").arg(path));
        return false;
    }

    return loadValidatedBytes(file.readAll(), path, QFileInfo(path).fileName());
}

bool K500PresetFileBridge::setPresetFolder(const QUrl &url)
{
    setError({});
    QString path = localPath(url, false);
    if (path.isEmpty()) {
        setError(QStringLiteral("Folder preset tidak valid."));
        return false;
    }

    QFileInfo info(path);
    if (info.isFile())
        path = info.absolutePath();
    path = QDir::cleanPath(path);

    QDir dir(path);
    if (!dir.exists()) {
        setError(QStringLiteral("Folder preset tidak ditemukan: %1").arg(path));
        return false;
    }

    m_presetFolder = dir.absolutePath();
    QSettings().setValue(QStringLiteral("pcPresetLibrary/folder"), m_presetFolder);
    rebuildFolderPresets();
    return true;
}

void K500PresetFileBridge::refreshPresetFolder()
{
    rebuildFolderPresets();
}

bool K500PresetFileBridge::loadFolderPreset(int index)
{
    setError({});
    if (index < 0 || index >= m_folderPresets.size()) {
        setError(QStringLiteral("Preset folder index tidak valid."));
        return false;
    }

    const QVariantMap entry = m_folderPresets.at(index).toMap();
    if (!entry.value(QStringLiteral("valid")).toBool()) {
        setError(QStringLiteral("Preset %1 tidak valid dan tidak akan diload.")
                     .arg(entry.value(QStringLiteral("fileName")).toString()));
        return false;
    }

    return loadFile(QUrl::fromLocalFile(entry.value(QStringLiteral("path")).toString()));
}

bool K500PresetFileBridge::loadBuiltInPreset(int index)
{
    setError({});
    if (index < 0 || index >= m_builtInPresets.size()) {
        setError(QStringLiteral("Built-in preset index tidak valid."));
        return false;
    }

    const QVariantMap entry = m_builtInPresets.at(index).toMap();
    const QString resourcePath = entry.value(QStringLiteral("resourcePath")).toString();
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("Built-in preset resource tidak ditemukan: %1").arg(resourcePath));
        return false;
    }

    const QString sourcePath = QStringLiteral("builtin://%1")
        .arg(entry.value(QStringLiteral("fileName")).toString());
    return loadValidatedBytes(
        file.readAll(),
        sourcePath,
        entry.value(QStringLiteral("fileName")).toString());
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

    if (!m_presetFolder.isEmpty()
        && QDir::cleanPath(QFileInfo(path).absolutePath()) == QDir::cleanPath(m_presetFolder)) {
        rebuildFolderPresets();
    }
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
