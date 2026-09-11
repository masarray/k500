#pragma once

#include "RuntimeMetrics.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QString>
#include <QTimer>
#include <QtGlobal>

class K500Controller;
class K500DeviceManager;

// FINAL_DEVICE_PERFORMANCE_V1
// Opt-in physical-device qualification monitor. It is dormant in normal runs
// and activates only with --device-perf or --device-perf-report=<path>.
// The monitor observes public DeviceManager/Controller signals only; it never
// owns transport state and never changes K500 protocol or transaction timing.
class DevicePerformanceMonitor final
{
public:
    DevicePerformanceMonitor(K500DeviceManager *manager, K500Controller *controller);

    bool active() const { return m_active; }
    QString reportPath() const { return m_reportPath; }

private:
    void onStatusChanged();
    void onActiveMemoryReady(const QByteArray &memory);
    void onLogLine(const QString &direction, const QString &label, const QString &hex);
    void onControllerFrameReady(const QString &label);
    void sampleEventLoop();
    void sampleResource();
    void flushReport();

    void recordConnectMs(qint64 value);
    void recordReadbackMs(qint64 value);
    void recordControllerToTxUs(qint64 value);

    K500DeviceManager *m_manager = nullptr;
    K500Controller *m_controller = nullptr;
    bool m_active = false;
    QString m_reportPath;
    QString m_startedUtc;

    QElapsedTimer m_bootstrapTimer;
    QElapsedTimer m_connectTimer;
    QElapsedTimer m_readbackTimer;
    QElapsedTimer m_eventLoopClock;
    QElapsedTimer m_controllerToTxTimer;
    bool m_controllerFramePending = false;
    QString m_pendingControllerLabel;

    QTimer m_eventLoopTimer;
    QTimer m_resourceTimer;
    QTimer m_flushTimer;

    qint64 m_bootstrapToEventLoopMs = -1;

    quint64 m_connectAttempts = 0;
    quint64 m_successfulConnections = 0;
    quint64 m_disconnectEvents = 0;
    quint64 m_errorStatusEvents = 0;
    qint64 m_connectLastMs = -1;
    qint64 m_connectBestMs = -1;
    qint64 m_connectWorstMs = -1;
    qint64 m_connectTotalMs = 0;

    quint64 m_readbackAttempts = 0;
    quint64 m_readbackCompletions = 0;
    qint64 m_readbackLastMs = -1;
    qint64 m_readbackBestMs = -1;
    qint64 m_readbackWorstMs = -1;
    qint64 m_readbackTotalMs = 0;

    quint64 m_txLogFrames = 0;
    quint64 m_rxLogFrames = 0;
    quint64 m_errorLogLines = 0;
    quint64 m_controllerFrames = 0;
    quint64 m_writeDeferred = 0;
    quint64 m_unsupportedPaths = 0;

    quint64 m_controllerToTxSamples = 0;
    qint64 m_controllerToTxLastUs = -1;
    qint64 m_controllerToTxMaxUs = 0;
    qint64 m_controllerToTxTotalUs = 0;

    quint64 m_eventLoopSamples = 0;
    qint64 m_eventLoopLagTotalMs = 0;
    qint64 m_eventLoopLagMaxMs = 0;
    quint64 m_eventLoopOver100Ms = 0;
    quint64 m_eventLoopOver250Ms = 0;

    RuntimeHealthSnapshot m_startupSnapshot;
    RuntimeHealthSnapshot m_qualificationBaseline;
    RuntimeHealthSnapshot m_currentSnapshot;
    bool m_qualificationBaselineSet = false;
    qint64 m_maxPrivateBytesAfterBaseline = -1;
    quint32 m_maxHandlesAfterBaseline = 0;
    quint32 m_maxThreadsAfterBaseline = 0;
};
