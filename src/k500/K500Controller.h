#pragma once

#include "K500Protocol.h"
#include "K500CanonicalState.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariant>

// P0_NATIVE_CONTEXT_QUALIFICATION_TRIGGER_V1 — source-path trigger only; removed with P0 cleanup.
class K500Controller final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool liveEnabled READ liveEnabled WRITE setLiveEnabled NOTIFY liveEnabledChanged)
    Q_PROPERTY(bool deviceReadbackReady READ deviceReadbackReady NOTIFY deviceReadbackReadyChanged)
    Q_PROPERTY(qulonglong sessionEpoch READ sessionEpoch NOTIFY canonicalStateChanged)
    Q_PROPERTY(bool canonicalStateReady READ canonicalStateReady NOTIFY canonicalStateChanged)
    Q_PROPERTY(QString canonicalSnapshotSha256 READ canonicalSnapshotSha256 NOTIFY canonicalStateChanged)
    Q_PROPERTY(int desiredStateCount READ desiredStateCount NOTIFY canonicalStateChanged)
    Q_PROPERTY(int inFlightStateCount READ inFlightStateCount NOTIFY canonicalStateChanged)
    Q_PROPERTY(qulonglong authoritativeSnapshotGeneration READ authoritativeSnapshotGeneration NOTIFY canonicalStateChanged)
    Q_PROPERTY(int divergentDesiredStateCount READ divergentDesiredStateCount NOTIFY canonicalStateChanged)

public:
    explicit K500Controller(QObject *parent = nullptr);

    bool liveEnabled() const { return m_liveEnabled; }
    bool deviceReadbackReady() const { return m_deviceScalars.size() >= 0x40; }
    qulonglong sessionEpoch() const { return m_canonicalState.sessionEpoch(); }
    bool canonicalStateReady() const { return m_canonicalState.snapshotReady(); }
    QString canonicalSnapshotSha256() const { return m_canonicalState.snapshotSha256Hex(); }
    int desiredStateCount() const { return m_canonicalState.desiredCount(); }
    int inFlightStateCount() const { return m_canonicalState.inFlightCount(); }
    qulonglong authoritativeSnapshotGeneration() const { return m_canonicalState.snapshotGeneration(); }
    int divergentDesiredStateCount() const { return m_canonicalState.divergentDesiredCount(); }

public slots:
    void beginDeviceSession();
    void endDeviceSession();
    void setLiveEnabled(bool enabled);
    void setCommandPlanningPaused(bool paused);
    void setDeviceScalars(const QByteArray &scalars);
    void hydrateFromDeviceMemory(const QByteArray &memory);
    void reconcileFromDeviceMemory(const QByteArray &memory);
    void clearDeviceState();
    void handleStateEdit(const QString &path, const QVariant &value);
    bool markCommandDispatched(quint64 sessionEpoch, quint64 token);
    void handleCommandDispatchResult(quint64 sessionEpoch, quint64 token,
                                     const QString &path, bool accepted,
                                     const QString &reason);

signals:
    void liveEnabledChanged();
    void deviceReadbackReadyChanged();
    void canonicalStateChanged();
    void commandReady(quint64 sessionEpoch, quint64 token,
                      const QByteArray &frame, const QString &label,
                      const QString &path, const QString &coalescingKey);
    // Compatibility/diagnostic signal. DeviceManager no longer transports this
    // directly; commandReady() is the authoritative session-bound envelope.
    void frameReady(const QByteArray &frame, const QString &label);
    void writeDeferred(const QString &path, const QString &reason);
    void unsupportedPath(const QString &path);

private:
    struct PendingFrame {
        K500CanonicalState::CommandPlan command;
    };

    struct CrossoverState {
        double hpfHz = 20.0;
        double lpfHz = 20000.0;
        QString hpType = QStringLiteral("HP Butter 12");
        QString lpType = QStringLiteral("LP Butter 12");
    };

    void queueEqFrame(const QString &key, const QString &path,
                      const QByteArray &frame, const QString &label);
    void queueBlockFrame(const QString &key, const QString &path,
                         const QByteArray &frame, const QString &label);
    void flushEqFrames();
    void flushBlockFrames();

    void queueTopMusic(const QString &path);
    void queueTopMic(const QString &path);
    void queueTopEffect(const QString &path);
    void queueReverb(const QString &path);
    void queueEcho(const QString &path);
    void queueEqBypass(const QString &path);
    void queueOutput(const QString &section, const QString &path);
    void queueCrossover(const QString &section, const QString &path, const QString &kind);
    bool updateReverbState(const QString &field, const QVariant &value);
    bool updateEchoState(const QString &field, const QVariant &value);
    bool updateOutputState(const QString &section, const QString &field, const QVariant &value);
    void rejectUnsupported(const QString &path);
    void deferWrite(const QString &path, const QString &reason);
    void recordConfirmedState(const QByteArray &memory);
    void applyDeviceMemory(const QByteArray &memory, bool preserveDesiredIntent);

    static constexpr int EqSendIntervalMs = 45;
    static constexpr int BlockSendIntervalMs = 55;

    bool m_liveEnabled = false;
    bool m_commandPlanningPaused = false;
    QHash<QString, QVariant> m_deferredEdits;
    K500CanonicalState m_canonicalState;
    QByteArray m_deviceScalars;

    K500MusicBlockState m_music;
    K500MicBlockState m_mic;
    K500EffectBlockState m_effect;
    K500ReverbBlockState m_reverb;
    QByteArray m_reverbRaw;
    K500EchoBlockState m_echo;
    QByteArray m_echoRaw;
    K500EqBypassImage m_eqBypass;
    bool m_eqBypassReady = false;
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
