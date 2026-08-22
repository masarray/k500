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
    void hydrateFromDeviceMemory(const QByteArray &memory);
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

    struct CrossoverState {
        double hpfHz = 20.0;
        double lpfHz = 20000.0;
        QString hpType = QStringLiteral("HP Butter 12");
        QString lpType = QStringLiteral("LP Butter 12");
    };

    void queueEqFrame(const QString &key, const QByteArray &frame, const QString &label);
    void queueBlockFrame(const QString &key, const QByteArray &frame, const QString &label);
    void flushEqFrames();
    void flushBlockFrames();

    void queueTopMusic(const QString &path);
    void queueTopMic(const QString &path);
    void queueTopEffect(const QString &path);
    void queueOutput(const QString &section, const QString &path);
    void queueCrossover(const QString &section, const QString &path, const QString &kind);
    bool updateOutputState(const QString &section, const QString &field, const QVariant &value);

    static constexpr int EqSendIntervalMs = 45;
    static constexpr int BlockSendIntervalMs = 55;

    bool m_liveEnabled = false;
    QByteArray m_deviceScalars;
    QByteArray m_activeMemory;

    K500MusicBlockState m_music;
    K500MicBlockState m_mic;
    K500EffectBlockState m_effect;
    QHash<QString, K500OutputBlockState> m_outputs;
    QHash<QString, QByteArray> m_outputRaw;
    QHash<QString, CrossoverState> m_crossovers;

    QHash<QString, PendingFrame> m_pendingEqFrames;
    QHash<QString, PendingFrame> m_pendingBlockFrames;
    QTimer m_eqTimer;
    QTimer m_blockTimer;
    QElapsedTimer m_lastEqFlush;
    QElapsedTimer m_lastBlockFlush;
};
