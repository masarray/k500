#include "AppUpdateManager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
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
#include <QTimer>
#include <QVersionNumber>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
const QUrl LatestReleaseUrl(QStringLiteral(
    "https://api.github.com/repos/masarray/k500/releases/latest"));
constexpr qint64 AutomaticCheckIntervalSecs = 6 * 60 * 60;
constexpr qint64 MinimumInstallerBytes = 1024 * 1024;

QByteArray normalizedTagVersion(const QString &tag)
{
    QString value = tag.trimmed();
    if (value.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        value.remove(0, 1);
    return value.toLatin1();
}

bool trustedGitHubApi(const QUrl &url)
{
    return url.isValid()
        && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && url.host().compare(QStringLiteral("api.github.com"), Qt::CaseInsensitive) == 0;
}

bool trustedGitHubAsset(const QUrl &url)
{
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0)
        return false;
    const QString host = url.host().toLower();
    return host == QStringLiteral("github.com")
        || host == QStringLiteral("objects.githubusercontent.com")
        || host == QStringLiteral("release-assets.githubusercontent.com")
        || host == QStringLiteral("github-releases.githubusercontent.com");
}

bool validSha256Hex(const QByteArray &value)
{
    if (value.size() != 64)
        return false;
    for (const char ch : value) {
        const bool decimal = ch >= '0' && ch <= '9';
        const bool lowerHex = ch >= 'a' && ch <= 'f';
        const bool upperHex = ch >= 'A' && ch <= 'F';
        if (!decimal && !lowerHex && !upperHex)
            return false;
    }
    return true;
}

QString expectedSetupName(const QString &version, bool perUser)
{
    return perUser
        ? QStringLiteral("SonKuPik-K500-v%1-Windows-Setup-PerUser.exe").arg(version)
        : QStringLiteral("SonKuPik-K500-v%1-Windows-Setup.exe").arg(version);
}
// Quote one Windows command-line argument using backslash/quote escaping.
// Parameters are passed as one string to ShellExecuteExW.
QString quoteWindowsArgument(const QString &value)
{
    QString result = QStringLiteral("\"");
    int slashes = 0;
    for (const QChar character : value) {
        if (character == QLatin1Char('\\')) {
            ++slashes;
        } else if (character == QLatin1Char('"')) {
            result += QString(slashes * 2 + 1, QLatin1Char('\\'));
            result += character;
            slashes = 0;
        } else {
            result += QString(slashes, QLatin1Char('\\'));
            slashes = 0;
            result += character;
        }
    }
    result += QString(slashes * 2, QLatin1Char('\\'));
    result += QLatin1Char('"');
    return result;
}

}

AppUpdateManager::AppUpdateManager(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

AppUpdateManager::InstallScope AppUpdateManager::detectedInstallScope() const
{
#ifdef Q_OS_WIN
    // Installation identity comes from Inno's uninstall registration, not a
    // writable install-scope.ini file or a guessed directory prefix. The
    // registry path must point to THIS executable's directory.
    const QString key = QStringLiteral(
        "/Software/Microsoft/Windows/CurrentVersion/Uninstall/"
        "{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1");
    const QString installed = QDir::toNativeSeparators(
        QDir::cleanPath(QCoreApplication::applicationDirPath()));
    const auto matches = [&installed, &key](const QString &root) {
        QSettings settings(root + key, QSettings::NativeFormat);
        const QString directory = settings.value(QStringLiteral("Inno Setup: App Path")).toString();
        return !directory.isEmpty()
            && QString::compare(
                QDir::toNativeSeparators(QDir::cleanPath(directory)),
                installed, Qt::CaseInsensitive) == 0;
    };
    const bool user = matches(QStringLiteral("HKEY_CURRENT_USER"));
    const bool machine = matches(QStringLiteral("HKEY_LOCAL_MACHINE"));
    if (user == machine)
        return InstallScope::Unknown; // absent OR ambiguous/duplicate registration
    if (!QFileInfo::exists(QDir(installed).filePath(QStringLiteral("unins000.exe"))))
        return InstallScope::Unknown; // a copied portable directory
    return user ? InstallScope::User : InstallScope::Machine;
#else
    return InstallScope::Unknown;
#endif
}

QString AppUpdateManager::installationScope() const
{
    switch (detectedInstallScope()) {
    case InstallScope::Machine: return QStringLiteral("machine");
    case InstallScope::User: return QStringLiteral("user");
    default: return QStringLiteral("unknown");
    }
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
        || m_state == QStringLiteral("installing")
        || m_state == QStringLiteral("waiting-for-device");
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
    m_migrationAvailable = false;
    m_migrationMode = false;
    m_targetScope = InstallScope::Unknown;
    m_setupAssetName.clear();
    m_setupAssetUrl = {};
    m_manifestUrl = {};
    m_checksumsUrl = {};
    m_manifestSha256.clear();
    m_expectedSha256.clear();
    m_manifestSetupBytes = -1;
    m_releaseSetupBytes = -1;
    m_machineSetupAssetName.clear();
    m_machineSetupAssetUrl = {};
    m_machineSetupBytes = -1;
    m_userSetupAssetName.clear();
    m_userSetupAssetUrl = {};
    m_userSetupBytes = -1;
    setProgress(0.0);
    emit updateChanged();
}

void AppUpdateManager::selectPackageForScope(InstallScope scope)
{
    m_targetScope = scope;
    if (scope == InstallScope::User) {
        m_setupAssetName = m_userSetupAssetName;
        m_setupAssetUrl = m_userSetupAssetUrl;
        m_releaseSetupBytes = m_userSetupBytes;
    } else if (scope == InstallScope::Machine) {
        m_setupAssetName = m_machineSetupAssetName;
        m_setupAssetUrl = m_machineSetupAssetUrl;
        m_releaseSetupBytes = m_machineSetupBytes;
    } else {
        m_setupAssetName.clear();
        m_setupAssetUrl = {};
        m_releaseSetupBytes = -1;
    }
    m_manifestSha256.clear();
    m_expectedSha256.clear();
    m_manifestSetupBytes = -1;
}

QString AppUpdateManager::perUserTargetApplication() const
{
#ifdef Q_OS_WIN
    QString local = qEnvironmentVariable("LOCALAPPDATA");
    if (local.isEmpty())
        local = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (local.isEmpty())
        return {};
    return QDir::toNativeSeparators(QDir(local).filePath(
        QStringLiteral("Programs/SonKuPik K500/SonKuPik-K500.exe")));
#else
    return {};
#endif
}

QString AppUpdateManager::partialInstallerPath() const
{
    return installerPath() + QStringLiteral(".part");
}

QNetworkReply *AppUpdateManager::get(const QUrl &url, qint64 rangeStart)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArray("SonKuPik-K500/") + currentVersion().toUtf8());
    request.setRawHeader("Accept", "application/vnd.github+json, application/octet-stream;q=0.9, */*;q=0.8");
    if (rangeStart >= 0)
        request.setRawHeader("Range", QByteArray("bytes=") + QByteArray::number(rangeStart) + '-');
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return m_network->get(request);
}

void AppUpdateManager::checkForUpdates(bool userInitiated)
{
    if (busy())
        return;

#ifdef Q_OS_WIN
    m_installScope = detectedInstallScope();
    if (m_installScope == InstallScope::Unknown) {
        setState(userInitiated ? QStringLiteral("error") : QStringLiteral("idle"),
                 userInitiated ? QStringLiteral("Installed copy required") : QString(),
                 userInitiated ? QStringLiteral("This copy has no matching Inno uninstall registration, or its install scope is ambiguous. Use the installed application to update.") : QString());
        return;
    }
#endif

    // SMART_UPDATE_THROTTLE_V1 — automatic discovery is deliberately quiet and
    // bounded. Manual checks always bypass the throttle; failed checks are not
    // cached, so a transient outage can recover on the next launch.
    if (!userInitiated) {
        const QDateTime lastCheck = QSettings().value(
            QStringLiteral("updates/lastSuccessfulCheckUtc")).toDateTime();
        if (lastCheck.isValid()
            && lastCheck.secsTo(QDateTime::currentDateTimeUtc()) >= 0
            && lastCheck.secsTo(QDateTime::currentDateTimeUtc()) < AutomaticCheckIntervalSecs) {
            setState(QStringLiteral("idle"));
            return;
        }
    }

    resetReleaseMetadata();
    setState(QStringLiteral("checking"), QStringLiteral("Checking for updates…"));

    QNetworkReply *reply = get(LatestReleaseUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply, userInitiated] {
        const QByteArray payload = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        const QUrl finalUrl = reply->url();
        reply->deleteLater();

        if (error != QNetworkReply::NoError || !trustedGitHubApi(finalUrl)) {
            const QString reason = error != QNetworkReply::NoError
                ? errorString : QStringLiteral("Update discovery left the trusted GitHub API endpoint.");
            setState(userInitiated ? QStringLiteral("error") : QStringLiteral("idle"),
                     userInitiated ? QStringLiteral("Update check failed") : QString(),
                     userInitiated ? reason : QString());
            return;
        }

        QSettings().setValue(QStringLiteral("updates/lastSuccessfulCheckUtc"),
                             QDateTime::currentDateTimeUtc());
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

    const QString machineName = expectedSetupName(version, false);
    const QString userName = expectedSetupName(version, true);
    QUrl manifestUrl;
    QUrl sumsUrl;
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
        if (!trustedGitHubAsset(url))
            continue;
        const qint64 bytes = qint64(asset.value(QStringLiteral("size")).toDouble(-1));
        if (name == machineName) {
            m_machineSetupAssetName = name;
            m_machineSetupAssetUrl = url;
            m_machineSetupBytes = bytes;
        } else if (name == userName) {
            m_userSetupAssetName = name;
            m_userSetupAssetUrl = url;
            m_userSetupBytes = bytes;
        } else if (name == QStringLiteral("release-manifest.json")) {
            manifestUrl = url;
        } else if (name == QStringLiteral("SHA256SUMS.txt")) {
            sumsUrl = url;
        }
    }

    m_latestVersion = version;
    m_releaseNotes = release.value(QStringLiteral("body")).toString().trimmed();
    m_manifestUrl = manifestUrl;
    m_checksumsUrl = sumsUrl;

    const bool commonMetadataReady = !manifestUrl.isEmpty() && !sumsUrl.isEmpty();
    m_migrationAvailable = m_installScope == InstallScope::Machine
        && commonMetadataReady
        && !m_userSetupAssetName.isEmpty()
        && !m_userSetupAssetUrl.isEmpty()
        && m_userSetupBytes >= MinimumInstallerBytes
        && QVersionNumber::compare(latest, current) >= 0;

    const int comparison = QVersionNumber::compare(latest, current);
    if (comparison <= 0) {
        emit updateChanged();
        if (userInitiated && m_migrationAvailable) {
            setState(QStringLiteral("migration-available"),
                     QStringLiteral("Move SonKuPik to a no-admin per-user installation"));
        } else {
            setState(userInitiated ? QStringLiteral("up-to-date") : QStringLiteral("idle"),
                     userInitiated ? QStringLiteral("You already have the latest version") : QString());
        }
        return;
    }

    selectPackageForScope(m_installScope);
    if (m_setupAssetName.isEmpty() || m_setupAssetUrl.isEmpty()
        || m_releaseSetupBytes < MinimumInstallerBytes || !commonMetadataReady) {
        setState(userInitiated ? QStringLiteral("error") : QStringLiteral("idle"),
                 userInitiated ? QStringLiteral("Stable update package is incomplete") : QString(),
                 userInitiated ? QStringLiteral("Required scope-matched setup/manifest/checksum assets were not found.") : QString());
        return;
    }

    const QString skipped = QSettings().value(QStringLiteral("updates/skippedVersion")).toString();
    if (!userInitiated && skipped == version) {
        setState(QStringLiteral("idle"));
        return;
    }

    m_updateAvailable = true;
    emit updateChanged();
    setState(QStringLiteral("available"),
             QStringLiteral("SonKuPik K500 %1 is ready").arg(version));
}

void AppUpdateManager::remindLater()
{
    if (busy())
        return;

    // REMIND_NEXT_LAUNCH_V1 — "Nanti" is intentionally literal: clear the
    // successful-check throttle so the next application launch performs fresh
    // discovery and can offer this release again. No installer is downloaded.
    QSettings().remove(QStringLiteral("updates/lastSuccessfulCheckUtc"));
    setState(QStringLiteral("available"),
             QStringLiteral("Update postponed until the next launch"));
}

void AppUpdateManager::setDeviceTransactionBusy(bool busy)
{
    m_deviceTransactionBusy = busy;
    if (busy || m_state != QStringLiteral("waiting-for-device"))
        return;

    // Allow the preset coordinator to publish its final settled state before
    // handing control to the external updater. Re-check at the final boundary.
    QTimer::singleShot(250, this, [this] {
        if (m_deviceTransactionBusy || m_state != QStringLiteral("waiting-for-device"))
            return;
        QString verifyError;
        if (!verifyInstaller(&verifyError)) {
            QFile::remove(installerPath());
            setState(QStringLiteral("error"), QStringLiteral("Update verification failed"), verifyError);
            return;
        }
        QString launchError;
        if (!launchInstallerElevated(&launchError))
            setState(QStringLiteral("error"), QStringLiteral("Could not start updater"), launchError);
    });
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
#ifdef Q_OS_WIN
    if (m_installScope == InstallScope::Unknown
        || detectedInstallScope() != m_installScope) {
        setState(QStringLiteral("error"), QStringLiteral("Installed application required"),
                 QStringLiteral("Installation scope changed or is not registered. Update was not started."));
        return;
    }
#endif
    m_migrationMode = false;
    selectPackageForScope(m_installScope);
    beginSelectedPackageInstall();
}

void AppUpdateManager::migrateToPerUser()
{
#ifdef Q_OS_WIN
    if (!m_migrationAvailable || busy()
        || m_installScope != InstallScope::Machine
        || detectedInstallScope() != InstallScope::Machine) {
        setState(QStringLiteral("error"), QStringLiteral("Migration is not available"),
                 QStringLiteral("Only a registered machine-wide installation can be explicitly moved to the current user."));
        return;
    }
    if (perUserTargetApplication().isEmpty()) {
        setState(QStringLiteral("error"), QStringLiteral("Migration path is unavailable"),
                 QStringLiteral("Windows LocalAppData could not be resolved."));
        return;
    }

    m_migrationMode = true;
    selectPackageForScope(InstallScope::User);
    emit updateChanged();
    beginSelectedPackageInstall();
#else
    setState(QStringLiteral("error"), QStringLiteral("Migration is available on Windows only."));
#endif
}

void AppUpdateManager::beginSelectedPackageInstall()
{
    if (m_setupAssetName.isEmpty() || m_setupAssetUrl.isEmpty()
        || m_releaseSetupBytes < MinimumInstallerBytes
        || m_manifestUrl.isEmpty() || m_checksumsUrl.isEmpty()) {
        setState(QStringLiteral("error"), QStringLiteral("Update package is incomplete"),
                 QStringLiteral("The selected install-scope package is missing from the stable release."));
        return;
    }

    m_manifestSha256.clear();
    m_expectedSha256.clear();
    m_manifestSetupBytes = -1;
    setProgress(0.0);
    setState(QStringLiteral("preparing"),
             m_migrationMode
                 ? QStringLiteral("Preparing explicit no-admin migration…")
                 : QStringLiteral("Validating release metadata…"));
    downloadManifest();
}

void AppUpdateManager::downloadManifest()
{
    QNetworkReply *reply = get(m_manifestUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray payload = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        const QUrl finalUrl = reply->url();
        reply->deleteLater();
        if (error != QNetworkReply::NoError || !trustedGitHubAsset(finalUrl)
            || !validateManifest(payload)) {
            const QString reason = error != QNetworkReply::NoError ? errorString
                : !trustedGitHubAsset(finalUrl)
                    ? QStringLiteral("Release manifest redirected outside trusted GitHub asset hosts.")
                    : m_errorText;
            setState(QStringLiteral("error"), QStringLiteral("Update verification failed"), reason);
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
    if (manifest.value(QStringLiteral("schema")).toString()
            != QStringLiteral("sonkupik-k500-release-manifest-v3")
        || manifest.value(QStringLiteral("product")).toString() != QStringLiteral("SonKuPik K500")
        || manifest.value(QStringLiteral("channel")).toString() != QStringLiteral("stable")
        || manifest.value(QStringLiteral("version")).toString() != m_latestVersion
        || !manifest.value(QStringLiteral("stableReleaseEligible")).toBool()
        || manifest.value(QStringLiteral("target")).toString() != QStringLiteral("windows-x64")
        || manifest.value(QStringLiteral("installerTechnology")).toString() != QStringLiteral("Inno Setup 6")) {
        m_errorText = QStringLiteral("Release manifest does not match this stable Windows update.");
        return false;
    }

    const QString requiredName = expectedSetupName(
        m_latestVersion, m_targetScope == InstallScope::User);
    if (m_setupAssetName != requiredName) {
        m_errorText = QStringLiteral("Setup filename does not match the requested application version.");
        return false;
    }

    QByteArray manifestHash;
    qint64 manifestBytes = -1;
    const QJsonArray artifacts = manifest.value(QStringLiteral("artifacts")).toArray();
    for (const QJsonValue &value : artifacts) {
        const QJsonObject artifact = value.toObject();
        if (artifact.value(QStringLiteral("file")).toString() != requiredName)
            continue;
        manifestHash = artifact.value(QStringLiteral("sha256")).toString().toLatin1().toLower();
        manifestBytes = qint64(artifact.value(QStringLiteral("bytes")).toDouble(-1));
        break;
    }

    if (!validSha256Hex(manifestHash) || manifestBytes < MinimumInstallerBytes) {
        m_errorText = QStringLiteral("Release manifest is missing a valid Setup artifact identity.");
        return false;
    }
    if (m_releaseSetupBytes > 0 && manifestBytes != m_releaseSetupBytes) {
        m_errorText = QStringLiteral("GitHub asset size does not match the signed release manifest metadata.");
        return false;
    }

    m_manifestSha256 = manifestHash;
    m_manifestSetupBytes = manifestBytes;
    return true;
}

void AppUpdateManager::downloadChecksums()
{
    setState(QStringLiteral("preparing"), QStringLiteral("Cross-checking release checksum…"));
    QNetworkReply *reply = get(m_checksumsUrl);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray payload = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        const QUrl finalUrl = reply->url();
        reply->deleteLater();
        if (error != QNetworkReply::NoError || !trustedGitHubAsset(finalUrl)
            || !parseExpectedChecksum(payload)) {
            const QString reason = error != QNetworkReply::NoError ? errorString
                : !trustedGitHubAsset(finalUrl)
                    ? QStringLiteral("Checksum file redirected outside trusted GitHub asset hosts.")
                    : (m_errorText.isEmpty()
                        ? QStringLiteral("Setup checksum is missing from SHA256SUMS.txt.")
                        : m_errorText);
            setState(QStringLiteral("error"), QStringLiteral("Update verification failed"), reason);
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
        if (line.size() < 66)
            continue;

        const QByteArray hash = line.left(64).toLower();
        if (!validSha256Hex(hash))
            continue;

        QByteArray tail = line.mid(64).trimmed();
        if (tail.startsWith('*'))
            tail.remove(0, 1);
        const QString name = QString::fromUtf8(tail.trimmed());
        if (name != m_setupAssetName)
            continue;

        if (m_manifestSha256.isEmpty() || hash != m_manifestSha256) {
            m_errorText = QStringLiteral("SHA256SUMS.txt disagrees with release-manifest.json.");
            return false;
        }
        m_expectedSha256 = hash;
        return true;
    }
    m_errorText = QStringLiteral("Setup checksum is missing from SHA256SUMS.txt.");
    return false;
}

void AppUpdateManager::downloadInstaller()
{
    const QString finalPath = installerPath();
    const QString partPath = partialInstallerPath();

    // Reuse a previously completed, already-verified package after a helper/UAC
    // failure instead of downloading the same release again.
    if (QFileInfo(finalPath).size() == m_manifestSetupBytes) {
        QString cachedError;
        if (verifyInstaller(&cachedError)) {
            setProgress(1.0);
            setState(QStringLiteral("verifying"), QStringLiteral("Verified cached installer…"));
            QString launchError;
            if (!launchInstallerElevated(&launchError))
                setState(QStringLiteral("error"), QStringLiteral("Could not start the updater"), launchError);
            return;
        }
        QFile::remove(finalPath);
    } else {
        QFile::remove(finalPath);
    }

    qint64 resumeOffset = QFileInfo(partPath).size();
    if (resumeOffset < 0 || resumeOffset > m_manifestSetupBytes) {
        QFile::remove(partPath);
        resumeOffset = 0;
    }
    if (resumeOffset == m_manifestSetupBytes && resumeOffset > 0) {
        QFile::remove(finalPath);
        if (QFile::rename(partPath, finalPath)) {
            QString cachedError;
            if (verifyInstaller(&cachedError)) {
                setProgress(1.0);
                QString launchError;
                if (!launchInstallerElevated(&launchError))
                    setState(QStringLiteral("error"), QStringLiteral("Could not start the updater"), launchError);
                return;
            }
        }
        QFile::remove(finalPath);
        QFile::remove(partPath);
        resumeOffset = 0;
    }

    setState(QStringLiteral("downloading"),
             resumeOffset > 0
                 ? QStringLiteral("Resuming SonKuPik K500 %1…").arg(m_latestVersion)
                 : QStringLiteral("Downloading SonKuPik K500 %1…").arg(m_latestVersion));

    QNetworkReply *reply = get(m_setupAssetUrl, resumeOffset > 0 ? resumeOffset : -1);
    auto *file = new QFile(partPath, reply);
    reply->setProperty("sonkupikResumeOffset", resumeOffset);
    reply->setProperty("sonkupikBaseBytes", qint64(0));
    reply->setProperty("sonkupikWriteFailed", false);
    reply->setProperty("sonkupikRangeInvalid", false);

    auto prepareFile = [reply, file]() -> bool {
        if (file->isOpen())
            return true;

        const qint64 requestedOffset = reply->property("sonkupikResumeOffset").toLongLong();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        bool append = requestedOffset > 0 && status == 206;

        if (append) {
            const QByteArray expectedPrefix = QByteArray("bytes ")
                + QByteArray::number(requestedOffset) + '-';
            const QByteArray range = reply->rawHeader("Content-Range").trimmed();
            if (!range.startsWith(expectedPrefix)) {
                reply->setProperty("sonkupikRangeInvalid", true);
                QFile::remove(file->fileName());
                reply->abort();
                return false;
            }
        } else if (requestedOffset > 0) {
            // Server ignored Range and returned a full 200 response. Restart
            // safely from byte zero rather than concatenating duplicate bytes.
            QFile::remove(file->fileName());
        }

        const QIODevice::OpenMode mode = append
            ? (QIODevice::WriteOnly | QIODevice::Append)
            : (QIODevice::WriteOnly | QIODevice::Truncate);
        if (!file->open(mode)) {
            reply->setProperty("sonkupikWriteFailed", true);
            reply->abort();
            return false;
        }
        reply->setProperty("sonkupikBaseBytes", append ? requestedOffset : qint64(0));
        return true;
    };

    connect(reply, &QNetworkReply::metaDataChanged, this, [prepareFile] {
        prepareFile();
    });
    connect(reply, &QNetworkReply::readyRead, this, [reply, file, prepareFile] {
        if (!prepareFile())
            return;
        const QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty() && file->write(chunk) != chunk.size()) {
            reply->setProperty("sonkupikWriteFailed", true);
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, reply](qint64 received, qint64) {
        const qint64 base = reply->property("sonkupikBaseBytes").toLongLong();
        if (m_manifestSetupBytes > 0) {
            const qreal ratio = qMin<qreal>(
                1.0, qreal(base + received) / qreal(m_manifestSetupBytes));
            setProgress(0.12 + ratio * 0.78);
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, prepareFile, partPath, finalPath] {
        const auto networkError = reply->error();
        // A connection failure before HTTP metadata must NOT truncate an
        // existing partial file. Only prepare/open after metadata succeeded,
        // or finish writing a stream that was already opened by readyRead.
        if (file->isOpen() || networkError == QNetworkReply::NoError) {
            if (prepareFile()) {
                const QByteArray remaining = reply->readAll();
                if (!remaining.isEmpty() && file->write(remaining) != remaining.size())
                    reply->setProperty("sonkupikWriteFailed", true);
            }
        }
        if (file->isOpen()) {
            file->flush();
            file->close();
        }

        const QString errorString = reply->errorString();
        const QUrl finalUrl = reply->url();
        const bool writeFailed = reply->property("sonkupikWriteFailed").toBool();
        const bool rangeInvalid = reply->property("sonkupikRangeInvalid").toBool();
        reply->deleteLater();

        if (!trustedGitHubAsset(finalUrl) || writeFailed || rangeInvalid) {
            QFile::remove(partPath);
            const QString reason = !trustedGitHubAsset(finalUrl)
                ? QStringLiteral("Installer download redirected outside trusted GitHub asset hosts.")
                : rangeInvalid
                    ? QStringLiteral("The server returned an invalid HTTP byte range; partial data was discarded.")
                    : QStringLiteral("Windows could not write the update package.");
            setState(QStringLiteral("error"), QStringLiteral("Download failed"), reason);
            return;
        }

        if (networkError != QNetworkReply::NoError) {
            const qint64 saved = QFileInfo(partPath).size();
            const QString resume = saved > 0 && saved < m_manifestSetupBytes
                ? QStringLiteral(" %1 MB was kept; press Coba lagi to resume.")
                    .arg(QString::number(double(saved) / (1024.0 * 1024.0), 'f', 1))
                : QString();
            setState(QStringLiteral("error"), QStringLiteral("Download interrupted"),
                     errorString + resume);
            return;
        }

        const qint64 downloadedBytes = QFileInfo(partPath).size();
        if (m_manifestSetupBytes <= 0 || downloadedBytes != m_manifestSetupBytes
            || (m_releaseSetupBytes > 0 && downloadedBytes != m_releaseSetupBytes)) {
            if (downloadedBytes > m_manifestSetupBytes)
                QFile::remove(partPath);
            setState(QStringLiteral("error"), QStringLiteral("Downloaded update is incomplete"),
                     QStringLiteral("Installer size does not match release metadata. Press Coba lagi to retry/resume safely."));
            return;
        }

        QFile::remove(finalPath);
        if (!QFile::rename(partPath, finalPath)) {
            setState(QStringLiteral("error"), QStringLiteral("Could not finalize the update"), finalPath);
            return;
        }

        setProgress(0.92);
        setState(QStringLiteral("verifying"), QStringLiteral("Verifying downloaded installer…"));
        QString verifyError;
        if (!verifyInstaller(&verifyError)) {
            QFile::remove(finalPath);
            QFile::remove(partPath);
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
    if (m_manifestSetupBytes < MinimumInstallerBytes || file.size() != m_manifestSetupBytes) {
        if (error) *error = QStringLiteral("Downloaded installer size is not the verified release size.");
        return false;
    }
    if (file.read(2) != QByteArrayLiteral("MZ") || !file.seek(0)) {
        if (error) *error = QStringLiteral("Downloaded package is not a Windows executable.");
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        if (error) *error = QStringLiteral("Could not hash the downloaded installer.");
        return false;
    }
    const QByteArray actual = hash.result().toHex().toLower();
    if (m_expectedSha256.isEmpty() || m_manifestSha256.isEmpty()
        || m_expectedSha256 != m_manifestSha256 || actual != m_expectedSha256) {
        if (error) *error = QStringLiteral("SHA-256 mismatch. The update was not executed.");
        return false;
    }
    return true;
}

bool AppUpdateManager::launchInstallerElevated(QString *error)
{
#ifdef Q_OS_WIN
    // UPDATE_HANDOFF_P3 — check authoritative K500 transaction state again at
    // the final process boundary, after any long/resumed package download.
    if (m_deviceTransactionBusy) {
        setState(QStringLiteral("waiting-for-device"),
                 QStringLiteral("Waiting for the K500 hardware transaction to finish…"));
        return true;
    }

    if (m_installScope == InstallScope::Unknown
        || detectedInstallScope() != m_installScope) {
        if (error) *error = QStringLiteral("Installation registration changed; the updater will not switch install scope.");
        return false;
    }
    if (m_targetScope == InstallScope::Unknown) {
        if (error) *error = QStringLiteral("Update target installation scope is unknown.");
        return false;
    }
    if (m_migrationMode
        && (m_installScope != InstallScope::Machine || m_targetScope != InstallScope::User)) {
        if (error) *error = QStringLiteral("Invalid migration direction; only machine-to-user is supported.");
        return false;
    }

    // The helper is part of the currently installed application and is copied
    // into the per-user update cache before the application exits.
    const QString source = QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("SonKuPik-K500-Updater.exe"));
    const QString staged = QDir(updateDirectory())
        .filePath(QStringLiteral("SonKuPik-K500-Updater.exe"));
    QFile original(source);
    if (!original.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("The installed update coordinator is missing. Please install the next official Setup once.");
        return false;
    }
    const QByteArray sourceHash = QCryptographicHash::hash(original.readAll(), QCryptographicHash::Sha256);
    original.close();
    QFile::remove(staged);
    if (!QFile::copy(source, staged)) {
        if (error) *error = QStringLiteral("Could not stage the update coordinator outside the installation directory.");
        return false;
    }
    QFile stagedFile(staged);
    if (!stagedFile.open(QIODevice::ReadOnly)
        || QCryptographicHash::hash(stagedFile.readAll(), QCryptographicHash::Sha256) != sourceHash) {
        if (error) *error = QStringLiteral("Staged update coordinator does not match the installed binary.");
        return false;
    }
    stagedFile.close();

    const QString setup = QDir::toNativeSeparators(installerPath());
    const QString app = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString targetApp = m_migrationMode
        ? perUserTargetApplication()
        : app;
    if (targetApp.isEmpty()) {
        if (error) *error = QStringLiteral("Target application path could not be resolved.");
        return false;
    }
    const QString log = QDir::toNativeSeparators(
        QDir(updateDirectory()).filePath(QStringLiteral("update-handoff.log")));
    const QString backup = QDir::toNativeSeparators(
        QDir(updateDirectory()).filePath(QStringLiteral("recovery-previous")));
    const QStringList args = {
        QStringLiteral("--parent-pid"), QString::number(GetCurrentProcessId()),
        QStringLiteral("--setup"), setup,
        QStringLiteral("--app"), app,
        QStringLiteral("--target-app"), QDir::toNativeSeparators(targetApp),
        QStringLiteral("--version"), m_latestVersion,
        QStringLiteral("--previous-version"), currentVersion(),
        QStringLiteral("--log"), log,
        QStringLiteral("--sha256"), QString::fromLatin1(m_expectedSha256),
        QStringLiteral("--scope"), m_targetScope == InstallScope::User
            ? QStringLiteral("user") : QStringLiteral("machine"),
        QStringLiteral("--mode"), m_migrationMode
            ? QStringLiteral("migrate") : QStringLiteral("update"),
        QStringLiteral("--backup"), backup
    };
    QStringList quoted;
    for (const QString &arg : args)
        quoted.append(quoteWindowsArgument(arg));
    const std::wstring executable = QDir::toNativeSeparators(staged).toStdWString();
    const std::wstring parameters = quoted.join(QLatin1Char(' ')).toStdWString();

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"open";
    info.lpFile = executable.c_str();
    info.lpParameters = parameters.c_str();
    info.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&info)) {
        const DWORD code = GetLastError();
        if (error)
            *error = QStringLiteral("Windows could not start the update coordinator (error %1).").arg(code);
        return false;
    }
    if (info.hProcess)
        CloseHandle(info.hProcess);

    setState(QStringLiteral("installing"),
             m_migrationMode
                 ? QStringLiteral("K500 is closing safely. The updater will move this installation to your Windows account, verify it, then remove the old Program Files copy.")
                 : QStringLiteral("K500 is closing safely. The updater will install, verify, recover on failure, and restart the application."));
    QCoreApplication::quit();
    return true;
#else
    if (error) *error = QStringLiteral("Automatic installation is available on Windows only.");
    return false;
#endif
}
