#include "K500TransactionScheduler.h"

#include <QCoreApplication>
#include <QDebug>

namespace {
using Scheduler = K500TransactionScheduler;

bool require(bool condition, const char *message)
{
    if (condition)
        return true;
    qCritical().noquote() << "P2 scheduler self-test failed:" << message;
    return false;
}

Scheduler::Transaction tx(quint64 epoch,
                          quint64 token,
                          const char *key,
                          Scheduler::Family family = Scheduler::Family::Immediate,
                          Scheduler::Priority priority = Scheduler::Priority::Interactive)
{
    Scheduler::Transaction transaction;
    transaction.sessionEpoch = epoch;
    transaction.token = token;
    transaction.frame = QByteArray::fromHex("AA010001FE");
    transaction.label = QStringLiteral("self-test");
    transaction.semanticPath = QStringLiteral("test.") + QString::fromLatin1(key);
    transaction.coalescingKey = QString::fromLatin1(key);
    transaction.family = family;
    transaction.priority = priority;
    return transaction;
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    {
        Scheduler scheduler;
        scheduler.beginSession(7);
        ok &= require(scheduler.enqueue(tx(6, 1, "stale"), 1000)
                          == Scheduler::EnqueueResult::RejectedStaleSession,
                      "stale session must be rejected");

        Scheduler::Transaction invalid = tx(7, 2, "invalid");
        invalid.frame.clear();
        ok &= require(scheduler.enqueue(invalid, 1000)
                          == Scheduler::EnqueueResult::RejectedInvalid,
                      "invalid envelope must be rejected");
    }

    {
        Scheduler scheduler;
        scheduler.beginSession(10);
        ok &= require(scheduler.enqueue(tx(10, 1, "eq:music:0"), 1000)
                          == Scheduler::EnqueueResult::Accepted,
                      "first keyed transaction must be accepted");
        ok &= require(scheduler.enqueue(tx(10, 2, "eq:music:0"), 1001)
                          == Scheduler::EnqueueResult::Replaced,
                      "same coalescing key must latest-win replace");
        ok &= require(scheduler.queuedCount() == 1, "coalescing must keep one queue entry");
        const auto ready = scheduler.takeReady(1001);
        ok &= require(ready.has_value() && ready->token == 2,
                      "coalesced queue must dispatch the newest token");
        ok &= require(scheduler.telemetry().coalesced == 1,
                      "coalescing telemetry must increment");
    }

    {
        Scheduler scheduler;
        scheduler.beginSession(11);
        scheduler.enqueue(tx(11, 1, "background", Scheduler::Family::Immediate,
                             Scheduler::Priority::Background), 1000);
        scheduler.enqueue(tx(11, 2, "critical", Scheduler::Family::Immediate,
                             Scheduler::Priority::Critical), 1000);
        const auto first = scheduler.takeReady(1000);
        const auto second = scheduler.takeReady(1000);
        ok &= require(first.has_value() && first->token == 2,
                      "higher priority must dispatch first");
        ok &= require(second.has_value() && second->token == 1,
                      "lower priority must remain queued");
    }

    {
        Scheduler scheduler;
        scheduler.beginSession(12);
        scheduler.enqueue(tx(12, 1, "fifo-a"), 1000);
        scheduler.enqueue(tx(12, 2, "fifo-b"), 1000);
        const auto first = scheduler.takeReady(1000);
        const auto second = scheduler.takeReady(1000);
        ok &= require(first.has_value() && first->token == 1,
                      "equal priority must preserve FIFO sequence");
        ok &= require(second.has_value() && second->token == 2,
                      "FIFO second token mismatch");
    }

    {
        Scheduler::Config config;
        config.eqMinSpacingMs = 45;
        Scheduler scheduler(config);
        scheduler.beginSession(13);
        scheduler.enqueue(tx(13, 1, "eq:a", Scheduler::Family::Eq), 1000);
        const auto first = scheduler.takeReady(1000);
        ok &= require(first.has_value(), "first EQ transaction must be immediately ready");
        if (first)
            scheduler.noteDispatched(*first, 1000);

        scheduler.enqueue(tx(13, 2, "eq:b", Scheduler::Family::Eq), 1001);
        ok &= require(!scheduler.takeReady(1044).has_value(),
                      "EQ family must respect 45 ms pacing");
        ok &= require(scheduler.nextWakeDelayMs(1044) == 1,
                      "EQ wake delay must be deterministic");
        const auto second = scheduler.takeReady(1045);
        ok &= require(second.has_value() && second->token == 2,
                      "EQ transaction must become ready at 45 ms boundary");
    }

    {
        Scheduler::Config config;
        config.blockMinSpacingMs = 55;
        Scheduler scheduler(config);
        scheduler.beginSession(14);
        scheduler.enqueue(tx(14, 1, "block:a", Scheduler::Family::Block), 2000);
        const auto first = scheduler.takeReady(2000);
        ok &= require(first.has_value(), "first block transaction must be ready");
        if (first)
            scheduler.noteDispatched(*first, 2000);
        scheduler.enqueue(tx(14, 2, "block:b", Scheduler::Family::Block), 2001);
        ok &= require(!scheduler.takeReady(2054).has_value(),
                      "block family must respect 55 ms pacing");
        ok &= require(scheduler.takeReady(2055).has_value(),
                      "block transaction must become ready at 55 ms boundary");
    }

    {
        Scheduler::Config config;
        config.maxQueued = 2;
        Scheduler scheduler(config);
        scheduler.beginSession(15);
        scheduler.enqueue(tx(15, 1, "a", Scheduler::Family::Immediate,
                             Scheduler::Priority::Background), 1000);
        scheduler.enqueue(tx(15, 2, "b", Scheduler::Family::Immediate,
                             Scheduler::Priority::Background), 1000);
        ok &= require(scheduler.enqueue(tx(15, 3, "critical", Scheduler::Family::Immediate,
                                           Scheduler::Priority::Critical), 1001)
                          == Scheduler::EnqueueResult::AcceptedAfterEviction,
                      "critical input must deterministically evict oldest low-priority item");
        ok &= require(scheduler.queuedCount() == 2 && scheduler.telemetry().evicted == 1,
                      "bounded queue/eviction telemetry mismatch");
        const auto evicted = scheduler.takeDropped();
        ok &= require(evicted.size() == 1 && evicted.first().token == 1,
                      "evicted work must be surfaced for canonical rejection");
        const auto first = scheduler.takeReady(1001);
        ok &= require(first.has_value() && first->token == 3,
                      "critical transaction must survive backpressure");

        scheduler.clearQueued();
        const auto cancelled = scheduler.takeDropped();
        ok &= require(!cancelled.isEmpty(),
                      "clearQueued must surface cancelled transactions");
        scheduler.enqueue(tx(15, 4, "critical-a", Scheduler::Family::Immediate,
                             Scheduler::Priority::Critical), 1010);
        scheduler.enqueue(tx(15, 5, "critical-b", Scheduler::Family::Immediate,
                             Scheduler::Priority::Critical), 1010);
        ok &= require(scheduler.enqueue(tx(15, 6, "background-new", Scheduler::Family::Immediate,
                                           Scheduler::Priority::Background), 1011)
                          == Scheduler::EnqueueResult::RejectedBackpressure,
                      "lower-priority input must not evict critical work");
    }

    {
        Scheduler::Config config;
        config.maxQueueAgeMs = 100;
        Scheduler scheduler(config);
        scheduler.beginSession(16);
        scheduler.enqueue(tx(16, 1, "expire"), 1000);
        ok &= require(scheduler.expire(1101) == 1 && scheduler.empty(),
                      "aged transaction must expire deterministically");
        ok &= require(scheduler.telemetry().expired == 1,
                      "expiry telemetry must increment");
        const auto expired = scheduler.takeDropped();
        ok &= require(expired.size() == 1 && expired.first().token == 1,
                      "expired work must be surfaced for canonical rejection");
    }

    {
        Scheduler scheduler;
        scheduler.beginSession(20);
        scheduler.enqueue(tx(20, 1, "old-session"), 1000);
        scheduler.beginSession(21);
        ok &= require(scheduler.empty(), "new session must clear old queue");
        ok &= require(scheduler.enqueue(tx(20, 2, "late-old-session"), 1001)
                          == Scheduler::EnqueueResult::RejectedStaleSession,
                      "late command from old epoch must be rejected");
    }

    if (!ok)
        return 1;

    qInfo().noquote() << "P2 deterministic transaction scheduler self-test passed";
    return 0;
}
