#include "AppUpdateManager.h"

#include <QCoreApplication>
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
#include <QVersionNumber>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
const QUrl LatestReleaseUrl(QStringLiteral(
    "https://api.github.com/repos/masarray/k500/releases/latest"));

QByteArray normalizedTagVersion(const QString &tag)
{
    QString value = tag.trimmed();
    if (value.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        value.remove(0, 1);
    return value.toLatin1();
}

bool trustedGitHubAsset(const QUrl &url)
{
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0)
        return false;
    const QString host = url.host().toLower();
    return host == QStringLiteral("github.com")
        || host == QStringLiteral("objects.githubusercontent.com")
        || host == QStringLiteral("release-assets.githubusercontent.com");
}
}

AppUpdateManager::AppUpdateManager(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

QString AppUpdateManager::currentVersion() const
{
    return QCoreApplication::applicationVersion();
}

bool AppUpdateManager::busy() const
{
    return m_state == QStringLiteral("checking")
        || m_state == QStringLiteral("preparing")
        || m_state == QStringLiteral("downloading")
        || m_state == QStringLiteral("verifying")
        || m_state == QStringLiteral("installing");
}

void AppUpdateManager::setState(const QString &state, const QString &status, const QString &error)
{
    m_state = state;
    m_statusText = status;
    m_errorText = error;
    emit stateChanged();
}

void AppUpdateManager::setProgress(qreal value)
{
    value = qBound<qreal>(0.0, value, 1.0);
    if (qFuzzyCompare(m_progress, value))
        return;
    m_progress = value;
    emit progressChanged();
}

void AppUpdateManager::resetReleaseMetadata()
{
    m_latestVersion.clear();
    m_releaseNotes.clear();
    m_updateAvailable = false;
    m_setupAssetName.clear();
    m_setupAssetUrl = {};
    m_manifestUrl = {};
    m_checksumsUrl = {};
    m_expectedSha256.clear();
    setProgress(0.0);
    emit updateChanged();
}

QNetworkReply *AppUpdateManager::get(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArray("SonKuPik-K500/") + currentVersion().toUtf8());
    request.setRawHeader("Accept", "application/vnd.github+json, application/octet-stream;q=0.9, */*;q=0.8");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return m_network->get(request);
}

void AppUpdateManager::checkForUpdates(bool userInitiated)
{
    if (busy())
        return;

    resetReleaseMetadata();
    setState(QStringLiteral("checking"), QStringLiteral("Checking for updates…"));

    QNetworkReply *reply = get(LatestReleaseUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply, userInitiated] {
        const QByteArray payload = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        reply->deleteLater();

        if (error != QNetworkReply::NoError) {
            setState(userInitiated ? QStringLiteral("error") : QStringLiteral("idle"),
                     userInitiated ? QStringLiteral("Update check failed") : QString(),
                     userInitiated ? errorString : QString());
            return;
        }
        handleLatestRelease(payload, userInitiated);
    });
}

void AppUpdateManager::handleLatestRelease(const QByteArray &payload, bool userInitiated)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setState(userInitiated ? QStringLiteral("error") : QStringLiteral("idle"),
                 userInitiated ? QStringLiteral("Update response is invalid") : QString(),
                 userInitiated ? parseError.errorString() : QString());
        return;
    }

    const QJsonObject release = document.object();
    if (release.value(QStringLiteral("draft")).toBool()
        || release.value(QStringLiteral("prerelease")).toBool()) {
        setState(QStringLiteral("idle"));
        return;
    }

    const QString version = QString::fromLatin1(
        normalizedTagVersion(release.value(QStringLiteral("tag_name")).toString()));
    const QVersionNumber latest = QVersionNumber::fromString(version);
    const QVersionNumber current = QVersionNumber::fromString(currentVersion());
    if (latest.isNull() || current.isNull()) {
        setState(userInitiated ? QStringLiteral("error") : QStringLiteral("idle"),
                 userInitiated ? QStringLiteral("Version information is invalid") : QString());
        return;
    }

    if (QVersionNumber::compare(latest, current) <= 0) {
        m_latestVersion = version;
        emit updateChanged();
        setState(userInitiated ? QStringLiteral("up-to-date") : QStringLiteral("idle"),
                 userInitiated ? QStringLiteral("You already have the latest version") : QString());
        return;
    }

    QString setupName;
    QUrl setupUrl;
    QUrl manifestUrl;
    QUrl sumsUrl;
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
        if (!trustedGitHubAsset(url))
            continue;
        if (name.endsWith(QStringLiteral("-Windows-Setup.exe"), Qt::CaseInsensitive)) {
            setupName = name;
            setupUrl = url;
        } else if (name == QStringLiteral("release-manifest.json")) {
            manifestUrl = url;
        } else if (name == QStringLiteral("SHA256SUMS.txt")) {
            sumsUrl = url;
        }
    }

    if (setupName.isEmpty() || setupUrl.isEmpty() || manifestUrl.isEmpty() || sumsUrl.isEmpty()) {
        setState(userInitiated ? QStringLiteral("error") : QStringLiteral("idle"),
                 userInitiated ? QStringLiteral("Stable update package is incomplete") : QString(),
                 userInitiated ? QStringLiteral("Required setup/manifest/checksum assets were not found.") : QString());
        return;
    }

    const QString skipped = QSettings().value(QStringLiteral("updates/skippedVersion")).toString();
    if (!userInitiated && skipped == version) {
        setState(QStringLiteral("idle"));
        return;
    }

    m_latestVersion = version;
    m_releaseNotes = release.value(QStringLiteral("body")).toString().trimmed();
    m_setupAssetName = setupName;
    m_setupAssetUrl = setupUrl;
    m_manifestUrl = manifestUrl;
    m_checksumsUrl = sumsUrl;
    m_updateAvailable = true;
    emit updateChanged();
    setState(QStringLiteral("available"),
             QStringLiteral("SonKuPik K500 %1 is ready").arg(version));
}

void AppUpdateManager::remindLater()
{
    if (busy())
        return;
    setState(QStringLiteral("available"),
             QStringLiteral("Update postponed until the next launch"));
}

void AppUpdateManager::skipThisVersion()
{
    if (m_latestVersion.isEmpty() || busy())
        return;
    QSettings().setValue(QStringLiteral("updates/skippedVersion"), m_latestVersion);
    m_updateAvailable = false;
    emit updateChanged();
    setState(QStringLiteral("idle"));
}

QString AppUpdateManager::updateDirectory() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty())
        base = QDir::tempPath() + QStringLiteral("/SonKuPik-K500");
    const QString versionDir = m_latestVersion.isEmpty()
        ? QStringLiteral("pending") : QStringLiteral("v%1").arg(m_latestVersion);
    const QString path = QDir(base).filePath(QStringLiteral("updates/%1").arg(versionDir));
    QDir().mkpath(path);
    return QDir::cleanPath(path);
}

QString AppUpdateManager::installerPath() const
{
    return QDir(updateDirectory()).filePath(m_setupAssetName);
}

void AppUpdateManager::downloadAndInstall()
{
    if (!m_updateAvailable || busy())
        return;
    m_expectedSha256.clear();
    setProgress(0.0);
    setState(QStringLiteral("preparing"), QStringLiteral("Validating release metadata…"));
    downloadManifest();
}

void AppUpdateManager::downloadManifest()
{
    QNetworkReply *reply = get(m_manifestUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray payload = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        reply->deleteLater();
        if (error != QNetworkReply::NoError || !validateManifest(payload)) {
            setState(QStringLiteral("error"), QStringLiteral("Update verification failed"),
                     error != QNetworkReply::NoError ? errorString : m_errorText);
            return;
        }
        setProgress(0.08);
        downloadChecksums();
    });
}

bool AppUpdateManager::validateManifest(const QByteArray &payload)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        m_errorText = QStringLiteral("release-manifest.json is invalid.");
        return false;
    }
    const QJsonObject manifest = document.object();
    if (manifest.value(QStringLiteral("channel")).toString() != QStringLiteral("stable")
        || manifest.value(QStringLiteral("version")).toString() != m_latestVersion
        || !manifest.value(QStringLiteral("stableReleaseEligible")).toBool()
        || manifest.value(QStringLiteral("target")).toString() != QStringLiteral("windows-x64")) {
        m_errorText = QStringLiteral("Release manifest does not match this stable Windows update.");
        return false;
    }
    return true;
}

void AppUpdateManager::downloadChecksums()
{
    setState(QStringLiteral("preparing"), QStringLiteral("Checking release checksum…"));
    QNetworkReply *reply = get(m_checksumsUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray payload = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        reply->deleteLater();
        if (error != QNetworkReply::NoError || !parseExpectedChecksum(payload)) {
            setState(QStringLiteral("error"), QStringLiteral("Update verification failed"),
                     error != QNetworkReply::NoError ? errorString
                                                     : QStringLiteral("Setup checksum is missing from SHA256SUMS.txt."));
            return;
        }
        setProgress(0.12);
        downloadInstaller();
    });
}

bool AppUpdateManager::parseExpectedChecksum(const QByteArray &payload)
{
    const QList<QByteArray> lines = payload.split('\n');
    for (const QByteArray &lineRaw : lines) {
        const QByteArray line = lineRaw.trimmed();
        if (line.isEmpty())
            continue;
        const int sep = line.indexOf("  ");
        if (sep <= 0)
            continue;
        const QByteArray hash = line.left(sep).trimmed().toLower();
        const QString name = QString::fromUtf8(line.mid(sep + 2).trimmed());
        if (name == m_setupAssetName && hash.size() == 64) {
            m_expectedSha256 = hash;
            return true;
        }
    }
    return false;
}

void AppUpdateManager::downloadInstaller()
{
    setState(QStringLiteral("downloading"),
             QStringLiteral("Downloading SonKuPik K500 %1…").arg(m_latestVersion));
    QNetworkReply *reply = get(m_setupAssetUrl);
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(0.12 + (qreal(received) / qreal(total)) * 0.78);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray bytes = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        reply->deleteLater();
        if (error != QNetworkReply::NoError) {
            setState(QStringLiteral("error"), QStringLiteral("Download failed"), errorString);
            return;
        }

        QSaveFile file(installerPath());
        if (!file.open(QIODevice::WriteOnly)
            || file.write(bytes) != bytes.size()
            || !file.commit()) {
            setState(QStringLiteral("error"), QStringLiteral("Could not save the update"), installerPath());
            return;
        }

        setProgress(0.92);
        setState(QStringLiteral("verifying"), QStringLiteral("Verifying downloaded installer…"));
        QString verifyError;
        if (!verifyInstaller(&verifyError)) {
            QFile::remove(installerPath());
            setState(QStringLiteral("error"), QStringLiteral("Downloaded update failed verification"), verifyError);
            return;
        }

        setProgress(1.0);
        emit updateReadyToInstall();
        QString launchError;
        if (!launchInstallerElevated(&launchError)) {
            setState(QStringLiteral("error"), QStringLiteral("Could not start the updater"), launchError);
            return;
        }
    });
}

bool AppUpdateManager::verifyInstaller(QString *error) const
{
    QFile file(installerPath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Downloaded installer cannot be opened.");
        return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        if (error) *error = QStringLiteral("Could not hash the downloaded installer.");
        return false;
    }
    const QByteArray actual = hash.result().toHex().toLower();
    if (m_expectedSha256.isEmpty() || actual != m_expectedSha256) {
        if (error) *error = QStringLiteral("SHA-256 mismatch. The update was not executed.");
        return false;
    }
    return true;
}

bool AppUpdateManager::launchInstallerElevated(QString *error)
{
#ifdef Q_OS_WIN
    const QString setup = QDir::toNativeSeparators(installerPath());
    const QString params = QStringLiteral(
        "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /AUToupdate=1");

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    const std::wstring setupW = setup.toStdWString();
    const std::wstring paramsW = params.toStdWString();
    info.lpFile = setupW.c_str();
    info.lpParameters = paramsW.c_str();
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info)) {
        const DWORD code = GetLastError();
        if (error) {
            *error = code == ERROR_CANCELLED
                ? QStringLiteral("Administrator permission was cancelled.")
                : QStringLiteral("Windows could not launch the installer (error %1).").arg(code);
        }
        return false;
    }
    if (info.hProcess)
        CloseHandle(info.hProcess);

    setState(QStringLiteral("installing"),
             QStringLiteral("Installer started. SonKuPik K500 will restart automatically."));
    QCoreApplication::quit();
    return true;
#else
    if (error) *error = QStringLiteral("Automatic installation is available on Windows only.");
    return false;
#endif
}
