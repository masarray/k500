#include "K500DeviceManager.h"

#include "K500Controller.h"
#include "K500Frame.h"
#include "K500Protocol.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QSysInfo>

namespace {
constexpr quint16 K500UsbVendorId = 0x10C4;
constexpr quint16 K500UsbProductId = 0x0321;
constexpr int HeartbeatIntervalMs = 3200;
constexpr int ProbeTimeoutMs = 1350;
constexpr int HandshakeTimeoutMs = 2200;
constexpr int ReadbackTimeoutMs = 2600;
constexpr int ConnectionWatchdogMs = 12000;
constexpr int DiagnosticLogLimit = 240;
constexpr int AuthoritativeReconcileIdleMs = 700;
constexpr int AuthoritativeReconcileRetryMs = 120;

// Exact donor/native active-memory readback used by the web editor.
constexpr int ActiveMemorySize = 0x03AB;       // 0x0000..0x03AA = 939 bytes
constexpr int ActiveMemoryBlockSize = 0x003A;  // 58-byte CMD 0x40 chunks
constexpr int ActiveMemoryInterBlockMs = 35;

K500TransactionScheduler::Family schedulerFamilyForPath(const QString &path)
{
    // Only PEQ band writes used the old 45 ms EQ timer. Crossovers and
    // global bypass are complete-block traffic and retain 55 ms pacing.
    if (path.startsWith(QStringLiteral("eq."))
        && path.contains(QStringLiteral(".bands.")))
        return K500TransactionScheduler::Family::Eq;
    return K500TransactionScheduler::Family::Block;
}
}

K500DeviceManager::K500DeviceManager(K500Controller *controller, QObject *parent)
    : QObject(parent), m_controller(controller), m_io(this)
{
    m_responseTimer.setSingleShot(true);
    m_probeDelayTimer.setSingleShot(true);
    m_heartbeatTimer.setInterval(HeartbeatIntervalMs);
    m_reconciliationTimer.setSingleShot(true);
    m_schedulerTimer.setSingleShot(true);
    m_schedulerClock.start();

    connect(&m_io, &K500WinIo::bytesReceived,
            this, &K500DeviceManager::onBytesReceived);
    connect(&m_io, &K500WinIo::errorOccurred,
            this, &K500DeviceManager::onIoError);
    connect(&m_responseTimer, &QTimer::timeout,
            this, &K500DeviceManager::connectionTimeout);
    connect(&m_probeDelayTimer, &QTimer::timeout,
            this, &K500DeviceManager::sendProbeHeartbeat);
    connect(&m_heartbeatTimer, &QTimer::timeout,
            this, &K500DeviceManager::heartbeatTick);
    connect(&m_reconciliationTimer, &QTimer::timeout,
            this, &K500DeviceManager::startAuthoritativeReconciliation);
    connect(&m_schedulerTimer, &QTimer::timeout,
            this, &K500DeviceManager::dispatchScheduledCommands);
    connect(this, &K500DeviceManager::logLine, this,
            [this](const QString &direction, const QString &label, const QString &hex) {
        appendDiagnosticLine(direction, label, hex);
    });

    if (m_controller) {
        connect(m_controller, &K500Controller::commandReady,
                this, &K500DeviceManager::sendPlannedCommand);
        connect(this, &K500DeviceManager::commandDispatchResult,
                m_controller, &K500Controller::handleCommandDispatchResult);
        connect(this, &K500DeviceManager::deviceScalarsReady,
                m_controller, &K500Controller::setDeviceScalars);
        connect(this, &K500DeviceManager::activeMemoryReady,
                m_controller, &K500Controller::hydrateFromDeviceMemory);
        connect(this, &K500DeviceManager::reconciliationMemoryReady,
                m_controller, &K500Controller::reconcileFromDeviceMemory);
    }

    QSettings settings;
    const QString savedMode = settings.value(QStringLiteral("k500/transportMode"),
                                              QStringLiteral("bt")).toString().toLower();
    if (savedMode == QStringLiteral("usb"))
        m_transportMode = savedMode;
    m_lastKnownSerialPort = settings.value(QStringLiteral("k500/lastBtPort")).toString().toUpper();
}

void K500DeviceManager::setTransportMode(const QString &mode)
{
    const QString normalized = mode.trimmed().toLower() == QStringLiteral("usb")
        ? QStringLiteral("usb") : QStringLiteral("bt");
    if (m_transportMode == normalized)
        return;

    if (m_status != QStringLiteral("disconnected"))
        disconnectDevice();

    m_transportMode = normalized;
    QSettings().setValue(QStringLiteral("k500/transportMode"), m_transportMode);
    emit transportModeChanged();
}

void K500DeviceManager::toggleConnection()
{
    if (connected() || m_status == QStringLiteral("connecting")
        || m_status == QStringLiteral("syncing")) {
        disconnectDevice();
        return;
    }
    connectDevice();
}

void K500DeviceManager::connectDevice()
{
    resetConnectionState(false);
    if (m_controller) {
        m_controller->beginDeviceSession();
        m_transactionScheduler.beginSession(m_controller->sessionEpoch());
    }
    setError({});
    setStatus(QStringLiteral("connecting"));

    if (m_transportMode == QStringLiteral("usb"))
        beginUsbProbe();
    else
        beginBluetoothScan();
}

void K500DeviceManager::disconnectDevice()
{
    resetConnectionState(false);
    setError({});
    setPortLabel(QStringLiteral("No device"));
    setStatus(QStringLiteral("disconnected"));
}

void K500DeviceManager::sendPlayerCommand(const QString &command)
{
    if (!connected())
        return;

    if (!writeFrame(K500Protocol::playerCommand(command),
                    QStringLiteral("Player %1").arg(command)))
        return;

    // PLAYER_STATUS_CAPTURED_V1
    // Never toggle the UI optimistically. Query the same device status used by
    // the manufacturer app so play/pause reflects BT/MP3 truth, including
    // changes made outside SonKuPik.
    QTimer::singleShot(180, this, [this] {
        if (m_stage == Stage::Ready && connected())
            writeFrame(K500Protocol::heartbeat(), QStringLiteral("Player status refresh"));
    });
}

void K500DeviceManager::toggleMute()
{
    if (!connected())
        return;
    const bool next = !m_muted;
    if (writeFrame(K500Protocol::mute(next), next ? QStringLiteral("Mute ON")
                                                  : QStringLiteral("Mute OFF")))
        setMuted(next);
}

void K500DeviceManager::sendLiveFrame(const QByteArray &frame, const QString &label)
{
    if (!connected() || !m_liveEnabled || frame.isEmpty())
        return;
    writeFrame(frame, label);
}


void K500DeviceManager::sendPlannedCommand(quint64 sessionEpoch, quint64 token,
                                            const QByteArray &frame, const QString &label,
                                            const QString &path, const QString &coalescingKey)
{
    // P2_SCHEDULER_TRANSPORT_BRIDGE_V1 — every canonical live command crosses
    // the bounded deterministic scheduler before entering K500WinIo. The worker
    // remains asynchronous; this GUI-thread scheduler only owns ordering/pacing.
    if (!m_controller || sessionEpoch != m_controller->sessionEpoch()
        || !m_transactionScheduler.sessionActive()
        || sessionEpoch != m_transactionScheduler.sessionEpoch()) {
        emit commandDispatchResult(sessionEpoch, token, path, false,
                                   QStringLiteral("Stale command rejected after device-session change"));
        return;
    }
    if (!connected() || !m_liveEnabled || frame.isEmpty()) {
        emit commandDispatchResult(sessionEpoch, token, path, false,
                                   QStringLiteral("Native transport is not LIVE for this session"));
        return;
    }

    K500TransactionScheduler::Transaction transaction;
    transaction.sessionEpoch = sessionEpoch;
    transaction.token = token;
    transaction.frame = frame;
    transaction.label = label;
    transaction.semanticPath = path;
    transaction.coalescingKey = coalescingKey;
    transaction.family = schedulerFamilyForPath(path);
    transaction.priority = K500TransactionScheduler::Priority::Interactive;

    const auto result = m_transactionScheduler.enqueue(transaction, schedulerNowMs());
    rejectDroppedTransactions(QStringLiteral("Native scheduler dropped queued command under bounded back-pressure/expiry"));

    if (result == K500TransactionScheduler::EnqueueResult::RejectedInvalid
        || result == K500TransactionScheduler::EnqueueResult::RejectedStaleSession
        || result == K500TransactionScheduler::EnqueueResult::RejectedBackpressure) {
        emit commandDispatchResult(
            sessionEpoch, token, path, false,
            QStringLiteral("Native scheduler rejected command: %1")
                .arg(QString::fromLatin1(K500TransactionScheduler::enqueueResultName(result))));
        return;
    }

    armTransactionScheduler();
}

qint64 K500DeviceManager::schedulerNowMs() const
{
    return m_schedulerClock.isValid() ? m_schedulerClock.elapsed() : 0;
}

void K500DeviceManager::armTransactionScheduler()
{
    if (!m_transactionScheduler.sessionActive() || m_transactionScheduler.empty()
        || !connected() || !m_liveEnabled) {
        m_schedulerTimer.stop();
        return;
    }

    const qint64 delay = m_transactionScheduler.nextWakeDelayMs(schedulerNowMs());
    if (delay < 0) {
        m_schedulerTimer.stop();
        return;
    }
    m_schedulerTimer.start(static_cast<int>(qMin<qint64>(delay, 2000)));
}

void K500DeviceManager::dispatchScheduledCommands()
{
    m_schedulerTimer.stop();
    if (!m_controller || !connected() || !m_liveEnabled
        || !m_transactionScheduler.sessionActive()) {
        cancelScheduledTransactions(QStringLiteral("Native transport left LIVE state before scheduler dispatch"));
        return;
    }

    for (;;) {
        const auto transaction = m_transactionScheduler.takeReady(schedulerNowMs());
        rejectDroppedTransactions(QStringLiteral("Native scheduler expired queued command before dispatch"));
        if (!transaction)
            break;

        if (transaction->sessionEpoch != m_controller->sessionEpoch()
            || !m_controller->markCommandDispatched(transaction->sessionEpoch,
                                                     transaction->token)) {
            emit commandDispatchResult(transaction->sessionEpoch, transaction->token,
                                       transaction->semanticPath, false,
                                       QStringLiteral("Command was superseded before scheduler dispatch"));
            continue;
        }

        const qint64 dispatchMs = schedulerNowMs();
        m_transactionScheduler.noteDispatched(*transaction, dispatchMs);
        const bool accepted = writeFrame(transaction->frame, transaction->label);
        emit commandDispatchResult(
            transaction->sessionEpoch, transaction->token, transaction->semanticPath, accepted,
            accepted ? QString{} : QStringLiteral("Native asynchronous transport rejected command"));

        // P3_1_NONDISRUPTIVE_LIVE_EDIT_V1
        // Never force a full 939-byte reconciliation after an ordinary live
        // control edit. The P3 hardware test proved that doing so made every
        // settled edit visibly leave LIVE/SYNC and could drop edits during the
        // verification window. DesiredState remains unresolved until an
        // explicit/session-bound authoritative refresh (connect/reconnect/Recall
        // or a future dedicated Verify action). Transport acceptance is still
        // NOT treated as hardware confirmation.
        if (!accepted || !connected() || !m_liveEnabled)
            break;
    }

    if (connected() && m_liveEnabled)
        armTransactionScheduler();
}

void K500DeviceManager::rejectDroppedTransactions(const QString &reason)
{
    const auto dropped = m_transactionScheduler.takeDropped();
    for (const auto &transaction : dropped) {
        emit commandDispatchResult(transaction.sessionEpoch, transaction.token,
                                   transaction.semanticPath, false, reason);
    }
}

void K500DeviceManager::cancelScheduledTransactions(const QString &reason)
{
    m_schedulerTimer.stop();
    m_transactionScheduler.clearQueued();
    rejectDroppedTransactions(reason);
}

QByteArray K500DeviceManager::supportReportJson() const
{
    // P5_SUPPORT_DIAGNOSTICS_V1
    // Deliberately omit active memory, preset bytes and PC preset paths. Raw
    // lastRx/lastTx are for the local live UI only; the exported values below
    // are populated exclusively by appendDiagnosticLine() after redaction.
    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("sonkupik-k500-support-report-v1"));
    root.insert(QStringLiteral("generatedUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    root.insert(QStringLiteral("application"), QCoreApplication::applicationName());
    root.insert(QStringLiteral("version"), QCoreApplication::applicationVersion());
    root.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));

    QJsonObject platform;
    platform.insert(QStringLiteral("os"), QSysInfo::prettyProductName());
    platform.insert(QStringLiteral("kernelType"), QSysInfo::kernelType());
    platform.insert(QStringLiteral("kernelVersion"), QSysInfo::kernelVersion());
    platform.insert(QStringLiteral("cpuArchitecture"), QSysInfo::currentCpuArchitecture());
    platform.insert(QStringLiteral("buildCpuArchitecture"), QSysInfo::buildCpuArchitecture());
    root.insert(QStringLiteral("platform"), platform);

    QJsonObject device;
    device.insert(QStringLiteral("transport"), m_transportMode);
    device.insert(QStringLiteral("status"), m_status);
    device.insert(QStringLiteral("portLabel"), m_portLabel);
    device.insert(QStringLiteral("connected"), connected());
    device.insert(QStringLiteral("liveEnabled"), m_liveEnabled);
    device.insert(QStringLiteral("playing"), m_playing);
    device.insert(QStringLiteral("muted"), m_muted);
    device.insert(QStringLiteral("lastError"), m_lastError);
    device.insert(QStringLiteral("lastTx"), m_lastTxDiagnostic);
    device.insert(QStringLiteral("lastRx"), m_lastRxDiagnostic);
    root.insert(QStringLiteral("device"), device);

    const auto schedulerTelemetry = m_transactionScheduler.telemetry();
    QJsonObject scheduler;
    scheduler.insert(QStringLiteral("sessionEpoch"), QString::number(m_transactionScheduler.sessionEpoch()));
    scheduler.insert(QStringLiteral("queued"), schedulerTelemetry.queued);
    scheduler.insert(QStringLiteral("peakQueued"), schedulerTelemetry.peakQueued);
    scheduler.insert(QStringLiteral("enqueued"), QString::number(schedulerTelemetry.enqueued));
    scheduler.insert(QStringLiteral("coalesced"), QString::number(schedulerTelemetry.coalesced));
    scheduler.insert(QStringLiteral("evicted"), QString::number(schedulerTelemetry.evicted));
    scheduler.insert(QStringLiteral("expired"), QString::number(schedulerTelemetry.expired));
    scheduler.insert(QStringLiteral("rejectedBackpressure"), QString::number(schedulerTelemetry.rejectedBackpressure));
    scheduler.insert(QStringLiteral("rejectedStale"), QString::number(schedulerTelemetry.rejectedStale));
    scheduler.insert(QStringLiteral("dispatched"), QString::number(schedulerTelemetry.dispatched));
    root.insert(QStringLiteral("transactionScheduler"), scheduler);

    QJsonObject canonical;
    if (m_controller) {
        canonical.insert(QStringLiteral("snapshotGeneration"),
                         QString::number(m_controller->authoritativeSnapshotGeneration()));
        canonical.insert(QStringLiteral("desired"), m_controller->desiredStateCount());
        canonical.insert(QStringLiteral("inFlight"), m_controller->inFlightStateCount());
        canonical.insert(QStringLiteral("divergentDesired"),
                         m_controller->divergentDesiredStateCount());
    }
    canonical.insert(QStringLiteral("reconciliationInProgress"), m_reconciliationInProgress);
    root.insert(QStringLiteral("canonicalState"), canonical);

    QJsonObject presetOperation;
    if (m_presetManager) {
        presetOperation.insert(QStringLiteral("busy"), m_presetManager->property("busy").toBool());
        presetOperation.insert(QStringLiteral("activeSlot"), m_presetManager->property("activeSlot").toInt());
        presetOperation.insert(QStringLiteral("useInitVolumeKnown"),
                               m_presetManager->property("useInitVolumeKnown").toBool());
        presetOperation.insert(QStringLiteral("useInitVolume"),
                               m_presetManager->property("useInitVolume").toBool());
        presetOperation.insert(QStringLiteral("progress"), m_presetManager->property("progress").toString());
    }
    root.insert(QStringLiteral("presetOperation"), presetOperation);

    QJsonObject presetFile;
    if (m_presetFileBridge) {
        presetFile.insert(QStringLiteral("loaded"), m_presetFileBridge->property("loaded").toBool());
        presetFile.insert(QStringLiteral("checksumOk"), m_presetFileBridge->property("checksumOk").toBool());
        presetFile.insert(QStringLiteral("dirty"), m_presetFileBridge->property("dirty").toBool());
        presetFile.insert(QStringLiteral("changedByteCount"), m_presetFileBridge->property("changedByteCount").toInt());
    }
    root.insert(QStringLiteral("presetFile"), presetFile);

    QJsonArray log;
    for (const QString &line : m_diagnosticLog)
        log.append(line);
    root.insert(QStringLiteral("protocolLog"), log);

    QJsonObject privacy;
    privacy.insert(QStringLiteral("activeMemoryIncluded"), false);
    privacy.insert(QStringLiteral("presetBytesIncluded"), false);
    privacy.insert(QStringLiteral("presetPathsIncluded"), false);
    privacy.insert(QStringLiteral("payloadRedaction"),
                   QStringLiteral("RSP 0xBF active-memory payloads and Store TX payloads are redacted"));
    privacy.insert(QStringLiteral("maxProtocolLogLines"), DiagnosticLogLimit);
    root.insert(QStringLiteral("privacy"), privacy);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool K500DeviceManager::saveSupportReport(const QUrl &url)
{
    QString path = url.isLocalFile() ? url.toLocalFile() : url.toString(QUrl::PreferLocalFile);
    path = QUrl::fromPercentEncoding(path.toUtf8());
    if (path.isEmpty()) {
        setError(QStringLiteral("Lokasi support report tidak valid."));
        return false;
    }
    if (!path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
        path += QStringLiteral(".json");

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(QStringLiteral("Tidak dapat membuat support report: %1").arg(path));
        return false;
    }
    const QByteArray report = supportReportJson();
    if (file.write(report) != report.size() || !file.commit()) {
        setError(QStringLiteral("Gagal menyimpan support report secara atomik: %1").arg(path));
        return false;
    }
    emit logLine(QStringLiteral("SYS"), QStringLiteral("support report saved"), QString{});
    return true;
}

void K500DeviceManager::appendDiagnosticLine(const QString &direction,
                                             const QString &label,
                                             const QString &hex)
{
    const QString normalizedDirection = direction.trimmed().toUpper();
    const QString normalizedLabel = label.trimmed();
    QString safePayload = hex.trimmed();

    // P5_SUPPORT_REPORT_REDACTION_V1
    // RSP 0xBF is the read-block response carrying active-memory bytes. Every
    // Store-labelled TX frame may contain all or part of a permanent 0x0290
    // preset image. Keep raw frames in m_lastRx/m_lastTx for the local UI and
    // --trace-k500 only; never persist those payloads in an exportable report.
    if (normalizedDirection == QStringLiteral("RX")
        && normalizedLabel.startsWith(QStringLiteral("RSP 0xBF"), Qt::CaseInsensitive)) {
        safePayload = QStringLiteral("[REDACTED ACTIVE MEMORY]");
    } else if (normalizedDirection == QStringLiteral("TX")
               && normalizedLabel.startsWith(QStringLiteral("Store "), Qt::CaseInsensitive)) {
        safePayload = QStringLiteral("[REDACTED PRESET PAYLOAD]");
    }

    if (normalizedDirection == QStringLiteral("RX"))
        m_lastRxDiagnostic = safePayload;
    else if (normalizedDirection == QStringLiteral("TX"))
        m_lastTxDiagnostic = safePayload;

    QString line = QStringLiteral("[%1] %2 %3")
                       .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
                            normalizedDirection, normalizedLabel);
    if (!safePayload.isEmpty())
        line += QStringLiteral(" | ") + safePayload;
    m_diagnosticLog.append(line);
    while (m_diagnosticLog.size() > DiagnosticLogLimit)
        m_diagnosticLog.removeFirst();
}

void K500DeviceManager::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
    emit logLine(QStringLiteral("SYS"), QStringLiteral("status"), status);
}

void K500DeviceManager::setPortLabel(const QString &label)
{
    if (m_portLabel == label)
        return;
    m_portLabel = label;
    emit portLabelChanged();
}

void K500DeviceManager::setError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
    if (!message.isEmpty())
        emit logLine(QStringLiteral("ERR"), QStringLiteral("device error"), message);
}

void K500DeviceManager::setLiveEnabled(bool enabled)
{
    if (m_liveEnabled == enabled)
        return;
    if (!enabled) {
        cancelScheduledTransactions(
            QStringLiteral("Native LIVE state ended before queued command could dispatch"));
    }
    m_liveEnabled = enabled;
    if (m_controller)
        m_controller->setLiveEnabled(enabled);
    emit liveEnabledChanged();
}

void K500DeviceManager::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    emit playingChanged();
    emit logLine(QStringLiteral("SYS"), QStringLiteral("player state"),
                 m_playing ? QStringLiteral("PLAYING") : QStringLiteral("STOPPED/PAUSED"));
}

void K500DeviceManager::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    emit mutedChanged();
    emit logLine(QStringLiteral("SYS"), QStringLiteral("mute state"),
                 m_muted ? QStringLiteral("MUTED") : QStringLiteral("UNMUTED"));
}


void K500DeviceManager::scheduleAuthoritativeReconciliation()
{
    // P3_AUTHORITATIVE_RECONCILIATION_V1 — debounce the already-proven full
    // active-memory readback until the canonical live-write burst is quiet.
    if (!m_controller || m_stage != Stage::Ready || !connected() || !m_liveEnabled
        || m_controller->desiredStateCount() <= 0)
        return;
    m_reconciliationTimer.start(AuthoritativeReconcileIdleMs);
}

void K500DeviceManager::startAuthoritativeReconciliation()
{
    if (!m_controller || m_stage != Stage::Ready || !connected() || !m_liveEnabled
        || m_controller->desiredStateCount() <= 0)
        return;

    // Never read through producer or scheduler work. Keep intent queued and
    // retry the barrier rather than guessing which state the hardware owns.
    if (!m_transactionScheduler.empty() || m_controller->inFlightStateCount() > 0) {
        m_reconciliationTimer.start(AuthoritativeReconcileRetryMs);
        return;
    }

    m_reconciliationInProgress = true;
    setLiveEnabled(false);
    setStatus(QStringLiteral("syncing"));
    emit logLine(QStringLiteral("SYS"), QStringLiteral("authoritative reconciliation"),
                 QStringLiteral("LIVE paused; verifying 939-byte hardware state"));
    requestActiveMemoryReadback();
}

void K500DeviceManager::beginBluetoothScan()
{
    m_serialCandidates = K500WinIo::serialPorts();
    if (m_serialCandidates.isEmpty()) {
        setError(QStringLiteral("Tidak ada COM port Windows yang tersedia untuk Bluetooth SPP."));
        setPortLabel(QStringLiteral("No BT COM port"));
        setStatus(QStringLiteral("error"));
        return;
    }

    if (!m_lastKnownSerialPort.isEmpty()) {
        const int index = m_serialCandidates.indexOf(m_lastKnownSerialPort);
        if (index > 0)
            m_serialCandidates.move(index, 0);
    }

    m_serialCandidateIndex = -1;
    openNextBluetoothCandidate();
}

void K500DeviceManager::openNextBluetoothCandidate()
{
    m_responseTimer.stop();
    m_probeDelayTimer.stop();
    m_io.close();
    m_parser.reset();

    ++m_serialCandidateIndex;
    if (m_serialCandidateIndex >= m_serialCandidates.size()) {
        setError(QStringLiteral("K500 Bluetooth tidak ditemukan. Pastikan KTV_BT sudah paired di Windows dan port SPP tidak sedang dipakai aplikasi lain."));
        setPortLabel(QStringLiteral("K500 BT not found"));
        setStatus(QStringLiteral("error"));
        m_stage = Stage::Idle;
        return;
    }

    m_currentSerialPort = m_serialCandidates.at(m_serialCandidateIndex);
    setPortLabel(QStringLiteral("Scanning %1 · %2/%3")
                     .arg(m_currentSerialPort)
                     .arg(m_serialCandidateIndex + 1)
                     .arg(m_serialCandidates.size()));

    QString error;
    if (!m_io.openSerial(m_currentSerialPort, &error)) {
        QTimer::singleShot(0, this, &K500DeviceManager::openNextBluetoothCandidate);
        return;
    }

    m_stage = Stage::ProbeBluetooth;
    m_probeAttempt = 0;
    m_probeDelayTimer.start(m_currentSerialPort == m_lastKnownSerialPort ? 500 : 250);
}

void K500DeviceManager::sendProbeHeartbeat()
{
    if (m_stage != Stage::ProbeBluetooth && m_stage != Stage::ProbeUsb)
        return;

    ++m_probeAttempt;
    if (!writeFrame(K500Protocol::heartbeat(),
                    QStringLiteral("Probe heartbeat %1").arg(m_probeAttempt))) {
        if (m_stage == Stage::ProbeBluetooth)
            openNextBluetoothCandidate();
        else {
            setStatus(QStringLiteral("error"));
            m_stage = Stage::Idle;
        }
        return;
    }
    m_responseTimer.start(ProbeTimeoutMs);
}

void K500DeviceManager::beginUsbProbe()
{
    QString label;
    QString error;
    if (!m_io.openUsbHid(K500UsbVendorId, K500UsbProductId, &label, &error)) {
        setError(error);
        setPortLabel(QStringLiteral("USB HID DSP AUDIO not found"));
        setStatus(QStringLiteral("error"));
        return;
    }

    setPortLabel(QStringLiteral("%1 · 10C4:0321").arg(label));
    m_stage = Stage::ProbeUsb;
    m_probeAttempt = 0;
    m_probeDelayTimer.start(60);
}

void K500DeviceManager::beginSync()
{
    m_responseTimer.stop();
    m_stage = Stage::AwaitHandshake;
    setStatus(QStringLiteral("syncing"));
    setPortLabel(QStringLiteral("%1 · handshake").arg(m_io.label()));
    if (!writeFrame(K500Protocol::handshake(), QStringLiteral("Handshake 0x3F")))
        return;
    m_responseTimer.start(HandshakeTimeoutMs);
}

void K500DeviceManager::requestActiveMemoryReadback()
{
    m_responseTimer.stop();
    m_stage = Stage::AwaitMemoryBlock;
    m_activeMemory = QByteArray(ActiveMemorySize, char(0));
    m_memoryReadOffset = 0;
    m_pendingReadLength = 0;
    setPortLabel(QStringLiteral("%1 · reading KTV 0/%2").arg(m_io.label()).arg(ActiveMemorySize));
    requestNextMemoryBlock();
}

void K500DeviceManager::requestNextMemoryBlock()
{
    if (m_stage != Stage::AwaitMemoryBlock)
        return;
    if (m_memoryReadOffset >= ActiveMemorySize) {
        finishConnected();
        return;
    }

    m_pendingReadLength = qMin(ActiveMemoryBlockSize, ActiveMemorySize - m_memoryReadOffset);
    const int offset = m_memoryReadOffset;
    // RETRIEVE_ALL_USB_MODE02_CAPTURED_V1 — native KTV startup capture.
    const quint8 mode = m_io.kind() == K500WinIo::Kind::UsbHid ? 0x02 : 0x63;
    setPortLabel(QStringLiteral("%1 · reading KTV %2/%3")
                     .arg(m_io.label()).arg(offset).arg(ActiveMemorySize));
    if (!writeFrame(K500Protocol::readBlock(static_cast<quint16>(offset),
                                             static_cast<quint16>(m_pendingReadLength), mode),
                    QStringLiteral("Read 0x%1 len %2")
                        .arg(offset, 4, 16, QLatin1Char('0')).arg(m_pendingReadLength))) {
        return;
    }
    m_responseTimer.start(ReadbackTimeoutMs);
}

void K500DeviceManager::acceptMemoryBlock(const QByteArray &data)
{
    m_responseTimer.stop();
    if (m_stage != Stage::AwaitMemoryBlock)
        return;
    if (m_pendingReadLength <= 0 || data.size() < m_pendingReadLength) {
        setError(QStringLiteral("K500 readback block 0x%1 terlalu pendek: %2/%3 byte.")
                     .arg(m_memoryReadOffset, 4, 16, QLatin1Char('0'))
                     .arg(data.size()).arg(m_pendingReadLength));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return;
    }

    m_activeMemory.replace(m_memoryReadOffset, m_pendingReadLength,
                           data.left(m_pendingReadLength));
    m_memoryReadOffset += m_pendingReadLength;
    m_pendingReadLength = 0;

    if (m_memoryReadOffset >= ActiveMemorySize) {
        finishConnected();
        return;
    }

    QTimer::singleShot(ActiveMemoryInterBlockMs, this,
                       &K500DeviceManager::requestNextMemoryBlock);
}

void K500DeviceManager::finishConnected()
{
    if (m_activeMemory.size() < ActiveMemorySize || m_memoryReadOffset < ActiveMemorySize) {
        setError(QStringLiteral("K500 active-memory readback tidak lengkap (%1/%2 byte).")
                     .arg(m_memoryReadOffset).arg(ActiveMemorySize));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return;
    }

    m_responseTimer.stop();
    m_stage = Stage::Ready;

    // Important ordering: hydrate controller + StudioEngine while LIVE is OFF.
    // Initial/Recall snapshots reset intent. P3 verification preserves unresolved
    // intent and lets semantic confirmation prove hardware convergence.
    const bool wasReconciliation = m_reconciliationInProgress;
    if (wasReconciliation) {
        emit reconciliationMemoryReady(m_activeMemory);
    } else {
        emit deviceScalarsReady(m_activeMemory.left(0x40));
        emit activeMemoryReady(m_activeMemory);
    }
    setLiveEnabled(true);
    setError({});
    setStatus(QStringLiteral("connected"));

    if (wasReconciliation && m_controller) {
        emit logLine(QStringLiteral("SYS"), QStringLiteral("authoritative reconciliation complete"),
                     QStringLiteral("generation %1 · unresolved %2 · divergent %3")
                         .arg(m_controller->authoritativeSnapshotGeneration())
                         .arg(m_controller->desiredStateCount())
                         .arg(m_controller->divergentDesiredStateCount()));
    } else {
        emit logLine(QStringLiteral("SYS"), QStringLiteral("device sync complete"),
                     QStringLiteral("%1 bytes loaded into native editor").arg(m_activeMemory.size()));
    }
    m_reconciliationInProgress = false;

    if (m_io.kind() == K500WinIo::Kind::Serial) {
        m_lastKnownSerialPort = m_currentSerialPort;
        QSettings().setValue(QStringLiteral("k500/lastBtPort"), m_lastKnownSerialPort);
        setPortLabel(QStringLiteral("%1 · Bluetooth SPP · LIVE").arg(m_currentSerialPort));
    } else {
        setPortLabel(QStringLiteral("%1 · USB HID · LIVE").arg(m_io.label()));
    }

    m_lastValidRx.start();
    m_heartbeatTimer.start();
}

void K500DeviceManager::connectionTimeout()
{
    if (m_stage == Stage::ProbeBluetooth) {
        const int maxAttempts = m_currentSerialPort == m_lastKnownSerialPort ? 4 : 3;
        if (m_probeAttempt < maxAttempts) {
            m_probeDelayTimer.start(220);
            return;
        }
        openNextBluetoothCandidate();
        return;
    }

    if (m_stage == Stage::ProbeUsb) {
        if (m_probeAttempt < 2) {
            m_probeDelayTimer.start(120);
            return;
        }
        setError(QStringLiteral("USB HID DSP AUDIO terbuka tetapi tidak menjawab heartbeat K500 0x1C."));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return;
    }

    if (m_stage == Stage::AwaitHandshake) {
        requestActiveMemoryReadback();
        return;
    }

    if (m_stage == Stage::AwaitMemoryBlock) {
        setError(QStringLiteral("Timeout membaca active memory K500 di 0x%1. LIVE tetap OFF agar state device tidak tertimpa.")
                     .arg(m_memoryReadOffset, 4, 16, QLatin1Char('0')));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
    }
}

void K500DeviceManager::heartbeatTick()
{
    if (m_stage != Stage::Ready || !connected())
        return;

    if (m_lastValidRx.isValid() && m_lastValidRx.elapsed() > ConnectionWatchdogMs) {
        setError(QStringLiteral("K500 heartbeat timeout; koneksi dilepas agar live write berhenti."));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return;
    }

    writeFrame(K500Protocol::heartbeat(), QStringLiteral("Heartbeat 0x1C"));
}

void K500DeviceManager::onBytesReceived(const QByteArray &bytes)
{
    const auto responses = m_parser.feed(bytes);
    for (const K500Response &response : responses) {
        m_lastRx = K500Frame::hex(response.raw);
        emit lastRxChanged();
        emit logLine(QStringLiteral("RX"),
                     QStringLiteral("RSP 0x%1%2")
                         .arg(response.rsp, 2, 16, QLatin1Char('0'))
                         .arg(response.checksumOk ? QString{} : QStringLiteral(" BAD-CS"))
                         .toUpper(),
                     m_lastRx);
        if (response.checksumOk) {
            m_lastValidRx.start();
            handleResponse(response);
        }
    }
}

void K500DeviceManager::handleResponse(const K500Response &response)
{
    bool decodedPlaying = false;
    if (K500ResponseParser::tryDecodePlaying(response, &decodedPlaying))
        setPlaying(decodedPlaying);

    bool decodedMuted = false;
    if (K500ResponseParser::tryDecodeMuted(response, &decodedMuted))
        setMuted(decodedMuted);

    if ((m_stage == Stage::ProbeBluetooth || m_stage == Stage::ProbeUsb)
        && response.rsp == 0xE3) {
        beginSync();
        return;
    }

    if (m_stage == Stage::AwaitHandshake && response.rsp == 0xC0) {
        requestActiveMemoryReadback();
        return;
    }

    if (m_stage == Stage::AwaitMemoryBlock && response.rsp == 0xBF) {
        acceptMemoryBlock(response.data);
        return;
    }

    if (m_stage == Stage::Ready && response.rsp == 0xE3)
        return;
}

void K500DeviceManager::onIoError(const QString &message)
{
    if (m_stage == Stage::Idle)
        return;

    if (m_stage == Stage::ProbeBluetooth) {
        setError({});
        openNextBluetoothCandidate();
        return;
    }

    setError(message);
    resetConnectionState(true);
    setStatus(QStringLiteral("error"));
}

bool K500DeviceManager::writeFrame(const QByteArray &frame, const QString &label)
{
    if (frame.isEmpty() || !m_io.isOpen())
        return false;

    QString error;
    if (!m_io.writeProtocolFrame(frame, &error)) {
        setError(error);
        if (m_stage == Stage::ProbeBluetooth) {
            m_io.close();
            return false;
        }
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return false;
    }

    const QByteArray wireFrame = m_io.kind() == K500WinIo::Kind::UsbHid
        ? K500Frame::toUsbFrame(frame) : frame;
    m_lastTx = K500Frame::hex(wireFrame);
    emit lastTxChanged();
    emit logLine(QStringLiteral("TX"), label, m_lastTx);
    return true;
}

void K500DeviceManager::resetConnectionState(bool keepError)
{
    m_responseTimer.stop();
    m_probeDelayTimer.stop();
    m_heartbeatTimer.stop();
    m_reconciliationTimer.stop();
    m_reconciliationInProgress = false;
    m_stage = Stage::Idle;
    m_parser.reset();
    m_io.close();
    setLiveEnabled(false);
    m_schedulerTimer.stop();
    m_transactionScheduler.endSession();
    if (m_controller)
        m_controller->clearDeviceState();
    m_activeMemory.clear();
    m_memoryReadOffset = 0;
    m_pendingReadLength = 0;
    setPlaying(false);
    setMuted(false);
    if (!keepError)
        setError({});
}
