#pragma once

#include "K500Protocol.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariant>

class K500Controller final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool liveEnabled READ liveEnabled WRITE setLiveEnabled NOTIFY liveEnabledChanged)
    Q_PROPERTY(bool deviceReadbackReady READ deviceReadbackReady NOTIFY deviceReadbackReadyChanged)

public:
    explicit K500Controller(QObject *parent = nullptr);

    bool liveEnabled() const { return m_liveEnabled; }
    bool deviceReadbackReady() const { return m_deviceScalars.size() >= 0x40; }

public slots:
    void setLiveEnabled(bool enabled);
    void setDeviceScalars(const QByteArray &scalars);
    void clearDeviceState();
    void handleStateEdit(const QString &path, const QVariant &value);

signals:
    void liveEnabledChanged();
    void deviceReadbackReadyChanged();
    void frameReady(const QByteArray &frame, const QString &label);
    void writeDeferred(const QString &path, const QString &reason);
    void unsupportedPath(const QString &path);

private:
    struct PendingFrame {
        QByteArray frame;
        QString label;
    };

    void queueEqFrame(const QString &key, const QByteArray &frame, const QString &label);
    void queueBlockFrame(const QString &key, const QByteArray &frame, const QString &label);
    void flushEqFrames();
    void flushBlockFrames();
    void queueTopMusic(const QString &path);
    void queueMusicCrossover(const QString &path, const QString &kind);

    static constexpr int EqSendIntervalMs = 45;
    static constexpr int BlockSendIntervalMs = 55;

    bool m_liveEnabled = false;
    QByteArray m_deviceScalars;
    K500MusicBlockState m_music;
    double m_musicHpfHz = 20.0;
    double m_musicLpfHz = 20000.0;
    QString m_musicHpType = QStringLiteral("HP Butter 12");
    QString m_musicLpType = QStringLiteral("LP Butter 12");

    QHash<QString, PendingFrame> m_pendingEqFrames;
    QHash<QString, PendingFrame> m_pendingBlockFrames;
    QTimer m_eqTimer;
    QTimer m_blockTimer;
    QElapsedTimer m_lastEqFlush;
    QElapsedTimer m_lastBlockFlush;
};
