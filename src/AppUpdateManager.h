#pragma once

#include <QObject>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

class QNetworkAccessManager;
class QNetworkReply;

// SMART_UPDATE_RUNTIME_V1
// App-owned update coordinator exposed to QML as a singleton. Keep this class
// non-final: Qt 6.8's generated QQmlElement wrapper derives from registered QML
// types during type registration.
class AppUpdateManager : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AppUpdater)
    QML_SINGLETON
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY updateChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)

public:
    explicit AppUpdateManager(QObject *parent = nullptr);

    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseNotes() const { return m_releaseNotes; }
    QString state() const { return m_state; }
    QString statusText() const { return m_statusText; }
    QString errorText() const { return m_errorText; }
    qreal progress() const { return m_progress; }
    bool updateAvailable() const { return m_updateAvailable; }
    bool busy() const;

    Q_INVOKABLE void checkForUpdates(bool userInitiated = false);
    Q_INVOKABLE void downloadAndInstall();
    Q_INVOKABLE void remindLater();
    Q_INVOKABLE void skipThisVersion();

    // UPDATE_METADATA_SELF_TEST_V1 — hardware/network-free release-integrity
    // regression used by Windows CI and packaged-runtime qualification.
    static bool selfTest(QString *error = nullptr);

signals:
    void updateChanged();
    void stateChanged();
    void progressChanged();
    void updateReadyToInstall();

private:
    void setState(const QString &state, const QString &status = {}, const QString &error = {});
    void setProgress(qreal value);
    void resetReleaseMetadata();
    void handleLatestRelease(const QByteArray &payload, bool userInitiated);
    void downloadManifest();
    void downloadChecksums();
    void downloadInstaller();
    bool validateManifest(const QByteArray &payload);
    bool parseExpectedChecksum(const QByteArray &payload);
    bool verifyInstaller(QString *error = nullptr) const;
    bool launchInstallerElevated(QString *error = nullptr);
    QString updateDirectory() const;
    QString installerPath() const;
    QNetworkReply *get(const QUrl &url);

    QNetworkAccessManager *m_network = nullptr;
    QString m_latestVersion;
    QString m_releaseNotes;
    QString m_state = QStringLiteral("idle");
    QString m_statusText;
    QString m_errorText;
    qreal m_progress = 0.0;
    bool m_updateAvailable = false;
    QString m_setupAssetName;
    QUrl m_setupAssetUrl;
    QUrl m_manifestUrl;
    QUrl m_checksumsUrl;
    QByteArray m_manifestSha256;
    QByteArray m_expectedSha256;
    qint64 m_manifestSetupBytes = -1;
    qint64 m_releaseSetupBytes = -1;
};
