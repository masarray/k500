from pathlib import Path


def read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    Path(path).write_text(text, encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


# Controller: transport phase changes to Dispatched only when the scheduler
# actually releases the command to the asynchronous K500WinIo worker.
path = "src/k500/K500Controller.h"
text = read(path)
text = replace_once(
    text,
    "    void handleStateEdit(const QString &path, const QVariant &value);\n"
    "    void handleCommandDispatchResult(quint64 sessionEpoch, quint64 token,\n",
    "    void handleStateEdit(const QString &path, const QVariant &value);\n"
    "    bool markCommandDispatched(quint64 sessionEpoch, quint64 token);\n"
    "    void handleCommandDispatchResult(quint64 sessionEpoch, quint64 token,\n",
    "controller header markCommandDispatched",
)
write(path, text)

path = "src/k500/K500Controller.cpp"
text = read(path)
text = replace_once(
    text,
    "void K500Controller::handleCommandDispatchResult(quint64 sessionEpoch, quint64 token,\n"
    "                                                  const QString &path, bool accepted,\n"
    "                                                  const QString &reason)\n",
    "bool K500Controller::markCommandDispatched(quint64 sessionEpoch, quint64 token)\n"
    "{\n"
    "    // P2_ACTUAL_DISPATCH_BARRIER_V1 — Queued remains Queued while the\n"
    "    // transaction waits inside the deterministic scheduler. Only the exact\n"
    "    // release to the asynchronous transport worker advances it to Dispatched.\n"
    "    if (sessionEpoch != m_canonicalState.sessionEpoch())\n"
    "        return false;\n"
    "    if (!m_canonicalState.markDispatched(token))\n"
    "        return false;\n"
    "    emit canonicalStateChanged();\n"
    "    return true;\n"
    "}\n\n"
    "void K500Controller::handleCommandDispatchResult(quint64 sessionEpoch, quint64 token,\n"
    "                                                  const QString &path, bool accepted,\n"
    "                                                  const QString &reason)\n",
    "controller dispatch barrier implementation",
)
old_dispatch = (
    "        if (!m_canonicalState.isCurrent(command) || !m_canonicalState.markDispatched(command.token))\n"
    "            continue;\n"
    "        emit canonicalStateChanged();\n"
    "        emit commandReady(command.sessionEpoch, command.token, command.frame, command.label,\n"
)
new_dispatch = (
    "        if (!m_canonicalState.isCurrent(command))\n"
    "            continue;\n"
    "        emit commandReady(command.sessionEpoch, command.token, command.frame, command.label,\n"
)
if text.count(old_dispatch) != 2:
    raise SystemExit(f"controller flush dispatch removal: expected 2 matches, found {text.count(old_dispatch)}")
text = text.replace(old_dispatch, new_dispatch)
write(path, text)

# Scheduler: surface evicted/expired/cancelled work so CanonicalState cannot keep
# a stranded in-flight token when bounded back-pressure drops a transaction.
path = "src/k500/K500TransactionScheduler.h"
text = read(path)
text = replace_once(
    text,
    "    void clearQueued();\n\n    int queuedCount() const",
    "    void clearQueued();\n    QList<Transaction> takeDropped();\n\n    int queuedCount() const",
    "scheduler header takeDropped",
)
text = replace_once(
    text,
    "    QList<Transaction> m_queue;\n    qint64 m_lastImmediateDispatchMs",
    "    QList<Transaction> m_queue;\n    QList<Transaction> m_dropped;\n    qint64 m_lastImmediateDispatchMs",
    "scheduler header dropped queue",
)
write(path, text)

path = "src/k500/K500TransactionScheduler.cpp"
text = read(path)
text = replace_once(
    text,
    "    m_queue.clear();\n    m_lastImmediateDispatchMs = -1;\n",
    "    m_queue.clear();\n    m_dropped.clear();\n    m_lastImmediateDispatchMs = -1;\n",
    "scheduler beginSession dropped reset",
)
# There is a second m_queue.clear() in endSession.
anchor = "void K500TransactionScheduler::endSession()\n{\n"
start = text.index(anchor)
end = text.index("}\n\nK500TransactionScheduler::EnqueueResult", start)
block = text[start:end]
block = replace_once(
    block,
    "    m_queue.clear();\n",
    "    m_queue.clear();\n    m_dropped.clear();\n",
    "scheduler endSession dropped reset",
)
text = text[:start] + block + text[end:]
text = replace_once(
    text,
    "        m_queue.removeAt(evictionIndex);\n        ++m_telemetry.evicted;\n",
    "        m_dropped.append(m_queue.at(evictionIndex));\n        m_queue.removeAt(evictionIndex);\n        ++m_telemetry.evicted;\n",
    "scheduler eviction recovery",
)
text = replace_once(
    text,
    "        m_queue.removeAt(i);\n        ++removed;\n",
    "        m_dropped.append(transaction);\n        m_queue.removeAt(i);\n        ++removed;\n",
    "scheduler expiry recovery",
)
text = replace_once(
    text,
    "void K500TransactionScheduler::clearQueued()\n{\n    m_queue.clear();\n    syncQueueTelemetry();\n}\n\nK500TransactionScheduler::Telemetry",
    "void K500TransactionScheduler::clearQueued()\n{\n    for (const Transaction &transaction : m_queue)\n        m_dropped.append(transaction);\n    m_queue.clear();\n    syncQueueTelemetry();\n}\n\nQList<K500TransactionScheduler::Transaction> K500TransactionScheduler::takeDropped()\n{\n    QList<Transaction> dropped;\n    dropped.swap(m_dropped);\n    return dropped;\n}\n\nK500TransactionScheduler::Telemetry",
    "scheduler takeDropped implementation",
)
write(path, text)

# DeviceManager: commandReady no longer writes directly. It enters the scheduler,
# which owns host pacing and releases work into the already-asynchronous K500WinIo worker.
path = "src/k500/K500DeviceManager.h"
text = read(path)
text = replace_once(
    text,
    '#include "K500ResponseParser.h"\n#include "K500WinIo.h"\n',
    '#include "K500ResponseParser.h"\n#include "K500TransactionScheduler.h"\n#include "K500WinIo.h"\n',
    "device manager scheduler include",
)
text = replace_once(
    text,
    "    void setLiveEnabled(bool enabled);\n    void appendDiagnosticLine",
    "    void setLiveEnabled(bool enabled);\n    qint64 schedulerNowMs() const;\n    void armTransactionScheduler();\n    void dispatchScheduledCommands();\n    void rejectDroppedTransactions(const QString &reason);\n    void cancelScheduledTransactions(const QString &reason);\n    void appendDiagnosticLine",
    "device manager scheduler helpers",
)
text = replace_once(
    text,
    "    K500WinIo m_io;\n    K500ResponseParser m_parser;\n",
    "    K500WinIo m_io;\n    K500ResponseParser m_parser;\n    K500TransactionScheduler m_transactionScheduler;\n",
    "device manager scheduler member",
)
text = replace_once(
    text,
    "    QTimer m_responseTimer;\n    QTimer m_probeDelayTimer;\n    QTimer m_heartbeatTimer;\n    QElapsedTimer m_lastValidRx;\n",
    "    QTimer m_responseTimer;\n    QTimer m_probeDelayTimer;\n    QTimer m_heartbeatTimer;\n    QTimer m_schedulerTimer;\n    QElapsedTimer m_schedulerClock;\n    QElapsedTimer m_lastValidRx;\n",
    "device manager scheduler timer",
)
write(path, text)

path = "src/k500/K500DeviceManager.cpp"
text = read(path)
text = replace_once(
    text,
    "constexpr int ActiveMemoryInterBlockMs = 35;\n}\n",
    "constexpr int ActiveMemoryInterBlockMs = 35;\n\n"
    "K500TransactionScheduler::Family schedulerFamilyForPath(const QString &path)\n"
    "{\n"
    "    // Only PEQ band writes used the old 45 ms EQ timer. Crossovers and\n"
    "    // global bypass are complete-block traffic and retain 55 ms pacing.\n"
    "    if (path.startsWith(QStringLiteral(\"eq.\"))\n"
    "        && path.contains(QStringLiteral(\".bands.\")))\n"
    "        return K500TransactionScheduler::Family::Eq;\n"
    "    return K500TransactionScheduler::Family::Block;\n"
    "}\n"
    "}\n",
    "device manager family classifier",
)
text = replace_once(
    text,
    "    m_responseTimer.setSingleShot(true);\n    m_probeDelayTimer.setSingleShot(true);\n    m_heartbeatTimer.setInterval(HeartbeatIntervalMs);\n",
    "    m_responseTimer.setSingleShot(true);\n    m_probeDelayTimer.setSingleShot(true);\n    m_heartbeatTimer.setInterval(HeartbeatIntervalMs);\n"
    "    m_schedulerTimer.setSingleShot(true);\n"
    "    m_schedulerClock.start();\n",
    "device manager scheduler timer setup",
)
text = replace_once(
    text,
    "    connect(&m_heartbeatTimer, &QTimer::timeout,\n            this, &K500DeviceManager::heartbeatTick);\n",
    "    connect(&m_heartbeatTimer, &QTimer::timeout,\n            this, &K500DeviceManager::heartbeatTick);\n"
    "    connect(&m_schedulerTimer, &QTimer::timeout,\n"
    "            this, &K500DeviceManager::dispatchScheduledCommands);\n",
    "device manager scheduler timer connection",
)
text = replace_once(
    text,
    "    if (m_controller)\n        m_controller->beginDeviceSession();\n",
    "    if (m_controller) {\n"
    "        m_controller->beginDeviceSession();\n"
    "        m_transactionScheduler.beginSession(m_controller->sessionEpoch());\n"
    "    }\n",
    "device manager scheduler session start",
)
old_send = '''void K500DeviceManager::sendPlannedCommand(quint64 sessionEpoch, quint64 token,
                                            const QByteArray &frame, const QString &label,
                                            const QString &path, const QString &coalescingKey)
{
    Q_UNUSED(coalescingKey);
    if (!m_controller || sessionEpoch != m_controller->sessionEpoch()) {
        emit commandDispatchResult(sessionEpoch, token, path, false,
                                   QStringLiteral("Stale command rejected after device-session change"));
        return;
    }
    if (!connected() || !m_liveEnabled || frame.isEmpty()) {
        emit commandDispatchResult(sessionEpoch, token, path, false,
                                   QStringLiteral("Native transport is not LIVE for this session"));
        return;
    }
    const bool accepted = writeFrame(frame, label);
    emit commandDispatchResult(sessionEpoch, token, path, accepted,
                               accepted ? QString{} : QStringLiteral("Native transport write failed"));
}
'''
new_send = '''void K500DeviceManager::sendPlannedCommand(quint64 sessionEpoch, quint64 token,
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
'''
text = replace_once(text, old_send, new_send, "device manager scheduled send path")
text = replace_once(
    text,
    "    root.insert(QStringLiteral(\"device\"), device);\n\n    QJsonObject presetOperation;\n",
    "    root.insert(QStringLiteral(\"device\"), device);\n\n"
    "    const auto schedulerTelemetry = m_transactionScheduler.telemetry();\n"
    "    QJsonObject scheduler;\n"
    "    scheduler.insert(QStringLiteral(\"sessionEpoch\"), QString::number(m_transactionScheduler.sessionEpoch()));\n"
    "    scheduler.insert(QStringLiteral(\"queued\"), schedulerTelemetry.queued);\n"
    "    scheduler.insert(QStringLiteral(\"peakQueued\"), schedulerTelemetry.peakQueued);\n"
    "    scheduler.insert(QStringLiteral(\"enqueued\"), QString::number(schedulerTelemetry.enqueued));\n"
    "    scheduler.insert(QStringLiteral(\"coalesced\"), QString::number(schedulerTelemetry.coalesced));\n"
    "    scheduler.insert(QStringLiteral(\"evicted\"), QString::number(schedulerTelemetry.evicted));\n"
    "    scheduler.insert(QStringLiteral(\"expired\"), QString::number(schedulerTelemetry.expired));\n"
    "    scheduler.insert(QStringLiteral(\"rejectedBackpressure\"), QString::number(schedulerTelemetry.rejectedBackpressure));\n"
    "    scheduler.insert(QStringLiteral(\"rejectedStale\"), QString::number(schedulerTelemetry.rejectedStale));\n"
    "    scheduler.insert(QStringLiteral(\"dispatched\"), QString::number(schedulerTelemetry.dispatched));\n"
    "    root.insert(QStringLiteral(\"transactionScheduler\"), scheduler);\n\n"
    "    QJsonObject presetOperation;\n",
    "device manager support telemetry",
)
old_live = '''void K500DeviceManager::setLiveEnabled(bool enabled)
{
    if (m_liveEnabled == enabled)
        return;
    m_liveEnabled = enabled;
    if (m_controller)
        m_controller->setLiveEnabled(enabled);
    emit liveEnabledChanged();
}
'''
new_live = '''void K500DeviceManager::setLiveEnabled(bool enabled)
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
'''
text = replace_once(text, old_live, new_live, "device manager live cancellation")
text = replace_once(
    text,
    "    m_io.close();\n    setLiveEnabled(false);\n    if (m_controller)\n",
    "    m_io.close();\n    setLiveEnabled(false);\n"
    "    m_schedulerTimer.stop();\n"
    "    m_transactionScheduler.endSession();\n"
    "    if (m_controller)\n",
    "device manager scheduler session reset",
)
write(path, text)

# Self-test dropped-transaction recovery contract.
path = "src/k500/P2TransactionSchedulerSelfTestMain.cpp"
text = read(path)
text = replace_once(
    text,
    "        ok &= require(scheduler.queuedCount() == 2 && scheduler.telemetry().evicted == 1,\n"
    "                      \"bounded queue/eviction telemetry mismatch\");\n"
    "        const auto first = scheduler.takeReady(1001);\n",
    "        ok &= require(scheduler.queuedCount() == 2 && scheduler.telemetry().evicted == 1,\n"
    "                      \"bounded queue/eviction telemetry mismatch\");\n"
    "        const auto evicted = scheduler.takeDropped();\n"
    "        ok &= require(evicted.size() == 1 && evicted.first().token == 1,\n"
    "                      \"evicted work must be surfaced for canonical rejection\");\n"
    "        const auto first = scheduler.takeReady(1001);\n",
    "selftest eviction recovery",
)
text = replace_once(
    text,
    "        scheduler.clearQueued();\n        scheduler.enqueue(tx(15, 4, \"critical-a\"",
    "        scheduler.clearQueued();\n"
    "        const auto cancelled = scheduler.takeDropped();\n"
    "        ok &= require(!cancelled.isEmpty(),\n"
    "                      \"clearQueued must surface cancelled transactions\");\n"
    "        scheduler.enqueue(tx(15, 4, \"critical-a\"",
    "selftest cancellation recovery",
)
text = replace_once(
    text,
    "        ok &= require(scheduler.telemetry().expired == 1,\n"
    "                      \"expiry telemetry must increment\");\n",
    "        ok &= require(scheduler.telemetry().expired == 1,\n"
    "                      \"expiry telemetry must increment\");\n"
    "        const auto expired = scheduler.takeDropped();\n"
    "        ok &= require(expired.size() == 1 && expired.first().token == 1,\n"
    "                      \"expired work must be surfaced for canonical rejection\");\n",
    "selftest expiry recovery",
)
write(path, text)

# CI guard now qualifies the actual Controller -> scheduler -> async worker bridge,
# not only the standalone queue algorithm.
path = ".github/workflows/p2-deterministic-transaction-scheduler.yml"
text = read(path)
for section in ("push", "pull_request"):
    pass
text = text.replace(
    "      - 'src/k500/K500TransactionScheduler.*'\n",
    "      - 'src/k500/K500TransactionScheduler.*'\n"
    "      - 'src/k500/K500Controller.*'\n"
    "      - 'src/k500/K500DeviceManager.*'\n",
)
if text.count("      - 'src/k500/K500Controller.*'") != 2:
    raise SystemExit("workflow path expansion failed")
text = replace_once(
    text,
    "          test = Path('src/k500/P2TransactionSchedulerSelfTestMain.cpp').read_text(encoding='utf-8')\n"
    "          docs = Path('docs/P2_DETERMINISTIC_TRANSACTION_SCHEDULER.md').read_text(encoding='utf-8')\n",
    "          test = Path('src/k500/P2TransactionSchedulerSelfTestMain.cpp').read_text(encoding='utf-8')\n"
    "          controller = Path('src/k500/K500Controller.cpp').read_text(encoding='utf-8')\n"
    "          manager = Path('src/k500/K500DeviceManager.cpp').read_text(encoding='utf-8')\n"
    "          docs = Path('docs/P2_DETERMINISTIC_TRANSACTION_SCHEDULER.md').read_text(encoding='utf-8')\n",
    "workflow reads transport bridge",
)
text = replace_once(
    text,
    "              'telemetry': 'peakQueued' in h and 'coalesced' in h and 'dispatched' in h,\n"
    "              'hardware-free qualification': 'P2 deterministic transaction scheduler self-test passed' in test,\n",
    "              'telemetry': 'peakQueued' in h and 'coalesced' in h and 'dispatched' in h,\n"
    "              'dropped work recovery': 'takeDropped' in h and 'm_dropped' in cpp and 'takeDropped' in test,\n"
    "              'transport bridge': 'P2_SCHEDULER_TRANSPORT_BRIDGE_V1' in manager and 'm_transactionScheduler.enqueue' in manager and 'dispatchScheduledCommands' in manager,\n"
    "              'actual dispatch barrier': 'P2_ACTUAL_DISPATCH_BARRIER_V1' in controller and 'markCommandDispatched' in manager,\n"
    "              'controller does not pre-dispatch': 'm_canonicalState.markDispatched(command.token)' not in controller,\n"
    "              'hardware-free qualification': 'P2 deterministic transaction scheduler self-test passed' in test,\n",
    "workflow integration checks",
)
write(path, text)

# Temporary patching machinery must not survive in the branch tree.
Path("tools/p2_next_phase_patch.py").unlink()
Path(".github/workflows/p2-next-phase-apply.yml").unlink()
print("P2 next-phase transport bridge patch applied")
