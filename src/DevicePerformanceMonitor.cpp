#include "DevicePerformanceMonitor.h"

#include "k500/K500Controller.h"
#include "k500/K500DeviceManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>

#include <algorithm>

#ifndef SONKUPIK_GIT_SHA
#define SONKUPIK_GIT_SHA "unknown"
#endif

namespace {
constexpr int EventLoopSampleMs = 50;
constexpr int ResourceSampleMs = 2000;
constexpr int ReportFlushMs = 5000;

constexpr qint64 ConnectToLiveTargetMs = 5000;
constexpr qint64 ReadbackTargetMs = 3000;
constexpr qint64 ControllerToTxTargetUs = 5000;
constexpr qint64 PrivateGrowthGuardBytes = 8LL * 1024LL * 1024LL;
constexpr qint64 HandleGrowthGuard = 2;
constexpr qint64 ThreadGrowthGuard = 1;

QJsonObject snapshotJson(const RuntimeHealthSnapshot &snapshot)
{
    QJsonObject object;
    object.insert(QStringLiteral("valid"), snapshot.valid);
    object.insert(QStringLiteral("workingSetBytes"), double(snapshot.workingSetBytes));
    object.insert(QStringLiteral("privateBytes"), double(snapshot.privateBytes));
    object.insert(QStringLiteral("peakWorkingSetBytes"), double(snapshot.peakWorkingSetBytes));
    object.insert(QStringLiteral("handleCount"), double(snapshot.handleCount));
    object.insert(QStringLiteral("threadCount"), double(snapshot.threadCount));
    return object;
}

QJsonObject snapshotDeltaJson(const RuntimeHealthSnapshot &from,
                              const RuntimeHealthSnapshot &to)
{
    QJsonObject object;
    const bool valid = from.valid && to.valid;
    object.insert(QStringLiteral("valid"), valid);
    if (!valid)
        return object;

    object.insert(QStringLiteral("workingSetBytes"), double(to.workingSetBytes - from.workingSetBytes));
    object.insert(QStringLiteral("privateBytes"), double(to.privateBytes - from.privateBytes));
    object.insert(QStringLiteral("peakWorkingSetBytes"), double(to.peakWorkingSetBytes - from.peakWorkingSetBytes));
    object.insert(QStringLiteral("handleCount"), qint64(to.handleCount) - qint64(from.handleCount));
    object.insert(QStringLiteral("threadCount"), qint64(to.threadCount) - qint64(from.threadCount));
    return object;
}

QString reportPathFromArguments()
{
    const QString prefix = QStringLiteral("--device-perf-report=");
    const QStringList arguments = QCoreApplication::arguments();
    for (const QString &argument : arguments) {
        if (argument.startsWith(prefix, Qt::CaseInsensitive))
            return argument.mid(prefix.size()).trimmed();
    }
    return {};
}

bool performanceModeRequested()
{
    const QStringList arguments = QCoreApplication::arguments();
    for (const QString &argument : arguments) {
        if (argument.compare(QStringLiteral("--device-perf"), Qt::CaseInsensitive) == 0
            || argument.startsWith(QStringLiteral("--device-perf-report="), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}
}

DevicePerformanceMonitor::DevicePerformanceMonitor(K500DeviceManager *manager,
                                                   K500Controller *controller)
    : m_manager(manager), m_controller(controller)
{
    m_active = performanceModeRequested();
    if (!m_active || !m_manager)
        return;

    m_startedUtc = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    m_reportPath = reportPathFromArguments();
    if (m_reportPath.isEmpty()) {
        m_reportPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                     + QStringLiteral("/device-performance-latest.json");
    }

    const QFileInfo reportInfo(m_reportPath);
    QDir().mkpath(reportInfo.absolutePath());

    m_bootstrapTimer.start();

    QObject::connect(m_manager, &K500DeviceManager::statusChanged,
                     m_manager, [this]() { onStatusChanged(); });
    QObject::connect(m_manager, &K500DeviceManager::activeMemoryReady,
                     m_manager, [this](const QByteArray &memory) { onActiveMemoryReady(memory); });
    QObject::connect(m_manager, &K500DeviceManager::logLine,
                     m_manager,
                     [this](const QString &direction, const QString &label, const QString &hex) {
        onLogLine(direction, label, hex);
    });

    if (m_controller) {
        QObject::connect(m_controller, &K500Controller::frameReady,
                         m_manager,
                         [this](const QByteArray &, const QString &label) {
            onControllerFrameReady(label);
        });
        QObject::connect(m_controller, &K500Controller::writeDeferred,
                         m_manager,
                         [this](const QString &, const QString &) { ++m_writeDeferred; });
        QObject::connect(m_controller, &K500Controller::unsupportedPath,
                         m_manager,
                         [this](const QString &) { ++m_unsupportedPaths; });
    }

    m_eventLoopTimer.setTimerType(Qt::PreciseTimer);
    m_eventLoopTimer.setInterval(EventLoopSampleMs);
    QObject::connect(&m_eventLoopTimer, &QTimer::timeout,
                     m_manager, [this]() { sampleEventLoop(); });

    m_resourceTimer.setInterval(ResourceSampleMs);
    QObject::connect(&m_resourceTimer, &QTimer::timeout,
                     m_manager, [this]() { sampleResource(); });

    m_flushTimer.setInterval(ReportFlushMs);
    QObject::connect(&m_flushTimer, &QTimer::timeout,
                     m_manager, [this]() { flushReport(); });

    if (QCoreApplication::instance()) {
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         m_manager, [this]() { flushReport(); });
    }

    // Runs only after QML/module construction has completed and the event loop
    // begins processing. This is intentionally a bootstrap/QML-load indicator,
    // not a claim about full process-launch time.
    QTimer::singleShot(0, m_manager, [this]() {
        m_bootstrapToEventLoopMs = m_bootstrapTimer.elapsed();
        m_startupSnapshot = RuntimeMetrics::capture();
        m_currentSnapshot = m_startupSnapshot;
        m_eventLoopClock.start();
        m_eventLoopTimer.start();
        m_resourceTimer.start();
        m_flushTimer.start();
        flushReport();
    });

    qInfo().noquote() << "K500 final device performance qualification enabled ->"
                      << QDir::toNativeSeparators(m_reportPath);
}

void DevicePerformanceMonitor::onStatusChanged()
{
    if (!m_active || !m_manager)
        return;

    const QString status = m_manager->status();
    if (status == QStringLiteral("connecting")) {
        ++m_connectAttempts;
        m_connectTimer.restart();
        return;
    }

    if (status == QStringLiteral("connected")) {
        ++m_successfulConnections;
        if (m_connectTimer.isValid()) {
            recordConnectMs(m_connectTimer.elapsed());
            m_connectTimer.invalidate();
        }

        // The first fully connected/LIVE state is the steady-state resource
        // baseline. Reconnect/stress growth is compared against this state,
        // not against cold startup where USB handles are not open yet.
        if (!m_qualificationBaselineSet) {
            m_qualificationBaseline = RuntimeMetrics::capture();
            if (m_qualificationBaseline.valid) {
                m_currentSnapshot = m_qualificationBaseline;
                m_maxPrivateBytesAfterBaseline = m_qualificationBaseline.privateBytes;
                m_maxHandlesAfterBaseline = m_qualificationBaseline.handleCount;
                m_maxThreadsAfterBaseline = m_qualificationBaseline.threadCount;
            }
            m_qualificationBaselineSet = true;
        }
        flushReport();
        return;
    }

    if (status == QStringLiteral("disconnected")) {
        ++m_disconnectEvents;
        return;
    }

    if (status == QStringLiteral("error"))
        ++m_errorStatusEvents;
}

void DevicePerformanceMonitor::onActiveMemoryReady(const QByteArray &memory)
{
    if (!m_active || memory.size() < 0x03AB)
        return;

    ++m_readbackCompletions;
    if (m_readbackTimer.isValid()) {
        recordReadbackMs(m_readbackTimer.elapsed());
        m_readbackTimer.invalidate();
    }
}

void DevicePerformanceMonitor::onLogLine(const QString &direction,
                                         const QString &label,
                                         const QString &)
{
    if (!m_active)
        return;

    const QString normalizedDirection = direction.trimmed().toUpper();
    if (normalizedDirection == QStringLiteral("TX")) {
        ++m_txLogFrames;

        if (label.startsWith(QStringLiteral("Read 0x0000"), Qt::CaseInsensitive)) {
            ++m_readbackAttempts;
            m_readbackTimer.restart();
        }

        if (m_controllerFramePending) {
            if (label == m_pendingControllerLabel && m_controllerToTxTimer.isValid())
                recordControllerToTxUs(m_controllerToTxTimer.nsecsElapsed() / 1000);
            m_controllerFramePending = false;
            m_pendingControllerLabel.clear();
            m_controllerToTxTimer.invalidate();
        }
    } else if (normalizedDirection == QStringLiteral("RX")) {
        ++m_rxLogFrames;
    } else if (normalizedDirection == QStringLiteral("ERR")) {
        ++m_errorLogLines;
    }
}

void DevicePerformanceMonitor::onControllerFrameReady(const QString &label)
{
    if (!m_active)
        return;

    ++m_controllerFrames;
    m_pendingControllerLabel = label;
    m_controllerFramePending = true;
    m_controllerToTxTimer.restart();
}

void DevicePerformanceMonitor::sampleEventLoop()
{
    if (!m_active)
        return;

    if (!m_eventLoopClock.isValid()) {
        m_eventLoopClock.start();
        return;
    }

    const qint64 elapsedMs = m_eventLoopClock.restart();
    const qint64 lagMs = std::max<qint64>(0, elapsedMs - EventLoopSampleMs);
    ++m_eventLoopSamples;
    m_eventLoopLagTotalMs += lagMs;
    m_eventLoopLagMaxMs = std::max(m_eventLoopLagMaxMs, lagMs);
    if (elapsedMs > 100)
        ++m_eventLoopOver100Ms;
    if (elapsedMs > 250)
        ++m_eventLoopOver250Ms;
}

void DevicePerformanceMonitor::sampleResource()
{
    if (!m_active)
        return;

    const RuntimeHealthSnapshot snapshot = RuntimeMetrics::capture();
    if (!snapshot.valid)
        return;

    m_currentSnapshot = snapshot;
    if (!m_qualificationBaselineSet || !m_qualificationBaseline.valid)
        return;

    m_maxPrivateBytesAfterBaseline = std::max(m_maxPrivateBytesAfterBaseline,
                                               snapshot.privateBytes);
    m_maxHandlesAfterBaseline = std::max(m_maxHandlesAfterBaseline,
                                          snapshot.handleCount);
    m_maxThreadsAfterBaseline = std::max(m_maxThreadsAfterBaseline,
                                          snapshot.threadCount);
}

void DevicePerformanceMonitor::recordConnectMs(qint64 value)
{
    m_connectLastMs = value;
    m_connectBestMs = m_connectBestMs < 0 ? value : std::min(m_connectBestMs, value);
    m_connectWorstMs = std::max(m_connectWorstMs, value);
    m_connectTotalMs += value;
}

void DevicePerformanceMonitor::recordReadbackMs(qint64 value)
{
    m_readbackLastMs = value;
    m_readbackBestMs = m_readbackBestMs < 0 ? value : std::min(m_readbackBestMs, value);
    m_readbackWorstMs = std::max(m_readbackWorstMs, value);
    m_readbackTotalMs += value;
}

void DevicePerformanceMonitor::recordControllerToTxUs(qint64 value)
{
    ++m_controllerToTxSamples;
    m_controllerToTxLastUs = value;
    m_controllerToTxMaxUs = std::max(m_controllerToTxMaxUs, value);
    m_controllerToTxTotalUs += value;
}

void DevicePerformanceMonitor::flushReport()
{
    if (!m_active || m_reportPath.isEmpty())
        return;

    sampleResource();

    const qint64 privateGrowth = m_qualificationBaseline.valid && m_currentSnapshot.valid
        ? m_currentSnapshot.privateBytes - m_qualificationBaseline.privateBytes : 0;
    const qint64 handleGrowth = m_qualificationBaseline.valid && m_currentSnapshot.valid
        ? qint64(m_currentSnapshot.handleCount) - qint64(m_qualificationBaseline.handleCount) : 0;
    const qint64 threadGrowth = m_qualificationBaseline.valid && m_currentSnapshot.valid
        ? qint64(m_currentSnapshot.threadCount) - qint64(m_qualificationBaseline.threadCount) : 0;

    const bool connectionTimingPass = m_successfulConnections > 0
        && m_connectWorstMs >= 0 && m_connectWorstMs <= ConnectToLiveTargetMs;
    const bool readbackTimingPass = m_readbackCompletions > 0
        && m_readbackWorstMs >= 0 && m_readbackWorstMs <= ReadbackTargetMs;
    const bool controllerDispatchPass = m_controllerToTxSamples > 0
        && m_controllerToTxMaxUs <= ControllerToTxTargetUs;
    const bool uiStallPass = m_eventLoopOver250Ms == 0;
    const bool errorPass = m_errorLogLines == 0
        && m_errorStatusEvents == 0
        && m_unsupportedPaths == 0;
    const bool resourcePass = m_qualificationBaseline.valid && m_currentSnapshot.valid
        && privateGrowth <= PrivateGrowthGuardBytes
        && handleGrowth <= HandleGrowthGuard
        && threadGrowth <= ThreadGrowthGuard;

    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("sonkupik-k500-device-performance-v1"));
    root.insert(QStringLiteral("startedUtc"), m_startedUtc);
    root.insert(QStringLiteral("generatedUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    root.insert(QStringLiteral("application"), QCoreApplication::applicationName());
    root.insert(QStringLiteral("version"), QCoreApplication::applicationVersion());
    root.insert(QStringLiteral("gitCommit"), QString::fromLatin1(SONKUPIK_GIT_SHA));
    root.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
    root.insert(QStringLiteral("bootstrapToEventLoopMs"), double(m_bootstrapToEventLoopMs));

    QJsonObject platform;
    platform.insert(QStringLiteral("os"), QSysInfo::prettyProductName());
    platform.insert(QStringLiteral("kernelVersion"), QSysInfo::kernelVersion());
    platform.insert(QStringLiteral("cpuArchitecture"), QSysInfo::currentCpuArchitecture());
    root.insert(QStringLiteral("platform"), platform);

    QJsonObject device;
    if (m_manager) {
        device.insert(QStringLiteral("transport"), m_manager->transportMode());
        device.insert(QStringLiteral("status"), m_manager->status());
        device.insert(QStringLiteral("portLabel"), m_manager->portLabel());
        device.insert(QStringLiteral("connected"), m_manager->connected());
        device.insert(QStringLiteral("liveEnabled"), m_manager->liveEnabled());
        device.insert(QStringLiteral("lastError"), m_manager->lastError());
    }
    root.insert(QStringLiteral("device"), device);

    QJsonObject connectTiming;
    connectTiming.insert(QStringLiteral("attempts"), double(m_connectAttempts));
    connectTiming.insert(QStringLiteral("successes"), double(m_successfulConnections));
    connectTiming.insert(QStringLiteral("disconnectEvents"), double(m_disconnectEvents));
    connectTiming.insert(QStringLiteral("errorStatusEvents"), double(m_errorStatusEvents));
    connectTiming.insert(QStringLiteral("lastMs"), double(m_connectLastMs));
    connectTiming.insert(QStringLiteral("bestMs"), double(m_connectBestMs));
    connectTiming.insert(QStringLiteral("worstMs"), double(m_connectWorstMs));
    connectTiming.insert(QStringLiteral("averageMs"), m_successfulConnections > 0
                         ? double(m_connectTotalMs) / double(m_successfulConnections) : -1.0);
    connectTiming.insert(QStringLiteral("targetWorstMs"), double(ConnectToLiveTargetMs));
    root.insert(QStringLiteral("connectToLive"), connectTiming);

    QJsonObject readbackTiming;
    readbackTiming.insert(QStringLiteral("attempts"), double(m_readbackAttempts));
    readbackTiming.insert(QStringLiteral("completions"), double(m_readbackCompletions));
    readbackTiming.insert(QStringLiteral("lastMs"), double(m_readbackLastMs));
    readbackTiming.insert(QStringLiteral("bestMs"), double(m_readbackBestMs));
    readbackTiming.insert(QStringLiteral("worstMs"), double(m_readbackWorstMs));
    readbackTiming.insert(QStringLiteral("averageMs"), m_readbackCompletions > 0
                          ? double(m_readbackTotalMs) / double(m_readbackCompletions) : -1.0);
    readbackTiming.insert(QStringLiteral("targetWorstMs"), double(ReadbackTargetMs));
    root.insert(QStringLiteral("activeMemoryReadback939"), readbackTiming);

    QJsonObject traffic;
    traffic.insert(QStringLiteral("txLogFrames"), double(m_txLogFrames));
    traffic.insert(QStringLiteral("rxLogFrames"), double(m_rxLogFrames));
    traffic.insert(QStringLiteral("controllerFrames"), double(m_controllerFrames));
    traffic.insert(QStringLiteral("writeDeferred"), double(m_writeDeferred));
    traffic.insert(QStringLiteral("unsupportedPaths"), double(m_unsupportedPaths));
    traffic.insert(QStringLiteral("errorLogLines"), double(m_errorLogLines));
    root.insert(QStringLiteral("traffic"), traffic);

    QJsonObject dispatch;
    dispatch.insert(QStringLiteral("samples"), double(m_controllerToTxSamples));
    dispatch.insert(QStringLiteral("lastUs"), double(m_controllerToTxLastUs));
    dispatch.insert(QStringLiteral("maxUs"), double(m_controllerToTxMaxUs));
    dispatch.insert(QStringLiteral("averageUs"), m_controllerToTxSamples > 0
                    ? double(m_controllerToTxTotalUs) / double(m_controllerToTxSamples) : -1.0);
    dispatch.insert(QStringLiteral("targetMaxUs"), double(ControllerToTxTargetUs));
    dispatch.insert(QStringLiteral("meaning"),
                    QStringLiteral("Controller frameReady -> DeviceManager TX accepted/logged on GUI thread"));
    root.insert(QStringLiteral("controllerToTransportAcceptance"), dispatch);

    QJsonObject eventLoop;
    eventLoop.insert(QStringLiteral("sampleIntervalMs"), EventLoopSampleMs);
    eventLoop.insert(QStringLiteral("samples"), double(m_eventLoopSamples));
    eventLoop.insert(QStringLiteral("averageLagMs"), m_eventLoopSamples > 0
                     ? double(m_eventLoopLagTotalMs) / double(m_eventLoopSamples) : -1.0);
    eventLoop.insert(QStringLiteral("maxLagMs"), double(m_eventLoopLagMaxMs));
    eventLoop.insert(QStringLiteral("ticksOver100Ms"), double(m_eventLoopOver100Ms));
    eventLoop.insert(QStringLiteral("ticksOver250Ms"), double(m_eventLoopOver250Ms));
    root.insert(QStringLiteral("eventLoop"), eventLoop);

    QJsonObject runtime;
    runtime.insert(QStringLiteral("startupPostQml"), snapshotJson(m_startupSnapshot));
    runtime.insert(QStringLiteral("qualificationBaselineFirstConnected"), snapshotJson(m_qualificationBaseline));
    runtime.insert(QStringLiteral("current"), snapshotJson(m_currentSnapshot));
    runtime.insert(QStringLiteral("deltaFromQualificationBaseline"),
                   snapshotDeltaJson(m_qualificationBaseline, m_currentSnapshot));
    QJsonObject observedMax;
    observedMax.insert(QStringLiteral("privateBytes"), double(m_maxPrivateBytesAfterBaseline));
    observedMax.insert(QStringLiteral("handleCount"), double(m_maxHandlesAfterBaseline));
    observedMax.insert(QStringLiteral("threadCount"), double(m_maxThreadsAfterBaseline));
    runtime.insert(QStringLiteral("observedMaxAfterBaseline"), observedMax);
    root.insert(QStringLiteral("runtime"), runtime);

    QJsonObject thresholds;
    thresholds.insert(QStringLiteral("connectToLiveWorstMs"), double(ConnectToLiveTargetMs));
    thresholds.insert(QStringLiteral("readback939WorstMs"), double(ReadbackTargetMs));
    thresholds.insert(QStringLiteral("controllerToTxMaxUs"), double(ControllerToTxTargetUs));
    thresholds.insert(QStringLiteral("privateGrowthBytes"), double(PrivateGrowthGuardBytes));
    thresholds.insert(QStringLiteral("handleGrowth"), HandleGrowthGuard);
    thresholds.insert(QStringLiteral("threadGrowth"), ThreadGrowthGuard);
    thresholds.insert(QStringLiteral("eventLoopTicksOver250Ms"), 0);
    root.insert(QStringLiteral("performanceTargets"), thresholds);

    QJsonObject gates;
    gates.insert(QStringLiteral("connectionTimingPass"), connectionTimingPass);
    gates.insert(QStringLiteral("readbackTimingPass"), readbackTimingPass);
    gates.insert(QStringLiteral("controllerDispatchPass"), controllerDispatchPass);
    gates.insert(QStringLiteral("uiStallPass"), uiStallPass);
    gates.insert(QStringLiteral("errorAndUnsupportedPathPass"), errorPass);
    gates.insert(QStringLiteral("steadyStateResourcePass"), resourcePass);
    gates.insert(QStringLiteral("automatedPerformancePass"),
                 connectionTimingPass && readbackTimingPass && controllerDispatchPass
                 && uiStallPass && errorPass && resourcePass);
    gates.insert(QStringLiteral("scope"),
                 QStringLiteral("Performance-only. Functional/destructive/unplug acceptance remains manual."));
    root.insert(QStringLiteral("gates"), gates);

    QSaveFile file(m_reportPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning().noquote() << "Unable to write K500 device performance report:"
                             << QDir::toNativeSeparators(m_reportPath);
        return;
    }

    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size() || !file.commit()) {
        qWarning().noquote() << "Unable to commit K500 device performance report:"
                             << QDir::toNativeSeparators(m_reportPath);
    }
}
