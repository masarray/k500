#include "K500TransactionScheduler.h"

#include <algorithm>
#include <limits>

namespace {
int priorityValue(K500TransactionScheduler::Priority priority)
{
    return static_cast<int>(priority);
}
}

K500TransactionScheduler::K500TransactionScheduler(Config config)
    : m_config(config)
{
    m_config.maxQueued = std::max(1, m_config.maxQueued);
    m_config.maxQueueAgeMs = std::max<qint64>(1, m_config.maxQueueAgeMs);
    m_config.immediateMinSpacingMs = std::max(0, m_config.immediateMinSpacingMs);
    m_config.eqMinSpacingMs = std::max(0, m_config.eqMinSpacingMs);
    m_config.blockMinSpacingMs = std::max(0, m_config.blockMinSpacingMs);
}

void K500TransactionScheduler::beginSession(quint64 sessionEpoch)
{
    m_sessionEpoch = sessionEpoch;
    m_nextSequence = 1;
    m_queue.clear();
    m_dropped.clear();
    m_lastImmediateDispatchMs = -1;
    m_lastEqDispatchMs = -1;
    m_lastBlockDispatchMs = -1;
    m_telemetry = {};
    syncQueueTelemetry();
}

void K500TransactionScheduler::endSession()
{
    m_sessionEpoch = 0;
    m_nextSequence = 1;
    m_queue.clear();
    m_dropped.clear();
    m_lastImmediateDispatchMs = -1;
    m_lastEqDispatchMs = -1;
    m_lastBlockDispatchMs = -1;
    syncQueueTelemetry();
}

K500TransactionScheduler::EnqueueResult K500TransactionScheduler::enqueue(
    Transaction transaction, qint64 nowMs)
{
    if (!transaction.valid()) {
        ++m_telemetry.rejectedInvalid;
        syncQueueTelemetry();
        return EnqueueResult::RejectedInvalid;
    }
    if (!sessionActive() || transaction.sessionEpoch != m_sessionEpoch) {
        ++m_telemetry.rejectedStale;
        syncQueueTelemetry();
        return EnqueueResult::RejectedStaleSession;
    }

    expire(nowMs);
    transaction.enqueuedAtMs = nowMs;
    transaction.sequence = m_nextSequence++;
    if (m_nextSequence == 0)
        m_nextSequence = 1;

    const int replaceIndex = findCoalescingKey(transaction.coalescingKey);
    if (replaceIndex >= 0) {
        // P2_COALESCED_TOKEN_RECOVERY_V1 — latest-wins must also surface the
        // superseded envelope so CanonicalState can reject its in-flight token.
        m_dropped.append(m_queue.at(replaceIndex));
        m_queue[replaceIndex] = std::move(transaction);
        ++m_telemetry.enqueued;
        ++m_telemetry.coalesced;
        syncQueueTelemetry();
        return EnqueueResult::Replaced;
    }

    EnqueueResult result = EnqueueResult::Accepted;
    if (m_queue.size() >= m_config.maxQueued) {
        const int evictionIndex = selectEvictionIndex(transaction.priority);
        if (evictionIndex < 0) {
            ++m_telemetry.rejectedBackpressure;
            syncQueueTelemetry();
            return EnqueueResult::RejectedBackpressure;
        }
        m_dropped.append(m_queue.at(evictionIndex));
        m_queue.removeAt(evictionIndex);
        ++m_telemetry.evicted;
        result = EnqueueResult::AcceptedAfterEviction;
    }

    m_queue.append(std::move(transaction));
    ++m_telemetry.enqueued;
    syncQueueTelemetry();
    return result;
}

std::optional<K500TransactionScheduler::Transaction>
K500TransactionScheduler::takeReady(qint64 nowMs)
{
    expire(nowMs);
    const int index = selectReadyIndex(nowMs);
    if (index < 0)
        return std::nullopt;
    Transaction transaction = m_queue.takeAt(index);
    syncQueueTelemetry();
    return transaction;
}

qint64 K500TransactionScheduler::nextWakeDelayMs(qint64 nowMs) const
{
    if (m_queue.isEmpty())
        return -1;

    qint64 delay = std::numeric_limits<qint64>::max();
    for (const Transaction &transaction : m_queue) {
        if (transaction.sessionEpoch != m_sessionEpoch)
            return 0;
        const qint64 age = nowMs - transaction.enqueuedAtMs;
        if (age > m_config.maxQueueAgeMs)
            return 0;
        const qint64 readyAt = familyReadyAtMs(transaction.family);
        delay = std::min(delay, std::max<qint64>(0, readyAt - nowMs));
    }
    return delay == std::numeric_limits<qint64>::max() ? -1 : delay;
}

void K500TransactionScheduler::noteDispatched(const Transaction &transaction, qint64 nowMs)
{
    if (!sessionActive() || transaction.sessionEpoch != m_sessionEpoch)
        return;

    switch (transaction.family) {
    case Family::Immediate:
        m_lastImmediateDispatchMs = nowMs;
        break;
    case Family::Eq:
        m_lastEqDispatchMs = nowMs;
        break;
    case Family::Block:
        m_lastBlockDispatchMs = nowMs;
        break;
    }
    ++m_telemetry.dispatched;
    syncQueueTelemetry();
}

int K500TransactionScheduler::expire(qint64 nowMs)
{
    int removed = 0;
    for (int i = m_queue.size() - 1; i >= 0; --i) {
        const Transaction &transaction = m_queue.at(i);
        const bool stale = transaction.sessionEpoch != m_sessionEpoch;
        const bool agedOut = nowMs - transaction.enqueuedAtMs > m_config.maxQueueAgeMs;
        if (!stale && !agedOut)
            continue;
        m_dropped.append(transaction);
        m_queue.removeAt(i);
        ++removed;
        if (stale)
            ++m_telemetry.rejectedStale;
        else
            ++m_telemetry.expired;
    }
    syncQueueTelemetry();
    return removed;
}

void K500TransactionScheduler::clearQueued()
{
    for (const Transaction &transaction : m_queue)
        m_dropped.append(transaction);
    m_queue.clear();
    syncQueueTelemetry();
}

QList<K500TransactionScheduler::Transaction> K500TransactionScheduler::takeDropped()
{
    QList<Transaction> dropped;
    dropped.swap(m_dropped);
    return dropped;
}

K500TransactionScheduler::Telemetry K500TransactionScheduler::telemetry() const
{
    Telemetry snapshot = m_telemetry;
    snapshot.queued = m_queue.size();
    return snapshot;
}

const char *K500TransactionScheduler::enqueueResultName(EnqueueResult result)
{
    switch (result) {
    case EnqueueResult::Accepted: return "accepted";
    case EnqueueResult::Replaced: return "replaced";
    case EnqueueResult::AcceptedAfterEviction: return "accepted-after-eviction";
    case EnqueueResult::RejectedInvalid: return "rejected-invalid";
    case EnqueueResult::RejectedStaleSession: return "rejected-stale-session";
    case EnqueueResult::RejectedBackpressure: return "rejected-backpressure";
    }
    return "unknown";
}

qint64 K500TransactionScheduler::familyMinSpacingMs(Family family) const
{
    switch (family) {
    case Family::Immediate: return m_config.immediateMinSpacingMs;
    case Family::Eq: return m_config.eqMinSpacingMs;
    case Family::Block: return m_config.blockMinSpacingMs;
    }
    return 0;
}

qint64 K500TransactionScheduler::familyReadyAtMs(Family family) const
{
    qint64 last = -1;
    switch (family) {
    case Family::Immediate: last = m_lastImmediateDispatchMs; break;
    case Family::Eq: last = m_lastEqDispatchMs; break;
    case Family::Block: last = m_lastBlockDispatchMs; break;
    }
    if (last < 0)
        return 0;
    return last + familyMinSpacingMs(family);
}

bool K500TransactionScheduler::isReady(const Transaction &transaction, qint64 nowMs) const
{
    return transaction.sessionEpoch == m_sessionEpoch
        && nowMs >= familyReadyAtMs(transaction.family);
}

int K500TransactionScheduler::findCoalescingKey(const QString &key) const
{
    for (int i = 0; i < m_queue.size(); ++i) {
        if (m_queue.at(i).coalescingKey == key)
            return i;
    }
    return -1;
}

int K500TransactionScheduler::selectReadyIndex(qint64 nowMs) const
{
    int best = -1;
    for (int i = 0; i < m_queue.size(); ++i) {
        const Transaction &candidate = m_queue.at(i);
        if (!isReady(candidate, nowMs))
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        const Transaction &current = m_queue.at(best);
        const int candidatePriority = priorityValue(candidate.priority);
        const int currentPriority = priorityValue(current.priority);
        if (candidatePriority > currentPriority
            || (candidatePriority == currentPriority && candidate.sequence < current.sequence)) {
            best = i;
        }
    }
    return best;
}

int K500TransactionScheduler::selectEvictionIndex(Priority incomingPriority) const
{
    int candidateIndex = -1;
    for (int i = 0; i < m_queue.size(); ++i) {
        const Transaction &candidate = m_queue.at(i);
        if (priorityValue(candidate.priority) > priorityValue(incomingPriority))
            continue;
        if (candidateIndex < 0) {
            candidateIndex = i;
            continue;
        }
        const Transaction &current = m_queue.at(candidateIndex);
        const int candidatePriority = priorityValue(candidate.priority);
        const int currentPriority = priorityValue(current.priority);
        if (candidatePriority < currentPriority
            || (candidatePriority == currentPriority && candidate.sequence < current.sequence)) {
            candidateIndex = i;
        }
    }
    return candidateIndex;
}

void K500TransactionScheduler::syncQueueTelemetry()
{
    m_telemetry.queued = m_queue.size();
    m_telemetry.peakQueued = std::max(m_telemetry.peakQueued, static_cast<int>(m_queue.size()));
}
