#include "K500PresetFileBridge.h"

#include "K500PresetCodec.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

namespace {
const QUrl OfficialCatalogUrl(QStringLiteral(
    "https://api.github.com/repos/masarray/k500/contents/resources/presets?ref=main"));

QNetworkRequest makeRequest(const QUrl &url, bool githubApi)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "SonKuPik-K500/0.6 official-preset-sync");
    if (githubApi) {
        request.setRawHeader("Accept", "application/vnd.github+json");
        request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    }
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

bool validPresetBytes(const QByteArray &bytes)
{
    const K500PresetCodec::Document document(bytes);
    return document.validSize() && document.checksumOk();
}

QString safeRemoteName(const QString &name)
{
    const QString fileName = QFileInfo(name).fileName();
    if (fileName != name || !fileName.endsWith(QStringLiteral(".k500"), Qt::CaseInsensitive))
        return {};
    return fileName;
}

bool trustedDownloadUrl(const QUrl &url)
{
    return url.isValid()
        && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && url.host().compare(QStringLiteral("raw.githubusercontent.com"), Qt::CaseInsensitive) == 0
        && url.path().startsWith(QStringLiteral("/masarray/k500/"));
}

QString sha256Hex(const QByteArray &bytes)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}
}

QString K500PresetFileBridge::officialCacheDirectory() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/.sonkupik-k500");
    const QString path = QDir(base).filePath(QStringLiteral("official-presets"));
    QDir().mkpath(path);
    return QDir::cleanPath(path);
}

void K500PresetFileBridge::setOfficialSyncState(bool busy,
                                                 const QString &status,
                                                 const QString &error)
{
    const bool changed = m_officialSyncBusy != busy
        || m_officialSyncStatus != status
        || m_officialSyncError != error;
    m_officialSyncBusy = busy;
    m_officialSyncStatus = status;
    m_officialSyncError = error;
    if (changed)
        emit officialSyncChanged();
}

void K500PresetFileBridge::syncOfficialPresets()
{
    if (m_officialSyncBusy)
        return;

    if (!m_networkManager)
        m_networkManager = new QNetworkAccessManager(this);

    m_officialUpdateCount = 0;
    m_officialSyncTotal = 0;
    m_officialDownloadQueue.clear();
    m_officialRemoteNames.clear();
    setOfficialSyncState(true, QStringLiteral("Checking SonKuPik preset updates…"));

    QNetworkReply *reply = m_networkManager->get(makeRequest(OfficialCatalogUrl, true));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            const QString reason = reply->errorString();
            reply->deleteLater();
            setOfficialSyncState(false,
                QStringLiteral("Offline · using bundled/cached SonKuPik presets"), reason);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
            reply->deleteLater();
            setOfficialSyncState(false,
                QStringLiteral("Using bundled/cached SonKuPik presets"),
                QStringLiteral("Official preset catalog response is invalid."));
            return;
        }

        QVariantList queue;
        QStringList remoteNames;
        QSettings settings;
        const QDir cacheDir(officialCacheDirectory());

        for (const QJsonValue &value : document.array()) {
            if (!value.isObject())
                continue;
            const QJsonObject object = value.toObject();
            if (object.value(QStringLiteral("type")).toString() != QStringLiteral("file"))
                continue;

            const QString name = safeRemoteName(object.value(QStringLiteral("name")).toString());
            const QString gitSha = object.value(QStringLiteral("sha")).toString();
            const QUrl downloadUrl(object.value(QStringLiteral("download_url")).toString());
            if (name.isEmpty() || gitSha.isEmpty() || !trustedDownloadUrl(downloadUrl))
                continue;

            remoteNames.append(name);
            const QString cachePath = cacheDir.filePath(name);
            QByteArray existing;
            QFile cacheFile(cachePath);
            if (cacheFile.open(QIODevice::ReadOnly))
                existing = cacheFile.readAll();

            const QString key = QStringLiteral("officialPresetLibrary/gitSha/%1").arg(name);
            const bool unchanged = validPresetBytes(existing)
                && settings.value(key).toString() == gitSha;
            if (!unchanged) {
                QVariantMap item;
                item.insert(QStringLiteral("name"), name);
                item.insert(QStringLiteral("gitSha"), gitSha);
                item.insert(QStringLiteral("url"), downloadUrl);
                queue.append(item);
            }
        }

        if (remoteNames.isEmpty()) {
            reply->deleteLater();
            setOfficialSyncState(false,
                QStringLiteral("Using bundled/cached SonKuPik presets"),
                QStringLiteral("No valid .k500 files were found in the official GitHub catalog."));
            return;
        }

        // Only after a complete valid directory listing do we honor remote removals.
        // A transient network/API failure therefore never destroys last-known-good data.
        const QStringList previousNames = settings.value(
            QStringLiteral("officialPresetLibrary/remoteNames")).toStringList();
        for (const QString &oldName : previousNames) {
            if (!remoteNames.contains(oldName, Qt::CaseInsensitive)) {
                QFile::remove(cacheDir.filePath(oldName));
                settings.remove(QStringLiteral("officialPresetLibrary/gitSha/%1").arg(oldName));
                settings.remove(QStringLiteral("officialPresetLibrary/sha256/%1").arg(oldName));
            }
        }
        settings.setValue(QStringLiteral("officialPresetLibrary/remoteNames"), remoteNames);

        m_officialRemoteNames = remoteNames;
        m_officialDownloadQueue = queue;
        m_officialSyncTotal = queue.size();
        reply->deleteLater();

        if (m_officialDownloadQueue.isEmpty()) {
            finishOfficialSync();
            return;
        }

        setOfficialSyncState(true,
            QStringLiteral("Updating SonKuPik presets · 0/%1").arg(m_officialSyncTotal));
        downloadNextOfficialPreset();
    });
}

void K500PresetFileBridge::downloadNextOfficialPreset()
{
    if (m_officialDownloadQueue.isEmpty()) {
        finishOfficialSync();
        return;
    }

    const QVariantMap item = m_officialDownloadQueue.takeFirst().toMap();
    const QString name = item.value(QStringLiteral("name")).toString();
    const QString gitSha = item.value(QStringLiteral("gitSha")).toString();
    const QUrl url = item.value(QStringLiteral("url")).toUrl();
    if (name.isEmpty() || gitSha.isEmpty() || !trustedDownloadUrl(url)) {
        m_officialSyncError = QStringLiteral("Skipped an invalid official preset catalog entry.");
        downloadNextOfficialPreset();
        return;
    }

    QNetworkReply *reply = m_networkManager->get(makeRequest(url, false));
    connect(reply, &QNetworkReply::finished, this, [this, reply, name, gitSha] {
        const QByteArray bytes = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            m_officialSyncError = QStringLiteral("%1: %2").arg(name, reply->errorString());
        } else if (!validPresetBytes(bytes)) {
            m_officialSyncError = QStringLiteral(
                "%1 failed K500 size/checksum validation; cached copy was kept.").arg(name);
        } else {
            const QString path = QDir(officialCacheDirectory()).filePath(name);
            QSaveFile file(path);
            if (!file.open(QIODevice::WriteOnly)
                || file.write(bytes) != bytes.size()
                || !file.commit()) {
                m_officialSyncError = QStringLiteral("Could not update official preset cache: %1").arg(name);
            } else {
                QSettings settings;
                settings.setValue(QStringLiteral("officialPresetLibrary/gitSha/%1").arg(name), gitSha);
                settings.setValue(QStringLiteral("officialPresetLibrary/sha256/%1").arg(name), sha256Hex(bytes));
                ++m_officialUpdateCount;
            }
        }

        reply->deleteLater();
        const int completed = m_officialSyncTotal - m_officialDownloadQueue.size();
        setOfficialSyncState(true,
            QStringLiteral("Updating SonKuPik presets · %1/%2")
                .arg(completed).arg(m_officialSyncTotal),
            m_officialSyncError);
        downloadNextOfficialPreset();
    });
}

void K500PresetFileBridge::finishOfficialSync()
{
    rebuildBuiltInPresets();
    rebuildCombinedPresets();
    emit libraryChanged();

    const int officialCount = m_builtInPresets.size();
    QString status;
    if (!m_officialSyncError.isEmpty()) {
        status = QStringLiteral("Preset sync completed with fallback · %1 official ready")
            .arg(officialCount);
    } else if (m_officialUpdateCount > 0) {
        status = QStringLiteral("Updated %1 preset(s) · %2 official ready")
            .arg(m_officialUpdateCount).arg(officialCount);
    } else {
        status = QStringLiteral("Preset library up to date · %1 official")
            .arg(officialCount);
    }

    m_officialDownloadQueue.clear();
    m_officialSyncTotal = 0;
    setOfficialSyncState(false, status, m_officialSyncError);
}
