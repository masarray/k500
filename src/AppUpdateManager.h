#pragma once

#include <QCryptographicHash>
#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class QNetworkReply;

class AppUpdateManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateMetadataChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY updateMetadataChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(qint64 downloadedBytes READ downloadedBytes NOTIFY progressChanged)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY progressChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)

public:
    explicit AppUpdateManager(QObject *parent = nullptr);
    ~AppUpdateManager() override;

    QString state() const { return m_state; }
    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseNotes() const { return m_releaseNotes; }
    QString statusText() const { return m_statusText; }
    QString errorMessage() const { return m_errorMessage; }
    double progress() const { return m_progress; }
    qint64 downloadedBytes() const { return m_downloadedBytes; }
    qint64 totalBytes() const { return m_totalBytes; }
    bool busy() const;

    Q_INVOKABLE void startAutomaticCheck();
    Q_INVOKABLE void checkForUpdates(bool userInitiated = true);
    Q_INVOKABLE void downloadAndInstall();
    Q_INVOKABLE void dismiss();

signals:
    void stateChanged();
    void updateMetadataChanged();
    void progressChanged();
    void updateAvailableFound();
    void attentionRequired();

private:
    void setState(const QString &state, const QString &status = {}, const QString &error = {});
    void setProgress(qint64 received, qint64 total);
    void fail(const QString &message, bool userVisible);
    void handleLatestRelease(QNetworkReply *reply);
    void fetchManifest();
    void handleManifest(QNetworkReply *reply);
    void beginInstallerDownload();
    void finishInstallerDownload(QNetworkReply *reply);
    void launchVerifiedInstaller();
    void clearMetadata();

    static bool trustedReleaseAssetUrl(const QUrl &url, const QString &version);
    static bool trustedRedirectUrl(const QUrl &url);
    static QString compactReleaseNotes(const QString &body);

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_metadataReply;
    QPointer<QNetworkReply> m_manifestReply;
    QPointer<QNetworkReply> m_downloadReply;
    QFile m_downloadFile;
    QCryptographicHash m_downloadHash{QCryptographicHash::Sha256};

    QString m_state = QStringLiteral("idle");
    QString m_statusText;
    QString m_errorMessage;
    QString m_latestVersion;
    QString m_releaseNotes;
    QUrl m_manifestUrl;
    QUrl m_installerUrl;
    QString m_installerName;
    QString m_expectedSha256;
    QString m_releaseAssetDigest;
    qint64 m_expectedBytes = -1;
    qint64 m_downloadedBytes = 0;
    qint64 m_totalBytes = 0;
    double m_progress = 0.0;
    QString m_installerPath;
    bool m_userInitiatedCheck = false;
    bool m_downloadWriteFailed = false;
    void *m_appMutex = nullptr;
};
