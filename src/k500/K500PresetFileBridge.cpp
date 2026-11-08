#include "K500PresetFileBridge.h"

#include "K500PresetCodec.h"
#include "K500PresetEditMapper.h"
#include "../StudioEngine.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
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
    // FOCUS_BT_PRIMARY_LIBRARY_V1 · 51 PC entries; GOLDEN #41 immutable.
    // UAUDIO_COMPANION_PRESET_BANK_V1 · 31–40 regenerated from Gen3 BT 21–30.
    {"FOCUS ARTIST [BT]", "Owner-accepted R4 singing + Music R6 Bluetooth", "01_FOCUS_ARTIST_BT.k500", ":/presets/01_FOCUS_ARTIST_BT.k500"},
    {"FOCUS MC RADIO [BT]", "Owner-accepted MC vocal + Music R6 Bluetooth", "02_FOCUS_MC_BT.k500", ":/presets/02_FOCUS_MC_BT.k500"},
    {"FOCUS DANGDUT [BT]", "FOCUS A research BT · owner-accepted bank", "03_FOCUS_DANGDUT_BT.k500", ":/presets/03_FOCUS_DANGDUT_BT.k500"},
    {"FOCUS ROCK [BT]", "FOCUS A research BT · owner-accepted bank", "04_FOCUS_ROCK_BT.k500", ":/presets/04_FOCUS_ROCK_BT.k500"},
    {"FOCUS ACOUSTIC [BT]", "FOCUS A research BT · owner-accepted bank", "05_FOCUS_ACOUSTIC_BT.k500", ":/presets/05_FOCUS_ACOUSTIC_BT.k500"},
    {"FOCUS JAZZ [BT]", "FOCUS A research BT · owner-accepted bank", "06_FOCUS_JAZZ_BT.k500", ":/presets/06_FOCUS_JAZZ_BT.k500"},
    {"FOCUS MELAYU [BT]", "FOCUS A research BT · owner-accepted bank", "07_FOCUS_MELAYU_BT.k500", ":/presets/07_FOCUS_MELAYU_BT.k500"},
    {"FOCUS QORI [BT]", "FOCUS A research BT · owner-accepted bank", "08_FOCUS_QORI_BT.k500", ":/presets/08_FOCUS_QORI_BT.k500"},
    {"FOCUS ADZAN [BT]", "FOCUS A research BT · owner-accepted bank", "09_FOCUS_ADZAN_BT.k500", ":/presets/09_FOCUS_ADZAN_BT.k500"},
    {"FOCUS BALLAD [BT]", "FOCUS A research BT · owner-accepted bank", "10_FOCUS_BALLAD_BT.k500", ":/presets/10_FOCUS_BALLAD_BT.k500"},
    {"ARTIST G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "11_ARTIST_D2R.k500", ":/presets/11_ARTIST_D2R.k500"},
    {"MC HOST RADIO [UAUDIO]", "Owner-selected Velvet R1 · Podcast voice / UAUDIO / gentle room / echo off", "12_MC_HOST_RADIO_D2R.k500", ":/presets/12_MC_HOST_RADIO_D2R.k500"},
    {"DANGDUT G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "13_DANGDUT_D2R.k500", ":/presets/13_DANGDUT_D2R.k500"},
    {"ROCK G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "14_ROCK_D2R.k500", ":/presets/14_ROCK_D2R.k500"},
    {"ACOUSTIC G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "15_ACOUSTIC_D2R.k500", ":/presets/15_ACOUSTIC_D2R.k500"},
    {"JAZZ G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "16_JAZZ_D2R.k500", ":/presets/16_JAZZ_D2R.k500"},
    {"MELAYU G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "17_MELAYU_D2R.k500", ":/presets/17_MELAYU_D2R.k500"},
    {"QORI G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "18_QORI_D2R.k500", ":/presets/18_QORI_D2R.k500"},
    {"ADZAN G5 [UAUDIO]", "GEN5 D2R UAUDIO · original sonic tuning; source raw=5", "19_ADZAN_D2R.k500", ":/presets/19_ADZAN_D2R.k500"},
    {"BALLAD G5 [UAUDIO]", "GEN5 UAUDIO · melodic ballad singing (moved from old 02)", "20_BALLAD_D2R.k500", ":/presets/20_BALLAD_D2R.k500"},
    {"ARTIST GEN3 ARI [BT]", "Archived GEN3 Bluetooth · original bytes", "21_ARTIST_GEN3_ARI.k500", ":/presets/21_ARTIST_GEN3_ARI.k500"},
    {"PODCAST REBORN [BT]", "Archived GEN3 Bluetooth · original bytes", "22_PODCAST_REBORN.k500", ":/presets/22_PODCAST_REBORN.k500"},
    {"DANGDUT GEN3 ARI [BT]", "Archived GEN3 Bluetooth · original bytes", "23_DANGDUT_GEN3_ARI.k500", ":/presets/23_DANGDUT_GEN3_ARI.k500"},
    {"KARAOKE ARTIST [BT]", "Archived GEN3 Bluetooth · original bytes", "24_KARAOKE_ARTIST.k500", ":/presets/24_KARAOKE_ARTIST.k500"},
    {"AKUSTIK GEN3 ARI [BT]", "Archived GEN3 Bluetooth · original bytes", "25_AKUSTIK_GEN3_ARI.k500", ":/presets/25_AKUSTIK_GEN3_ARI.k500"},
    {"IMAM QORI GEN 3 [BT]", "Archived GEN3 Bluetooth · original bytes", "26_IMAM_QORI_GEN3.k500", ":/presets/26_IMAM_QORI_GEN3.k500"},
    {"JAZZ GEN3 ARI [BT]", "Archived GEN3 Bluetooth · original bytes", "27_JAZZ_GEN3_ARI.k500", ":/presets/27_JAZZ_GEN3_ARI.k500"},
    {"ROCK GEN3 ARI [BT]", "Archived GEN3 Bluetooth · original bytes", "28_ROCK_GEN3_ARI.k500", ":/presets/28_ROCK_GEN3_ARI.k500"},
    {"MC CERAMAH GEN 3 [BT]", "Archived GEN3 Bluetooth · original bytes", "29_MC_CERAMAH_GEN3.k500", ":/presets/29_MC_CERAMAH_GEN3.k500"},
    {"ADZAN MEKAH GEN3 [BT]", "Archived GEN3 Bluetooth · original bytes", "30_ADZAN_MEKAH_GEN3.k500", ":/presets/30_ADZAN_MEKAH_GEN3.k500"},
    {"ARTIST GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "31_ARTIST_GEN3_ARI_UAUDIO.k500", ":/presets/31_ARTIST_GEN3_ARI_UAUDIO.k500"},
    {"PODCAST GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "32_PODCAST_REBORN_UAUDIO.k500", ":/presets/32_PODCAST_REBORN_UAUDIO.k500"},
    {"DANGDUT GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "33_DANGDUT_GEN3_ARI_UAUDIO.k500", ":/presets/33_DANGDUT_GEN3_ARI_UAUDIO.k500"},
    {"KARAOKE GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "34_KARAOKE_ARTIST_UAUDIO.k500", ":/presets/34_KARAOKE_ARTIST_UAUDIO.k500"},
    {"AKUSTIK GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "35_AKUSTIK_GEN3_ARI_UAUDIO.k500", ":/presets/35_AKUSTIK_GEN3_ARI_UAUDIO.k500"},
    {"QORI GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "36_IMAM_QORI_GEN3_UAUDIO.k500", ":/presets/36_IMAM_QORI_GEN3_UAUDIO.k500"},
    {"JAZZ GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "37_JAZZ_GEN3_ARI_UAUDIO.k500", ":/presets/37_JAZZ_GEN3_ARI_UAUDIO.k500"},
    {"ROCK GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "38_ROCK_GEN3_ARI_UAUDIO.k500", ":/presets/38_ROCK_GEN3_ARI_UAUDIO.k500"},
    {"CERAMAH GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "39_MC_CERAMAH_GEN3_UAUDIO.k500", ":/presets/39_MC_CERAMAH_GEN3_UAUDIO.k500"},
    {"ADZAN GEN3 [UAUDIO]", "Archived GEN3 USB Audio · generated from 11–20", "40_ADZAN_MEKAH_GEN3_UAUDIO.k500", ":/presets/40_ADZAN_MEKAH_GEN3_UAUDIO.k500"},
    {"D2R GOLDEN [BT]", "OWNER GOLDEN • exact original D2R · never overwrite", "41_GOLDEN_D2R.k500", ":/presets/41_GOLDEN_D2R.k500"},
    {"KONSER NYANYI [BT]", "Historical SonKuPik preset · original bytes", "42_KONSER_NYANYI.k500", ":/presets/42_KONSER_NYANYI.k500"},
    {"MC HOST RADIO [BT]", "Historical SonKuPik preset · original bytes", "43_MC_HOST_RADIO.k500", ":/presets/43_MC_HOST_RADIO.k500"},
    {"KAR DANGDUT [BT]", "Historical SonKuPik preset · original bytes", "44_KAR_DANGDUT.k500", ":/presets/44_KAR_DANGDUT.k500"},
    {"POP ROCK BALLAD [BT]", "Historical SonKuPik preset · original bytes", "45_POP_ROCK_BALLAD.k500", ":/presets/45_POP_ROCK_BALLAD.k500"},
    {"POP KENANGAN V2 [BT]", "Historical SonKuPik preset · original bytes", "46_POP_KENANGAN_V2.k500", ":/presets/46_POP_KENANGAN_V2.k500"},
    {"SHOLAWAT SYAHDU [BT]", "Historical SonKuPik preset · original bytes", "47_SHOLAWAT_SYAHDU.k500", ":/presets/47_SHOLAWAT_SYAHDU.k500"},
    {"JAZZ LOUNGE [BT]", "Historical SonKuPik preset · original bytes", "48_JAZZ_LOUNGE.k500", ":/presets/48_JAZZ_LOUNGE.k500"},
    {"BLUES CLUB [BT]", "Historical SonKuPik preset · original bytes", "49_BLUES_CLUB.k500", ":/presets/49_BLUES_CLUB.k500"},
    {"ACOUSTIC NATURAL [BT]", "Historical SonKuPik preset · original bytes", "50_ACOUSTIC_NATURAL.k500", ":/presets/50_ACOUSTIC_NATURAL.k500"},
    {"REGGAE DUB [BT]", "Historical SonKuPik preset · original bytes", "51_REGGAE_DUB.k500", ":/presets/51_REGGAE_DUB.k500"},
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

    // LOCAL_PRESET_LEGACY_FOLDER_DISCOVERY_V1 — older SonKuPik installations
    // used Documents/SONKUPIK STUDIO Presets, while later builds remembered
    // Documents/SonKuPikK500/Presets. A stale *empty* remembered directory
    // must not hide real user presets that already exist in the legacy folder.
    const QString rememberedFolder = QSettings().value(
        QStringLiteral("pcPresetLibrary/folder")).toString();
    QStringList candidates;
    if (!rememberedFolder.isEmpty())
        candidates << QDir::cleanPath(rememberedFolder);
    const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documents.isEmpty())
        candidates << QDir(documents).filePath(QStringLiteral("SONKUPIK STUDIO Presets"));
    if (!rememberedFolder.isEmpty())
        candidates << QDir(rememberedFolder).filePath(QStringLiteral("../../SONKUPIK STUDIO Presets"));
    candidates << QDir::home().filePath(QStringLiteral("Documents/SONKUPIK STUDIO Presets"));

    // Only specific, expected folder names are considered; never crawl the
    // full Documents tree or import arbitrary files outside the user directory.
    for (const QString &candidate : std::as_const(candidates)) {
        const QDir folder(QDir::cleanPath(candidate));
        if (!folder.exists())
            continue;
        if (!folder.entryList({QStringLiteral("*.k500")},
                              QDir::Files | QDir::Readable | QDir::NoSymLinks).isEmpty()) {
            m_presetFolder = folder.absolutePath();
            break;
        }
    }
    if (m_presetFolder.isEmpty() && !rememberedFolder.isEmpty()
        && QDir(rememberedFolder).exists())
        m_presetFolder = QDir(rememberedFolder).absolutePath();
    if (!m_presetFolder.isEmpty()) {
        if (QDir::cleanPath(rememberedFolder) != QDir::cleanPath(m_presetFolder))
            QSettings().setValue(QStringLiteral("pcPresetLibrary/folder"), m_presetFolder);
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

        // FOCUS_BT_PRIMARY_CACHE_GUARD_V1 · fail closed on stale same-named cache.
        const int catalogId = fileName.left(2).toInt();
        const bool focusPrimary = catalogId >= 1 && catalogId <= 10;
        const bool archivedUa = catalogId >= 11 && catalogId <= 20;
        static constexpr const char *FocusSha256[] = {
            "eb64073940b3907e15b0c77fc630baf8aef737fb3ab6980d5310d8b459604dd9",
            "4c8937765af3e2c7bc441b25daf4674cfdc87fbbb94240809cfbb8e00dfc9bfe",
            "d76de172139c8c3641d176ee1693fe10c46f7626abce5234157d51e05b6ae8d7",
            "32d64cc38c606b645ce73782a3a0b32d505be2e3c03caa0aa6bc42e9e96fb014",
            "bbb37c515a989036bcf13e137a96cd9afa16c754173637ea0576f449ae1e50c8",
            "3c645deaf988003096ccd38213f808b69489ea937ac31735a83af08e963e8467",
            "315b750917e028d99472cf46ad50240a792904594ebe4e9e0c0ca237d3904030",
            "987578d889a945aea6646195fc931d22c34d3c475f7f1b863ca8038f913f0e25",
            "f0c1881102e22d98efde210b66e3d29428732f992e9b5bec9d6449ec3db4dd4c",
            "9c90d1e7de5ea838ae3bf3f321a6d9a170c15b8350deb142171e4fc2d83da9e8"
        };
        QByteArray bytes;
        const QString cachedPath = cacheDir.filePath(fileName);
        QString path = QString::fromLatin1(definition.resourcePath);
        QString source = QStringLiteral("official-bundled");
        const bool cacheValid = readValidPreset(cachedPath, &bytes);
        const int sourceRaw = bytes.size() > 0x0E ? static_cast<quint8>(bytes.at(0x0E)) : -1;
        const bool cacheModeOk = (!focusPrimary || sourceRaw == 2)
            && (!archivedUa || sourceRaw == 5);
        const QByteArray cacheSha = cacheValid
            ? QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex() : QByteArray();
        const bool focusHashOk = !focusPrimary ||
            (cacheValid && cacheSha == QByteArray(FocusSha256[catalogId - 1]));
        const bool ownerVelvet = fileName == QStringLiteral("12_MC_HOST_RADIO_D2R.k500");
        const bool velvetHashOk = !ownerVelvet ||
            (cacheValid && cacheSha == QByteArrayLiteral("af86797542e11d1079d1c2ee61000307469704db208daae10795b1e2dad7586e"));
        const bool cacheWinnerOk = focusHashOk && velvetHashOk;
        if (cacheValid && cacheModeOk && cacheWinnerOk) {
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
        // OFFICIAL_PRESET_VISIBLE_SOURCE_MODE_V1: mode suffix is a PC-only
        // display label, not the firmware's 16-char native preset name.
        QVariantMap entry = describePreset(
            bytes,
            QString::fromLatin1(definition.displayName),
            fileName,
            path,
            source,
            index++);
        // OFFICIAL_PRESET_CANONICAL_CATALOG_ID_V1 — identity belongs to the
        // canonical bundled filename, not ListView order or a mutable cache index.
        // Valid cache replacements inherit the ID of their matching official file.
        // Noncanonical official extras and LOCAL presets intentionally have no ID.
        bool catalogNumberOk = false;
        const int catalogNumber = fileName.left(2).toInt(&catalogNumberOk);
        if (catalogNumberOk && fileName.size() > 3
            && fileName.at(2) == QLatin1Char('_')
            && catalogNumber >= 1 && catalogNumber <= 51) {
            entry.insert(QStringLiteral("catalogNumber"), catalogNumber);
        }
        entry.insert(QStringLiteral("description"), QString::fromLatin1(definition.description));
        entry.insert(QStringLiteral("originLabel"), QStringLiteral("SONKUPIK"));
        next.append(entry);
    }

    // GEN5_ARCHIVE_CACHE_HIDE_V1: pre-migration cache copies may survive an offline
    // upgrade; archived originals are now bundled at 21–30. Do not show
    // duplicates, and do not delete user cache content without a valid sync.
    const QStringList oldNumberedCacheNames{
        // Hide prior official migration aliases; never delete user-local files.
        QStringLiteral("01_ARTIST_D2R.k500"),
        QStringLiteral("02_MC_HOST_RADIO_D2R.k500"),
        QStringLiteral("03_DANGDUT_D2R.k500"),
        QStringLiteral("04_ROCK_D2R.k500"),
        QStringLiteral("05_ACOUSTIC_D2R.k500"),
        QStringLiteral("06_JAZZ_D2R.k500"),
        QStringLiteral("07_MELAYU_D2R.k500"),
        QStringLiteral("08_QORI_D2R.k500"),
        QStringLiteral("09_ADZAN_D2R.k500"),
        QStringLiteral("10_BALLAD_D2R.k500"),
        QStringLiteral("11_ARTIST_GEN3_ARI.k500"),
        QStringLiteral("12_PODCAST_REBORN.k500"),
        QStringLiteral("13_DANGDUT_GEN3_ARI.k500"),
        QStringLiteral("14_KARAOKE_ARTIST.k500"),
        QStringLiteral("15_AKUSTIK_GEN3_ARI.k500"),
        QStringLiteral("16_IMAM_QORI_GEN3.k500"),
        QStringLiteral("17_JAZZ_GEN3_ARI.k500"),
        QStringLiteral("18_ROCK_GEN3_ARI.k500"),
        QStringLiteral("19_MC_CERAMAH_GEN3.k500"),
        QStringLiteral("20_ADZAN_MEKAH_GEN3.k500"),
        QStringLiteral("21_ARTIST_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("22_PODCAST_REBORN_UAUDIO.k500"),
        QStringLiteral("23_DANGDUT_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("24_KARAOKE_ARTIST_UAUDIO.k500"),
        QStringLiteral("25_AKUSTIK_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("26_IMAM_QORI_GEN3_UAUDIO.k500"),
        QStringLiteral("27_JAZZ_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("28_ROCK_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("29_MC_CERAMAH_GEN3_UAUDIO.k500"),
        QStringLiteral("30_ADZAN_MEKAH_GEN3_UAUDIO.k500"),
        QStringLiteral("31_KONSER_NYANYI.k500"),
        QStringLiteral("32_MC_HOST_RADIO.k500"),
        QStringLiteral("33_KAR_DANGDUT.k500"),
        QStringLiteral("34_POP_ROCK_BALLAD.k500"),
        QStringLiteral("35_POP_KENANGAN_V2.k500"),
        QStringLiteral("36_SHOLAWAT_SYAHDU.k500"),
        QStringLiteral("37_JAZZ_LOUNGE.k500"),
        QStringLiteral("38_BLUES_CLUB.k500"),
        QStringLiteral("39_ACOUSTIC_NATURAL.k500"),
        QStringLiteral("40_REGGAE_DUB.k500"),

        QStringLiteral("02_BALLAD_D2R.k500"),
        QStringLiteral("10_CERAMAH_D2R.k500"),
        QStringLiteral("01_ARTIST_GEN3_ARI.k500"),
        QStringLiteral("02_PODCAST_REBORN.k500"),
        QStringLiteral("03_DANGDUT_GEN3_ARI.k500"),
        QStringLiteral("04_KARAOKE_ARTIST.k500"),
        QStringLiteral("05_AKUSTIK_GEN3_ARI.k500"),
        QStringLiteral("06_IMAM_QORI_GEN3.k500"),
        QStringLiteral("07_JAZZ_GEN3_ARI.k500"),
        QStringLiteral("08_ROCK_GEN3_ARI.k500"),
        QStringLiteral("09_MC_CERAMAH_GEN3.k500"),
        QStringLiteral("10_ADZAN_MEKAH_GEN3.k500"),
        QStringLiteral("11_ARTIST_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("12_PODCAST_REBORN_UAUDIO.k500"),
        QStringLiteral("13_DANGDUT_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("14_KARAOKE_ARTIST_UAUDIO.k500"),
        QStringLiteral("15_AKUSTIK_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("16_IMAM_QORI_GEN3_UAUDIO.k500"),
        QStringLiteral("17_JAZZ_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("18_ROCK_GEN3_ARI_UAUDIO.k500"),
        QStringLiteral("19_MC_CERAMAH_GEN3_UAUDIO.k500"),
        QStringLiteral("20_ADZAN_MEKAH_GEN3_UAUDIO.k500"),
        QStringLiteral("21_KONSER_NYANYI.k500"),
        QStringLiteral("22_MC_HOST_RADIO.k500"),
        QStringLiteral("23_KAR_DANGDUT.k500"),
        QStringLiteral("24_POP_ROCK_BALLAD.k500"),
        QStringLiteral("25_POP_KENANGAN_V2.k500"),
        QStringLiteral("26_SHOLAWAT_SYAHDU.k500"),
        QStringLiteral("27_JAZZ_LOUNGE.k500"),
        QStringLiteral("28_BLUES_CLUB.k500"),
        QStringLiteral("29_ACOUSTIC_NATURAL.k500"),
        QStringLiteral("30_REGGAE_DUB.k500"),
        QStringLiteral("01_KONSER_NYANYI.k500"),
        QStringLiteral("02_MC_HOST_RADIO.k500"),
        QStringLiteral("03_KAR_DANGDUT.k500"),
        QStringLiteral("04_POP_ROCK_BALLAD.k500"),
        QStringLiteral("05_POP_KENANGAN_V2.k500"),
        QStringLiteral("06_SHOLAWAT_SYAHDU.k500"),
        QStringLiteral("07_JAZZ_LOUNGE.k500"),
        QStringLiteral("08_BLUES_CLUB.k500"),
        QStringLiteral("09_ACOUSTIC_NATURAL.k500"),
        QStringLiteral("10_REGGAE_DUB.k500"),
    };
    if (cacheDir.exists()) {
        const QFileInfoList files = cacheDir.entryInfoList(
            {QStringLiteral("*.k500")},
            QDir::Files | QDir::Readable | QDir::NoSymLinks,
            QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo &info : files) {
            if (knownNames.contains(info.fileName().toLower())
                || oldNumberedCacheNames.contains(info.fileName(), Qt::CaseInsensitive))
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

// SAVE_PC_AS_SMART_FOLDER_V1 — distinct from pcPresetLibrary/folder so a
// user's imported Local library never changes merely by opening Save PC As.
// Creating the default folder happens only after the user requests Save As.
QUrl K500PresetFileBridge::prepareSaveDirectory()
{
    const QString lastUsed = QSettings().value(
        QStringLiteral("pcPresetExport/lastFolder")).toString();
    if (!lastUsed.isEmpty()) {
        const QFileInfo previous(QDir::cleanPath(lastUsed));
        if (previous.isDir() && previous.isReadable() && previous.isWritable())
            return QUrl::fromLocalFile(previous.absoluteFilePath());
    }

    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documents.isEmpty())
        documents = QDir::homePath();
    const QString target = QDir(documents).filePath(
        QStringLiteral("SonKuPikK500/Presets"));
    if (!QDir(target).exists() && !QDir().mkpath(target)) {
        setError(QStringLiteral("Tidak dapat menyiapkan folder preset SonKuPik K500: %1")
                     .arg(target));
        return {};
    }
    return QUrl::fromLocalFile(QDir(target).absolutePath());
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

    // Only a successfully committed .k500 export becomes the next folder.
    // Failed or cancelled dialogs must not replace the last-known-good path.
    QSettings().setValue(QStringLiteral("pcPresetExport/lastFolder"),
                         QFileInfo(path).absolutePath());
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
    // OFFLINE_PREVIEW_NOT_HARDWARE_V1 — never overwrite last verified K500
    // mode names/values with the padded preview's all-zero hardware-only bytes.
    m_engine->hydrateFromPreviewMemory(preview);

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