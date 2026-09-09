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

const BuiltInPresetDefinition *definitionForFileName(const QString &fileName)
{
    for (const auto &definition : BuiltInPresetDefinitions) {
        if (fileName.compare(QString::fromLatin1(definition.fileName), Qt::CaseInsensitive) == 0)
            return &definition;
    }
    return nullptr;
}

bool readValidPreset(const QString &path, QByteArray *bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray candidate = file.readAll();
    const K500PresetCodec::Document document(candidate);
    if (!document.validSize() || !document.checksumOk())
        return false;
    if (bytes)
        *bytes = candidate;
    return true;
}
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
    } else {
        rebuildCombinedPresets();
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
        m_engineEditConnection = QObject::connect(
            m_engine, &StudioEngine::stateEdited,
            this, &K500PresetFileBridge::onEngineEdit);
    } else {
        m_engineEditConnection = {};
    }
    emit engineChanged();
    emit sourceChanged();
}

void K500PresetFileBridge::setEditTracking(bool enabled)
{
    if (m_editTracking == enabled)
        return;
    m_editTracking = enabled;
    emit editTrackingChanged();
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
    QVariantList next;
    const QDir cacheDir(officialCacheDirectory());
    QStringList knownNames;
    int index = 0;

    // Bundled presets are a permanent offline fallback. A checksum-valid cached
    // copy with the same filename transparently overrides it after GitHub sync.
    for (const BuiltInPresetDefinition &definition : BuiltInPresetDefinitions) {
        const QString fileName = QString::fromLatin1(definition.fileName);
        knownNames.append(fileName.toLower());

        QByteArray bytes;
        const QString cachedPath = cacheDir.filePath(fileName);
        QString path = QString::fromLatin1(definition.resourcePath);
        QString source = QStringLiteral("official-bundled");
        if (readValidPreset(cachedPath, &bytes)) {
            path = cachedPath;
            source = QStringLiteral("official-cache");
        } else {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly))
                bytes = file.readAll();
        }

        QVariantMap entry = describePreset(
            bytes,
            QString::fromLatin1(definition.displayName),
            fileName,
            path,
            source,
            index++);
        entry.insert(QStringLiteral("description"), QString::fromLatin1(definition.description));
        entry.insert(QStringLiteral("originLabel"), QStringLiteral("SONKUPIK"));
        next.append(entry);
    }

    // Any additional .k500 file added later to resources/presets on GitHub is
    // discovered by sync and appears automatically without an application update.
    if (cacheDir.exists()) {
        const QFileInfoList files = cacheDir.entryInfoList(
            {QStringLiteral("*.k500")},
            QDir::Files | QDir::Readable | QDir::NoSymLinks,
            QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo &info : files) {
            if (knownNames.contains(info.fileName().toLower()))
                continue;
            QByteArray bytes;
            if (!readValidPreset(info.absoluteFilePath(), &bytes))
                continue;
            QVariantMap entry = describePreset(
                bytes,
                QString(),
                info.fileName(),
                info.absoluteFilePath(),
                QStringLiteral("official-cache"),
                index++);
            entry.insert(QStringLiteral("description"), QStringLiteral("Official SonKuPik preset"));
            entry.insert(QStringLiteral("originLabel"), QStringLiteral("SONKUPIK"));
            next.append(entry);
        }
    }

    m_builtInPresets = next;
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

                QVariantMap entry = describePreset(
                    bytes,
                    QString(),
                    info.fileName(),
                    info.absoluteFilePath(),
                    QStringLiteral("folder"),
                    index++);
                entry.insert(QStringLiteral("description"), QStringLiteral("User local preset"));
                entry.insert(QStringLiteral("originLabel"), QStringLiteral("LOCAL"));
                next.append(entry);
            }
        }
    }

    m_folderPresets = next;
    rebuildCombinedPresets();
    emit libraryChanged();
}

void K500PresetFileBridge::rebuildCombinedPresets()
{
    QVariantList next = m_builtInPresets;
    for (const QVariant &preset : m_folderPresets)
        next.append(preset);
    m_combinedPresets = next;
}

bool K500PresetFileBridge::loadValidatedBytes(const QByteArray &bytes,
                                              const QString &sourcePath,
                                              const QString &sourceName)
{
    setError({});

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

    const bool trackingChanged = m_editTracking;
    m_editTracking = false;
    m_sourceBytes = bytes;
    m_savedBytes = bytes;
    m_sourcePath = sourcePath;
    m_sourceName = sourceName;
    refreshDocumentMetadata();

    if (trackingChanged)
        emit editTrackingChanged();
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
        setError(QStringLiteral("Official preset index tidak valid."));
        return false;
    }

    const QVariantMap entry = m_builtInPresets.at(index).toMap();
    const QString path = entry.value(QStringLiteral("path")).toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QStringLiteral("Official preset tidak ditemukan: %1")
                     .arg(entry.value(QStringLiteral("fileName")).toString()));
        return false;
    }

    const QString sourcePath = path.startsWith(QStringLiteral(":/"))
        ? QStringLiteral("official://%1").arg(entry.value(QStringLiteral("fileName")).toString())
        : path;
    return loadValidatedBytes(
        file.readAll(),
        sourcePath,
        entry.value(QStringLiteral("fileName")).toString());
}

void K500PresetFileBridge::onEngineEdit(const QString &path, const QVariant &value)
{
    if (!m_editTracking || !loaded())
        return;

    const auto edit = K500PresetEditMapper::applyEngineEdit(m_sourceBytes, path, value);
    if (!edit.supported)
        return;

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
    const bool trackingChanged = m_editTracking;
    m_editTracking = false;
    if (m_sourceBytes.isEmpty() && m_sourcePath.isEmpty() && m_presetName.isEmpty()) {
        if (trackingChanged) {
            emit editTrackingChanged();
            emit sourceChanged();
        }
        return;
    }
    m_sourceBytes.clear();
    m_savedBytes.clear();
    m_sourcePath.clear();
    m_sourceName.clear();
    m_presetName.clear();
    m_checksumOk = false;
    setError({});
    if (trackingChanged)
        emit editTrackingChanged();
    emit sourceChanged();
}

QByteArray K500PresetFileBridge::deviceSlotImage() const
{
    if (!loaded() || !K500PresetCodec::validateChecksum(m_sourceBytes))
        return {};
    return K500PresetCodec::buildDeviceSlotImage(m_sourceBytes);
}

bool K500PresetFileBridge::previewLoadedPreset()
{
    setError({});
    if (!m_engine) {
        setError(QStringLiteral("Editor belum tersedia untuk offline preview."));
        return false;
    }
    if (!loaded() || !K500PresetCodec::validateChecksum(m_sourceBytes)) {
        setError(QStringLiteral("Belum ada preset .k500 valid untuk dipreview."));
        return false;
    }

    QString slotError;
    const QByteArray slot = K500PresetCodec::buildDeviceSlotImage(m_sourceBytes, &slotError);
    if (slot.size() != K500PresetCodec::DeviceSlotImageLength) {
        setError(slotError.isEmpty() ? QStringLiteral("Gagal membentuk image offline preview 0x0290.") : slotError);
        return false;
    }

    QByteArray preview(ActiveMemorySize, char(0));
    std::copy(slot.cbegin(), slot.cend(), preview.begin());
    m_engine->hydrateFromDeviceMemory(preview);
    setEditTracking(true);
    return true;
}
