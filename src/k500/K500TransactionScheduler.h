#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QtGlobal>

#include <optional>

// P2_DETERMINISTIC_TRANSACTION_SCHEDULER_V1
// Host-side scheduling policy between CanonicalState CommandPlan envelopes and
// the asynchronous HID worker. This layer never changes protocol bytes and it
// never treats transport acceptance as hardware confirmation.
class K500TransactionScheduler final
{
public:
    enum class Family : quint8 {
        Immediate = 0,
        Eq,
        Block,
    };

    enum class Priority : quint8 {
        Background = 0,
        Normal,
        Interactive,
        Critical,
    };

    enum class EnqueueResult : quint8 {
        Accepted = 0,
        Replaced,
        AcceptedAfterEviction,
        RejectedInvalid,
        RejectedStaleSession,
        RejectedBackpressure,
    };

    struct Config {
        int maxQueued = 64;
        qint64 maxQueueAgeMs = 1500;
        int immediateMinSpacingMs = 0;
        int eqMinSpacingMs = 45;
        int blockMinSpacingMs = 55;
    };

    struct Transaction {
        quint64 sessionEpoch = 0;
        quint64 token = 0;
        QByteArray frame;
        QString label;
        QString semanticPath;
        QString coalescingKey;
        Family family = Family::Block;
        Priority priority = Priority::Interactive;
        qint64 enqueuedAtMs = 0;
        quint64 sequence = 0;

        bool valid() const
        {
            return sessionEpoch != 0 && token != 0 && !frame.isEmpty()
                && !semanticPath.trimmed().isEmpty()
                && !coalescingKey.trimmed().isEmpty();
        }
    };

    struct Telemetry {
        quint64 enqueued = 0;
        quint64 coalesced = 0;
        quint64 evicted = 0;
        quint64 rejectedBackpressure = 0;
        quint64 rejectedStale = 0;
        quint64 rejectedInvalid = 0;
        quint64 expired = 0;
        quint64 dispatched = 0;
        int queued = 0;
        int peakQueued = 0;
    };

    explicit K500TransactionScheduler(Config config = {});

    void beginSession(quint64 sessionEpoch);
    void endSession();

    bool sessionActive() const { return m_sessionEpoch != 0; }
    quint64 sessionEpoch() const { return m_sessionEpoch; }
    const Config &config() const { return m_config; }

    EnqueueResult enqueue(Transaction transaction, qint64 nowMs);
    std::optional<Transaction> takeReady(qint64 nowMs);
    qint64 nextWakeDelayMs(qint64 nowMs) const;
    void noteDispatched(const Transaction &transaction, qint64 nowMs);
    int expire(qint64 nowMs);
    void clearQueued();
    QList<Transaction> takeDropped();

    int queuedCount() const { return m_queue.size(); }
    bool empty() const { return m_queue.isEmpty(); }
    Telemetry telemetry() const;

    static const char *enqueueResultName(EnqueueResult result);

private:
    qint64 familyMinSpacingMs(Family family) const;
    qint64 familyReadyAtMs(Family family) const;
    bool isReady(const Transaction &transaction, qint64 nowMs) const;
    int findCoalescingKey(const QString &key) const;
    int selectReadyIndex(qint64 nowMs) const;
    int selectEvictionIndex(Priority incomingPriority) const;
    void syncQueueTelemetry();

    Config m_config;
    quint64 m_sessionEpoch = 0;
    quint64 m_nextSequence = 1;
    QList<Transaction> m_queue;
    QList<Transaction> m_dropped;
    qint64 m_lastImmediateDispatchMs = -1;
    qint64 m_lastEqDispatchMs = -1;
    qint64 m_lastBlockDispatchMs = -1;
    Telemetry m_telemetry;
};
