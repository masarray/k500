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
    m_setupAssetName.clear();
    m_setupAssetUrl = {};
    m_manifestUrl = {};
    m_checksumsUrl = {};
    m_manifestSha256.clear();
    m_expectedSha256.clear();
    m_manifestSetupBytes = -1;
    m_releaseSetupBytes = -1;
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

    if (QVersionNumber::compare(latest, current) <= 0) {
        m_latestVersion = version;
        emit updateChanged();
        setState(userInitiated ? QStringLiteral("up-to-date") : QStringLiteral("idle"),
                 userInitiated ? QStringLiteral("You already have the latest version") : QString());
        return;
    }

    const QString requiredSetupName = expectedSetupName(
        version, m_installScope == InstallScope::User);
    QString setupName;
    QUrl setupUrl;
    qint64 setupBytes = -1;
    QUrl manifestUrl;
    QUrl sumsUrl;
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
        if (!trustedGitHubAsset(url))
            continue;
        if (name == requiredSetupName) {
            setupName = name;
            setupUrl = url;
            setupBytes = qint64(asset.value(QStringLiteral("size")).toDouble(-1));
        } else if (name == QStringLiteral("release-manifest.json")) {
            manifestUrl = url;
        } else if (name == QStringLiteral("SHA256SUMS.txt")) {
            sumsUrl = url;
        }
    }

    if (setupName.isEmpty() || setupUrl.isEmpty() || setupBytes < MinimumInstallerBytes
        || manifestUrl.isEmpty() || sumsUrl.isEmpty()) {
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
    m_releaseSetupBytes = setupBytes;
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
    // Portable ZIP must never be silently converted into a machine-wide install.
    // A future dedicated portable updater must be an explicit separate workflow.
    // Re-check at the download/install boundary. A copied portable directory
    // or a changed registry registration must never change the package scope.
    if (m_installScope == InstallScope::Unknown
        || detectedInstallScope() != m_installScope) {
        setState(QStringLiteral("error"), QStringLiteral("Installed application required"),
                 QStringLiteral("Installation scope changed or is not registered. Update was not started."));
        return;
    }
#endif
    m_manifestSha256.clear();
    m_expectedSha256.clear();
    m_manifestSetupBytes = -1;
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
        m_latestVersion, m_installScope == InstallScope::User);
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
    setState(QStringLiteral("downloading"),
             QStringLiteral("Downloading SonKuPik K500 %1…").arg(m_latestVersion));

    QFile::remove(installerPath());
    QNetworkReply *reply = get(m_setupAssetUrl);
    auto *file = new QSaveFile(reply);
    file->setFileName(installerPath());
    if (!file->open(QIODevice::WriteOnly)) {
        reply->abort();
        reply->deleteLater();
        setState(QStringLiteral("error"), QStringLiteral("Could not save the update"), installerPath());
        return;
    }

    connect(reply, &QNetworkReply::readyRead, this, [reply, file] {
        const QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty() && file->write(chunk) != chunk.size())
            file->setProperty("sonkupikWriteFailed", true);
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
        const qint64 expected = m_manifestSetupBytes > 0 ? m_manifestSetupBytes : total;
        if (expected > 0)
            setProgress(0.12 + qMin<qreal>(1.0, qreal(received) / qreal(expected)) * 0.78);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file] {
        const QByteArray remaining = reply->readAll();
        if (!remaining.isEmpty() && file->write(remaining) != remaining.size())
            file->setProperty("sonkupikWriteFailed", true);

        const auto error = reply->error();
        const QString errorString = reply->errorString();
        const QUrl finalUrl = reply->url();
        const bool writeFailed = file->property("sonkupikWriteFailed").toBool();

        if (error != QNetworkReply::NoError || !trustedGitHubAsset(finalUrl) || writeFailed) {
            file->cancelWriting();
            reply->deleteLater();
            const QString reason = error != QNetworkReply::NoError ? errorString
                : !trustedGitHubAsset(finalUrl)
                    ? QStringLiteral("Installer download redirected outside trusted GitHub asset hosts.")
                    : QStringLiteral("Windows could not write the complete update package.");
            setState(QStringLiteral("error"), QStringLiteral("Download failed"), reason);
            return;
        }

        if (!file->commit()) {
            reply->deleteLater();
            setState(QStringLiteral("error"), QStringLiteral("Could not save the update"), installerPath());
            return;
        }
        reply->deleteLater();

        const qint64 downloadedBytes = QFileInfo(installerPath()).size();
        if (m_manifestSetupBytes <= 0 || downloadedBytes != m_manifestSetupBytes
            || (m_releaseSetupBytes > 0 && downloadedBytes != m_releaseSetupBytes)) {
            QFile::remove(installerPath());
            setState(QStringLiteral("error"), QStringLiteral("Downloaded update failed verification"),
                     QStringLiteral("Installer size does not match the release metadata."));
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
    // UPDATE_HANDOFF_P1 — the UI block alone is insufficient. A device Store,
    // Upload, Recall, or Mass Upload may begin during a long package download.
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

    // The helper is the installed app's own native binary, not downloaded from
    // a release asset. It runs unelevated; only Inno requests UAC after K500 exits.
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
    const QString log = QDir::toNativeSeparators(
        QDir(updateDirectory()).filePath(QStringLiteral("update-handoff.log")));
    const QStringList args = {
        QStringLiteral("--parent-pid"), QString::number(GetCurrentProcessId()),
        QStringLiteral("--setup"), setup,
        QStringLiteral("--app"), app,
        QStringLiteral("--version"), m_latestVersion,
        QStringLiteral("--log"), log,
        QStringLiteral("--sha256"), QString::fromLatin1(m_expectedSha256),
        QStringLiteral("--scope"), m_installScope == InstallScope::User
            ? QStringLiteral("user") : QStringLiteral("machine")
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
             QStringLiteral("K500 is closing safely. The updater will verify and restart the application."));
    QCoreApplication::quit();
    return true;
#else
    if (error) *error = QStringLiteral("Automatic installation is available on Windows only.");
    return false;
#endif
}
