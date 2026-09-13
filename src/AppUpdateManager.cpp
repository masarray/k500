#include "AppUpdateManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QVersionNumber>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
const QUrl LatestReleaseApi(QStringLiteral("https://api.github.com/repos/masarray/k500/releases/latest"));
constexpr auto LastCheckKey = "updates/lastCheckUtc";
constexpr auto LastDismissedVersionKey = "updates/lastDismissedVersion";
constexpr auto LastDismissedUtcKey = "updates/lastDismissedUtc";
constexpr qint64 AutomaticCheckIntervalSecs = 12 * 60 * 60;
constexpr qint64 DismissedReminderSecs = 24 * 60 * 60;

QNetworkRequest jsonRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArray("SonKuPik-K500/") + QCoreApplication::applicationVersion().toUtf8());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(20000);
    return request;
}

QNetworkRequest assetRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArray("SonKuPik-K500/") + QCoreApplication::applicationVersion().toUtf8());
    request.setRawHeader("Accept", "application/octet-stream");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(120000);
    return request;
}

QString normalizedVersion(QString version)
{
    version = version.trimmed();
    if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        version.remove(0, 1);
    return version;
}
}

AppUpdateManager::AppUpdateManager(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"SonKuPikK500.K500.Native");
    m_appMutex = mutex;
#endif
}

AppUpdateManager::~AppUpdateManager()
{
#ifdef Q_OS_WIN
    if (m_appMutex)
        CloseHandle(static_cast<HANDLE>(m_appMutex));
#endif
}

QString AppUpdateManager::currentVersion() const
{
    return QCoreApplication::applicationVersion();
}

bool AppUpdateManager::busy() const
{
    return m_state == QStringLiteral("checking")
        || m_state == QStringLiteral("downloading")
        || m_state == QStringLiteral("verifying")
        || m_state == QStringLiteral("installing");
}

bool AppUpdateManager::managedInstall() const
{
#ifdef Q_OS_WIN
    // The installer writes this machine-level identity. Portable ZIP builds do
    // not, so they never silently convert themselves into installed software.
    QSettings registry(
        QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\MasArray\\SonKuPik K500"),
        QSettings::NativeFormat);
    const QString installDir = QDir::cleanPath(registry.value(QStringLiteral("InstallDir")).toString());
    const QString appDir = QDir::cleanPath(QCoreApplication::applicationDirPath());
    return !installDir.isEmpty()
        && installDir.compare(appDir, Qt::CaseInsensitive) == 0;
#else
    return false;
#endif
}

void AppUpdateManager::setState(const QString &state,
                                const QString &status,
                                const QString &error)
{
    const bool changed = m_state != state
        || m_statusText != status
        || m_errorMessage != error;
    m_state = state;
    m_statusText = status;
    m_errorMessage = error;
    if (changed)
        emit stateChanged();
}

void AppUpdateManager::setProgress(qint64 received, qint64 total)
{
    m_downloadedBytes = qMax<qint64>(0, received);
    m_totalBytes = qMax<qint64>(0, total);
    m_progress = m_totalBytes > 0
        ? qBound(0.0, double(m_downloadedBytes) / double(m_totalBytes), 1.0)
        : 0.0;
    emit progressChanged();
}

void AppUpdateManager::clearMetadata()
{
    m_latestVersion.clear();
    m_releaseNotes.clear();
    m_manifestUrl = {};
    m_installerUrl = {};
    m_installerName.clear();
    m_expectedSha256.clear();
    m_releaseAssetDigest.clear();
    m_expectedBytes = -1;
    m_installerPath.clear();
    emit updateMetadataChanged();
}

bool AppUpdateManager::automaticCheckSuppressedByArguments() const
{
    const QStringList args = QCoreApplication::arguments();
    for (const QString &arg : args) {
        if (arg == QStringLiteral("--font-self-test")
            || arg == QStringLiteral("--protocol-self-test")
            || arg == QStringLiteral("--engine-self-test")
            || arg.startsWith(QStringLiteral("--update-self-test"))) {
            return true;
        }
    }
    return false;
}

void AppUpdateManager::startAutomaticCheck()
{
    // A failed/cancelled elevated handoff restarts the previous app with a tiny
    // result token. Surface it after QML Connections are alive rather than
    // leaving a novice wondering why the update apparently vanished.
    for (const QString &arg : QCoreApplication::arguments()) {
        if (arg.startsWith(QStringLiteral("--update-failed="))) {
            QTimer::singleShot(350, this, [this, arg] {
                const QString code = arg.section(QLatin1Char('='), 1);
                setState(QStringLiteral("error"),
                         QStringLiteral("Update belum terpasang."),
                         QStringLiteral("Windows membatalkan atau gagal memasang update (kode %1). Versi sebelumnya tetap aman.").arg(code));
                emit attentionRequired();
            });
            return;
        }
    }

    if (automaticCheckSuppressedByArguments() || !managedInstall())
        return;

    const QDateTime lastCheck = QSettings().value(QString::fromLatin1(LastCheckKey)).toDateTime();
    const bool due = !lastCheck.isValid()
        || lastCheck.secsTo(QDateTime::currentDateTimeUtc()) >= AutomaticCheckIntervalSecs;
    if (!due)
        return;

    QTimer::singleShot(2500, this, [this] {
        if (!busy())
            checkForUpdates(false);
    });
}

void AppUpdateManager::checkForUpdates(bool userInitiated)
{
    if (!managedInstall()) {
        if (userInitiated) {
            setState(QStringLiteral("error"),
                     QStringLiteral("Update otomatis tersedia untuk versi terinstal."),
                     QStringLiteral("Build portable tidak diubah diam-diam. Gunakan installer stable sekali untuk mengaktifkan update otomatis."));
            emit attentionRequired();
        }
        return;
    }

    if (m_state == QStringLiteral("downloading")
        || m_state == QStringLiteral("verifying")
        || m_state == QStringLiteral("installing")) {
        return;
    }

    if (m_metadataReply)
        m_metadataReply->abort();
    if (m_manifestReply)
        m_manifestReply->abort();

    clearMetadata();
    setProgress(0, 0);
    m_userInitiatedCheck = userInitiated;
    setState(QStringLiteral("checking"), QStringLiteral("Memeriksa update stabil terbaru…"));

    QNetworkReply *reply = m_network.get(jsonRequest(LatestReleaseApi));
    m_metadataReply = reply;
    connect(reply, &QNetworkReply::redirected, this, [this, reply](const QUrl &url) {
        if (!trustedRedirectUrl(url)) {
            reply->abort();
            fail(QStringLiteral("Update metadata dialihkan ke host yang tidak dipercaya."), m_userInitiatedCheck);
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        handleLatestRelease(reply);
        reply->deleteLater();
        if (m_metadataReply == reply)
            m_metadataReply = nullptr;
    });
}

void AppUpdateManager::handleLatestRelease(QNetworkReply *reply)
{
    if (!reply)
        return;
    if (reply->error() != QNetworkReply::NoError) {
        fail(QStringLiteral("Tidak dapat memeriksa update: %1").arg(reply->errorString()),
             m_userInitiatedCheck);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        fail(QStringLiteral("Metadata release GitHub tidak valid."), m_userInitiatedCheck);
        return;
    }

    const QJsonObject release = document.object();
    if (release.value(QStringLiteral("draft")).toBool()
        || release.value(QStringLiteral("prerelease")).toBool()) {
        fail(QStringLiteral("Release terbaru bukan channel stable."), m_userInitiatedCheck);
        return;
    }

    const QString version = normalizedVersion(release.value(QStringLiteral("tag_name")).toString());
    const QVersionNumber latest = QVersionNumber::fromString(version);
    const QVersionNumber current = QVersionNumber::fromString(normalizedVersion(currentVersion()));
    if (version.isEmpty() || latest.isNull()) {
        fail(QStringLiteral("Versi release terbaru tidak dapat dibaca."), m_userInitiatedCheck);
        return;
    }

    QSettings().setValue(QString::fromLatin1(LastCheckKey), QDateTime::currentDateTimeUtc());

    if (!current.isNull() && QVersionNumber::compare(latest, current) <= 0) {
        if (m_userInitiatedCheck) {
            setState(QStringLiteral("upToDate"),
                     QStringLiteral("SonKuPik K500 sudah versi terbaru."));
            emit attentionRequired();
        } else {
            setState(QStringLiteral("idle"));
        }
        return;
    }

    m_latestVersion = version;
    m_releaseNotes = compactReleaseNotes(release.value(QStringLiteral("body")).toString());
    const QString expectedInstaller = QStringLiteral("SonKuPik-K500-v%1-Windows-Setup.exe").arg(version);

    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
        if (name == expectedInstaller) {
            m_installerName = name;
            m_installerUrl = url;
            m_releaseAssetDigest = asset.value(QStringLiteral("digest")).toString().toLower();
        } else if (name == QStringLiteral("release-manifest.json")) {
            m_manifestUrl = url;
        }
    }

    if (m_installerUrl.isEmpty() || m_manifestUrl.isEmpty()
        || !trustedReleaseAssetUrl(m_installerUrl, version)
        || !trustedReleaseAssetUrl(m_manifestUrl, version)) {
        fail(QStringLiteral("Release terbaru tidak memiliki paket installer/manifest resmi yang valid."),
             m_userInitiatedCheck);
        return;
    }

    emit updateMetadataChanged();
    fetchManifest();
}

void AppUpdateManager::fetchManifest()
{
    setState(QStringLiteral("checking"), QStringLiteral("Memverifikasi metadata update…"));
    QNetworkReply *reply = m_network.get(assetRequest(m_manifestUrl));
    m_manifestReply = reply;
    connect(reply, &QNetworkReply::redirected, this, [this, reply](const QUrl &url) {
        if (!trustedRedirectUrl(url)) {
            reply->abort();
            fail(QStringLiteral("Manifest update dialihkan ke host yang tidak dipercaya."),
                 m_userInitiatedCheck);
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        handleManifest(reply);
        reply->deleteLater();
        if (m_manifestReply == reply)
            m_manifestReply = nullptr;
    });
}

bool AppUpdateManager::shouldNotifyAvailable() const
{
    if (m_userInitiatedCheck)
        return true;

    QSettings settings;
    if (settings.value(QString::fromLatin1(LastDismissedVersionKey)).toString() != m_latestVersion)
        return true;

    const QDateTime dismissed = settings.value(QString::fromLatin1(LastDismissedUtcKey)).toDateTime();
    return !dismissed.isValid()
        || dismissed.secsTo(QDateTime::currentDateTimeUtc()) >= DismissedReminderSecs;
}

void AppUpdateManager::handleManifest(QNetworkReply *reply)
{
    if (!reply)
        return;
    if (reply->error() != QNetworkReply::NoError) {
        fail(QStringLiteral("Tidak dapat mengambil release manifest: %1").arg(reply->errorString()),
             m_userInitiatedCheck);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        fail(QStringLiteral("Release manifest tidak valid."), m_userInitiatedCheck);
        return;
    }

    const QJsonObject manifest = document.object();
    const QString schema = manifest.value(QStringLiteral("schema")).toString();
    if (!schema.startsWith(QStringLiteral("sonkupik-k500-release-manifest-v"))
        || manifest.value(QStringLiteral("channel")).toString() != QStringLiteral("stable")
        || manifest.value(QStringLiteral("version")).toString() != m_latestVersion
        || manifest.value(QStringLiteral("target")).toString() != QStringLiteral("windows-x64")
        || !manifest.value(QStringLiteral("stableReleaseEligible")).toBool()) {
        fail(QStringLiteral("Release manifest gagal pemeriksaan stable-channel."), m_userInitiatedCheck);
        return;
    }

    QString sha256;
    qint64 bytes = -1;
    const QJsonArray artifacts = manifest.value(QStringLiteral("artifacts")).toArray();
    for (const QJsonValue &value : artifacts) {
        const QJsonObject artifact = value.toObject();
        if (artifact.value(QStringLiteral("file")).toString() == m_installerName) {
            sha256 = artifact.value(QStringLiteral("sha256")).toString().toLower();
            bytes = qint64(artifact.value(QStringLiteral("bytes")).toDouble(-1));
            break;
        }
    }

    if (sha256.size() != 64 || bytes <= 0) {
        fail(QStringLiteral("SHA-256/ukuran installer tidak tersedia di release manifest."),
             m_userInitiatedCheck);
        return;
    }

    const QString expectedDigest = QStringLiteral("sha256:%1").arg(sha256);
    if (m_releaseAssetDigest.isEmpty() || m_releaseAssetDigest != expectedDigest) {
        fail(QStringLiteral("Digest GitHub dan release manifest tidak cocok; update dibatalkan."),
             m_userInitiatedCheck);
        return;
    }

    m_expectedSha256 = sha256;
    m_expectedBytes = bytes;
    emit updateMetadataChanged();
    setState(QStringLiteral("available"),
             QStringLiteral("Update v%1 siap diunduh.").arg(m_latestVersion));
    if (shouldNotifyAvailable())
        emit updateAvailableFound();
}

void AppUpdateManager::downloadAndInstall()
{
    if (m_state != QStringLiteral("available")
        || m_installerUrl.isEmpty()
        || m_expectedSha256.size() != 64
        || m_expectedBytes <= 0) {
        return;
    }
    beginInstallerDownload();
}

void AppUpdateManager::beginInstallerDownload()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/.sonkupik-k500");
    QDir updateRoot(QDir(base).filePath(QStringLiteral("updates/v%1").arg(m_latestVersion)));
    if (!QDir().mkpath(updateRoot.absolutePath())) {
        fail(QStringLiteral("Folder update lokal tidak dapat dibuat."), true);
        return;
    }

    const QString finalPath = updateRoot.filePath(m_installerName);
    const QString partialPath = finalPath + QStringLiteral(".part");
    QFile::remove(partialPath);
    m_downloadFile.setFileName(partialPath);
    if (!m_downloadFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("File update lokal tidak dapat dibuat."), true);
        return;
    }

    m_installerPath = finalPath;
    m_downloadHash.reset();
    m_downloadWriteFailed = false;
    setProgress(0, m_expectedBytes);
    setState(QStringLiteral("downloading"),
             QStringLiteral("Mengunduh update v%1…").arg(m_latestVersion));

    QNetworkReply *reply = m_network.get(assetRequest(m_installerUrl));
    m_downloadReply = reply;
    connect(reply, &QNetworkReply::redirected, this, [this, reply](const QUrl &url) {
        if (!trustedRedirectUrl(url)) {
            m_downloadWriteFailed = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        const QByteArray chunk = reply->readAll();
        if (chunk.isEmpty())
            return;
        if (m_downloadFile.write(chunk) != chunk.size()) {
            m_downloadWriteFailed = true;
            reply->abort();
            return;
        }
        m_downloadHash.addData(chunk);
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
        setProgress(received, total > 0 ? total : m_expectedBytes);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        finishInstallerDownload(reply);
        reply->deleteLater();
        if (m_downloadReply == reply)
            m_downloadReply = nullptr;
    });
}

void AppUpdateManager::finishInstallerDownload(QNetworkReply *reply)
{
    if (!reply)
        return;

    const QByteArray tail = reply->readAll();
    if (!tail.isEmpty() && !m_downloadWriteFailed) {
        if (m_downloadFile.write(tail) != tail.size())
            m_downloadWriteFailed = true;
        else
            m_downloadHash.addData(tail);
    }
    m_downloadFile.flush();
    m_downloadFile.close();

    const QString partialPath = m_downloadFile.fileName();
    if (reply->error() != QNetworkReply::NoError || m_downloadWriteFailed) {
        QFile::remove(partialPath);
        fail(m_downloadWriteFailed
                 ? QStringLiteral("Update dibatalkan karena write/redirect validation gagal.")
                 : QStringLiteral("Download update gagal: %1").arg(reply->errorString()),
             true);
        return;
    }

    setState(QStringLiteral("verifying"), QStringLiteral("Memverifikasi SHA-256 installer…"));
    const QFileInfo info(partialPath);
    if (info.size() != m_expectedBytes) {
        QFile::remove(partialPath);
        fail(QStringLiteral("Ukuran installer tidak cocok dengan release manifest."), true);
        return;
    }

    const QString actualSha = QString::fromLatin1(m_downloadHash.result().toHex()).toLower();
    if (actualSha != m_expectedSha256) {
        QFile::remove(partialPath);
        fail(QStringLiteral("SHA-256 installer tidak cocok; update ditolak demi keamanan."), true);
        return;
    }

    QFile::remove(m_installerPath);
    if (!QFile::rename(partialPath, m_installerPath)) {
        QFile::remove(partialPath);
        fail(QStringLiteral("Installer terverifikasi tidak dapat dipindahkan ke lokasi final."), true);
        return;
    }

    setProgress(info.size(), info.size());
    setState(QStringLiteral("installing"),
             QStringLiteral("Installer terverifikasi. Menyiapkan update aman…"));
    QTimer::singleShot(180, this, &AppUpdateManager::handoffVerifiedInstaller);
}

void AppUpdateManager::handoffVerifiedInstaller()
{
#ifdef Q_OS_WIN
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString helperPath = QDir(appDir).filePath(QStringLiteral("SonKuPik-K500-Updater.exe"));
    const QString appPath = QCoreApplication::applicationFilePath();
    if (!QFileInfo::exists(m_installerPath) || !QFileInfo::exists(helperPath)) {
        fail(QStringLiteral("Komponen update helper tidak ditemukan; versi saat ini tidak diubah."), true);
        return;
    }

    const QStringList args{
        QStringLiteral("--installer"), m_installerPath,
        QStringLiteral("--app"), appPath,
        QStringLiteral("--sha256"), m_expectedSha256,
        QStringLiteral("--bytes"), QString::number(m_expectedBytes),
        QStringLiteral("--pid"), QString::number(QCoreApplication::applicationPid())
    };

    if (!QProcess::startDetached(helperPath, args, appDir)) {
        fail(QStringLiteral("Update helper tidak dapat dijalankan; versi saat ini tidak diubah."), true);
        return;
    }

    setState(QStringLiteral("installing"),
             QStringLiteral("Aplikasi akan ditutup, meminta izin Windows, memasang update, lalu restart otomatis…"));
    QTimer::singleShot(180, qApp, [] { QCoreApplication::quit(); });
#else
    fail(QStringLiteral("Auto-update installer saat ini hanya tersedia di Windows."), true);
#endif
}

void AppUpdateManager::dismiss()
{
    if (busy())
        return;
    if (!m_latestVersion.isEmpty()) {
        QSettings settings;
        settings.setValue(QString::fromLatin1(LastDismissedVersionKey), m_latestVersion);
        settings.setValue(QString::fromLatin1(LastDismissedUtcKey), QDateTime::currentDateTimeUtc());
    }
    setState(QStringLiteral("idle"));
}

void AppUpdateManager::fail(const QString &message, bool userVisible)
{
    if (userVisible) {
        setState(QStringLiteral("error"), QStringLiteral("Update tidak dapat dilanjutkan."), message);
        emit attentionRequired();
    } else {
        setState(QStringLiteral("idle"));
    }
}

bool AppUpdateManager::trustedReleaseAssetUrl(const QUrl &url, const QString &version)
{
    if (!url.isValid()
        || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0
        || url.host().compare(QStringLiteral("github.com"), Qt::CaseInsensitive) != 0) {
        return false;
    }
    const QString prefix = QStringLiteral("/masarray/k500/releases/download/v%1/").arg(version);
    return url.path().startsWith(prefix);
}

bool AppUpdateManager::trustedRedirectUrl(const QUrl &url)
{
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0)
        return false;
    const QString host = url.host().toLower();
    return host == QStringLiteral("api.github.com")
        || host == QStringLiteral("github.com")
        || host == QStringLiteral("release-assets.githubusercontent.com")
        || host == QStringLiteral("objects.githubusercontent.com");
}

QString AppUpdateManager::compactReleaseNotes(const QString &body)
{
    QString text = body;
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    while (text.contains(QStringLiteral("\n\n\n")))
        text.replace(QStringLiteral("\n\n\n"), QStringLiteral("\n\n"));
    text = text.trimmed();
    constexpr int MaxChars = 1200;
    if (text.size() > MaxChars)
        text = text.left(MaxChars).trimmed() + QStringLiteral("…");
    return text;
}
