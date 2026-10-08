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
    {"KONSER NYANYI",     "Balanced, punchy and forgiving concert karaoke",       "01_KONSER_NYANYI.k500",       ":/presets/01_KONSER_NYANYI.k500"},
    {"MC HOST RADIO",     "Dry and controlled broadcast utility",             "02_MC_HOST_RADIO.k500",       ":/presets/02_MC_HOST_RADIO.k500"},
    {"KAR DANGDUT",       "Punchy dangdut groove with smooth vocal space",        "03_KAR_DANGDUT.k500", ":/presets/03_KAR_DANGDUT.k500"},
    {"POP ROCK BALLAD",   "Pop/slow-rock ambience with singer control",       "04_POP_ROCK_BALLAD.k500",            ":/presets/04_POP_ROCK_BALLAD.k500"},
    {"POP KENANGAN V2",   "Tempo-aware romantic slow-pop with lush vocal halo",                   "05_POP_KENANGAN_V2.k500",    ":/presets/05_POP_KENANGAN_V2.k500"},
    {"SHOLAWAT SYAHDU",   "Fresh soft slow-tempo sholawat ambience",          "06_SHOLAWAT_SYAHDU.k500",   ":/presets/06_SHOLAWAT_SYAHDU.k500"},
    {"JAZZ LOUNGE",       "Intimate classy lounge vocal with restrained echo","07_JAZZ_LOUNGE.k500",            ":/presets/07_JAZZ_LOUNGE.k500"},
    {"BLUES CLUB",        "Warm vintage blues with a single slap repeat",     "08_BLUES_CLUB.k500",           ":/presets/08_BLUES_CLUB.k500"},
    {"ACOUSTIC NATURAL",  "Natural organic vocal with subtle ambience",       "09_ACOUSTIC_NATURAL.k500",        ":/presets/09_ACOUSTIC_NATURAL.k500"},
    {"REGGAE DUB",        "Bass/groove-led voicing with rhythmic echo",       "10_REGGAE_DUB.k500",          ":/presets/10_REGGAE_DUB.k500"},

    // UAUDIO_COMPANION_PRESET_BANK_V1 — deterministic 01–10 counterparts.
    // 11–20 are PC library options; physical K500 hardware still has 10 slots.
    {"KONSER UAUDIO", "KONSER NYANYI tuning · USB Audio music source", "11_KONSER_NYANYI_UAUDIO.k500", ":/presets/11_KONSER_NYANYI_UAUDIO.k500"},
    {"MC HOST UAUDIO", "MC HOST RADIO tuning · USB Audio music source", "12_MC_HOST_RADIO_UAUDIO.k500", ":/presets/12_MC_HOST_RADIO_UAUDIO.k500"},
    {"DANGDUT UAUDIO", "KAR DANGDUT tuning · USB Audio music source", "13_KAR_DANGDUT_UAUDIO.k500", ":/presets/13_KAR_DANGDUT_UAUDIO.k500"},
    {"POP ROCK UAUDIO", "POP ROCK BALLAD tuning · USB Audio music source", "14_POP_ROCK_BALLAD_UAUDIO.k500", ":/presets/14_POP_ROCK_BALLAD_UAUDIO.k500"},
    {"KENANGAN UAUDIO", "POP KENANGAN V2 tuning · USB Audio music source", "15_POP_KENANGAN_V2_UAUDIO.k500", ":/presets/15_POP_KENANGAN_V2_UAUDIO.k500"},
    {"SHOLAWAT UAUDIO", "SHOLAWAT SYAHDU tuning · USB Audio music source", "16_SHOLAWAT_SYAHDU_UAUDIO.k500", ":/presets/16_SHOLAWAT_SYAHDU_UAUDIO.k500"},
    {"JAZZ UAUDIO", "JAZZ LOUNGE tuning · USB Audio music source", "17_JAZZ_LOUNGE_UAUDIO.k500", ":/presets/17_JAZZ_LOUNGE_UAUDIO.k500"},
    {"BLUES UAUDIO", "BLUES CLUB tuning · USB Audio music source", "18_BLUES_CLUB_UAUDIO.k500", ":/presets/18_BLUES_CLUB_UAUDIO.k500"},
    {"ACOUSTIC UAUDIO", "ACOUSTIC NATURAL tuning · USB Audio music source", "19_ACOUSTIC_NATURAL_UAUDIO.k500", ":/presets/19_ACOUSTIC_NATURAL_UAUDIO.k500"},
    {"REGGAE UAUDIO", "REGGAE DUB tuning · USB Audio music source", "20_REGGAE_DUB_UAUDIO.k500", ":/presets/20_REGGAE_DUB_UAUDIO.k500"},
};

bool readValidPreset(const QString &path, QByteArray *bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray candidate = file.readAll();
    const K500PresetCodec::Document document(candidate);
    if (!document.validSize() || !document.checksumOk())
        return false;
    QString compatibilityError;
    if (!K500PresetCodec::validateDeviceSlotCompatibility(candidate, &compatibilityError))
        return false;
    if (bytes)
        *bytes = candidate;
    return true;
}

QString crossoverFilterLabel(quint16 raw, bool hpf)
{
    const quint16 expectedFamily = hpf ? 0x0400 : 0x0300;
    if ((raw & 0xFF00) != expectedFamily)
        return {};

    const char *shape = nullptr;
    switch (raw & 0x00FF) {
    case 0x01: shape = "Bessel 12"; break;
    case 0x02: shape = "Butter 12"; break;
    case 0x03: shape = "Bessel 18"; break;
    case 0x04: shape = "Butter 18"; break;
    case 0x05: shape = "Bessel 24"; break;
    case 0x06: shape = "Butter 24"; break;
    case 0x07: shape = "LR 24"; break;
    default: return {};
    }
    return QStringLiteral("%1 %2")
        .arg(hpf ? QStringLiteral("HP") : QStringLiteral("LP"), QString::fromLatin1(shape));
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
        // P3_4_CONTROLLED_EDIT_PERSISTENCE_V1
        // Controlled persistence exists only when editTracking is explicitly
        // enabled by offline Preview. Connected LIVE sessions force it off.
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
    QString compatibilityError;
    const bool deviceCompatible = checksumOk
        && K500PresetCodec::validateDeviceSlotCompatibility(bytes, &compatibilityError);

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
    entry.insert(QStringLiteral("deviceCompatible"), deviceCompatible);
    entry.insert(QStringLiteral("compatibilityError"), compatibilityError);
    entry.insert(QStringLiteral("valid"), sizeOk && checksumOk && deviceCompatible);
    return entry;
}

void K500PresetFileBridge::rebuildBuiltInPresets()
{
    QVariantList next;
    const QDir cacheDir(officialCacheDirectory());
    QStringList knownNames;
    int index = 0;

    // P6_PC_PRESET_LIBRARY_V1 — bundled files remain the guaranteed offline
    // bank; checksum-valid official cache overrides/additions are PC-side only.
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

        // OFFICIAL_CACHE_IDENTITY_TRUTH_V1
        // The exact validated bytes that won (cache or bundled fallback) own
        // the visible preset identity. A cache/catalog mismatch is not enough
        // evidence to call either side stale, so never promise that another
        // sync will change it without explicit provenance.
        QVariantMap entry = describePreset(
            bytes,
            QString(),
            fileName,
            path,
            source,
            index++);
        entry.insert(QStringLiteral("description"), QString::fromLatin1(definition.description));
        entry.insert(QStringLiteral("originLabel"), QStringLiteral("SONKUPIK"));
        next.append(entry);
    }

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

    // DEVICE_TRUTH_STAGING_V1
    // Every newly selected file starts staging-only. It does not hydrate the
    // StudioEngine and it resets any prior offline edit session.
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

bool K500PresetFileBridge::persistMappedEdit(const QString &path,
                                                   const QVariant &value,
                                                   bool rejectUnsupported)
{
    if (!m_editTracking || !loaded())
        return false;

    const auto edit = K500PresetEditMapper::applyEngineEdit(m_sourceBytes, path, value);
    if (!edit.supported) {
        if (rejectUnsupported) {
            const QString reason = QStringLiteral("Edit path belum memiliki mapping file yang terverifikasi.");
            setError(QStringLiteral("%1: %2").arg(path, reason));
            emit persistenceRejected(path, reason);
        }
        return false;
    }

    if (!edit.patch.ok) {
        const QString reason = edit.patch.error.isEmpty()
            ? QStringLiteral("Edit tidak lolos whitelist serializer.")
            : edit.patch.error;
        setError(QStringLiteral("%1: %2").arg(path, reason));
        emit persistenceRejected(path, reason);
        return false;
    }

    if (edit.patch.bytes == m_sourceBytes) {
        setError({});
        return true;
    }
    if (!K500PresetCodec::validateChecksum(edit.patch.bytes)) {
        const QString reason = QStringLiteral("Serializer menghasilkan checksum tidak valid; perubahan ditolak.");
        setError(reason);
        emit persistenceRejected(path, reason);
        return false;
    }

    QString compatibilityError;
    if (!K500PresetCodec::validateDeviceSlotCompatibility(edit.patch.bytes, &compatibilityError)) {
        const QString reason = QStringLiteral("Edit membuat preset tidak dapat direpresentasikan persis oleh hardware: %1")
            .arg(compatibilityError);
        setError(QStringLiteral("%1: %2").arg(path, reason));
        emit persistenceRejected(path, reason);
        return false;
    }

    m_sourceBytes = edit.patch.bytes;
    refreshDocumentMetadata();
    setError({});
    emit sourceChanged();
    emit persistedEdit(path, edit.patch.changedOffsets.size());
    return true;
}

void K500PresetFileBridge::onEngineEdit(const QString &path, const QVariant &value)
{
    if (!m_editTracking || !loaded())
        return;
    persistMappedEdit(path, value, false);
}

bool K500PresetFileBridge::setOfflineAdjMannerVrOff(bool enabled)
{
    // Never reinterpret this as a deferred hardware command. It mutates only
    // the currently previewed working .k500 document.
    if (!editPersistenceEnabled()) {
        const QString path = QStringLiteral("system.adjMannerVrOff");
        const QString reason = QStringLiteral("Preview preset terlebih dahulu untuk mengedit file offline.");
        setError(reason);
        emit persistenceRejected(path, reason);
        return false;
    }
    const bool persisted = persistMappedEdit(
        QStringLiteral("system.adjMannerVrOff"), enabled, true);
    if (persisted && m_engine)
        m_engine->syncAdjMannerVrOff(enabled);
    return persisted;
}

bool K500PresetFileBridge::saveFile(const QUrl &url)
{
    setError({});
    if (!loaded() || !K500PresetCodec::validateChecksum(m_sourceBytes)) {
        setError(QStringLiteral("Belum ada working .k500 valid untuk diekspor."));
        return false;
    }
    QString compatibilityError;
    if (!K500PresetCodec::validateDeviceSlotCompatibility(m_sourceBytes, &compatibilityError)) {
        setError(QStringLiteral("Preset belum aman untuk hardware dan tidak diekspor: %1")
                     .arg(compatibilityError));
        return false;
    }

    const QString path = localPath(url, true);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(QStringLiteral("Tidak dapat membuat file preset: %1").arg(path));
        return false;
    }

    // P3_4_EDITED_EXPORT_V1
    // No-op Save As is still byte-identical. Once verified offline edits exist,
    // only mapper-whitelisted bytes plus the checksum differ from the checkpoint.
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

    // OFFLINE_PREVIEW_V1
    // Only verified 0x0290 audio/settings data is hydrated. Hardware-only mode
    // names/BT metadata/active slot are never synthesized from a PC preset.
    QByteArray preview(ActiveMemorySize, char(0));
    std::copy(slot.cbegin(), slot.cend(), preview.begin());
    m_engine->hydrateFromDeviceMemory(preview);

    // PRESET_PREVIEW_CROSSOVER_OVERLAY_V1
    // The native 0x0290 slot image intentionally contains PEQ bands/scalars but
    // not the .k500 crossover TYPE footers. Overlay the full-file authoritative
    // HPF/LPF type + anchor metadata after scalar hydration. Unknown raw type
    // families are deliberately left unchanged rather than guessed.
    const K500PresetCodec::Document document(m_sourceBytes);
    for (const K500PresetCodec::EqSection &section : document.eqSections()) {
        if (section.key.endsWith(QStringLiteral("Alt")))
            continue;
        m_engine->syncPresetCrossover(
            section.key,
            section.crossover.hpfHz,
            section.crossover.lpfHz,
            crossoverFilterLabel(section.crossover.hpTypeRaw, true),
            crossoverFilterLabel(section.crossover.lpTypeRaw, false));
    }

    // P3_4_OFFLINE_EDIT_SESSION_V1
    // Explicit Preview is the opt-in boundary for controlled offline editing.
    setEditTracking(true);
    return true;
}