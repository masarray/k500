#include "K500CanonicalState.h"

#include <QCoreApplication>
#include <QDebug>

namespace {
bool require(bool condition, const char *message)
{
    if (condition)
        return true;
    qCritical().noquote() << "P1 canonical state self-test failed:" << message;
    return false;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    K500CanonicalState state;
    QString error;

    QByteArray snapshot(K500CanonicalState::ActiveMemorySize, char(0));
    snapshot[0x10] = char(0x5A);
    snapshot[0x120] = char(0xA5);

    if (!require(!state.adoptSnapshot(snapshot, &error), "snapshot accepted without a session")) return 1;
    const quint64 epoch1 = state.beginSession();
    if (!require(epoch1 != 0 && state.sessionActive(), "session did not start")) return 2;
    if (!require(!state.adoptSnapshot(snapshot.left(snapshot.size() - 1), &error), "short snapshot accepted")) return 3;
    if (!require(state.adoptSnapshot(snapshot, &error), "exact 939-byte snapshot rejected")) return 4;
    if (!require(state.snapshotReady() && state.rawSnapshot() == snapshot, "snapshot was not adopted exactly")) return 5;
    if (!require(state.snapshotSha256().size() == 32, "snapshot digest is not SHA-256")) return 6;
    if (!require(state.snapshotGeneration() == 1, "initial snapshot generation mismatch")) return 7;

    QByteArray callerCopy = snapshot;
    callerCopy[0x10] = char(0x00);
    if (!require(static_cast<unsigned char>(state.rawSnapshot().at(0x10)) == 0x5A,
                 "stored snapshot was aliased/mutated by caller")) return 8;

    state.confirm(QStringLiteral("effects.reverb.level"), 55,
                  K500CanonicalState::Evidence::SnapshotCaptured);
    const auto confirmed = state.confirmed(QStringLiteral("effects.reverb.level"));
    if (!require(confirmed.has_value() && confirmed->value.toInt() == 55,
                 "ConfirmedState missing captured value")) return 9;

    quint64 revision1 = 0;
    if (!require(state.stageDesired(QStringLiteral("effects.reverb.level"), 61, &revision1, &error)
                 && revision1 != 0 && state.desiredCount() == 1,
                 "DesiredState did not stage")) return 10;

    const QByteArray frame1 = QByteArray::fromHex("aa010001fe");
    const auto plan1 = state.plan(QStringLiteral("effects.reverb.level"),
                                  QStringLiteral("fx:reverb"), frame1,
                                  QStringLiteral("Reverb level 61"), &error);
    if (!require(plan1.has_value() && plan1->valid() && state.inFlightCount() == 1,
                 "first command plan invalid")) return 11;

    quint64 revision2 = 0;
    if (!require(state.stageDesired(QStringLiteral("effects.reverb.level"), 62, &revision2, &error)
                 && revision2 > revision1,
                 "second DesiredState revision did not advance")) return 12;
    const QByteArray frame2 = QByteArray::fromHex("aa010002fd");
    const auto plan2 = state.plan(QStringLiteral("effects.reverb.level"),
                                  QStringLiteral("fx:reverb"), frame2,
                                  QStringLiteral("Reverb level 62"), &error);
    if (!require(plan2.has_value() && plan2->token != plan1->token,
                 "latest command did not receive a new token")) return 13;
    if (!require(!state.isCurrent(*plan1) && state.isCurrent(*plan2)
                 && state.inFlightCount() == 1,
                 "latest-wins coalescing state is incorrect")) return 14;

    // P3 must never cross an active command envelope.
    QByteArray reconcileImage = snapshot;
    reconcileImage[0x10] = char(0x62);
    if (!require(!state.reconcileSnapshot(reconcileImage, &error),
                 "P3 reconciliation crossed an in-flight command")) return 15;

    if (!require(state.markDispatched(plan2->token), "current command could not be marked dispatched")) return 16;
    if (!require(state.completeTransport(epoch1, plan2->token, true)
                 && state.inFlightCount() == 0 && state.desiredCount() == 1,
                 "transport completion incorrectly confirmed hardware state")) return 17;

    // P3 authoritative reconciliation preserves intent, replaces hardware truth,
    // and clears old ConfirmedState before semantic values are rebuilt.
    if (!require(state.reconcileSnapshot(reconcileImage, &error),
                 "P3 authoritative snapshot rejected")) return 18;
    if (!require(state.snapshotGeneration() == 2 && state.desiredCount() == 1
                 && state.confirmedCount() == 0,
                 "P3 reconciliation did not preserve unresolved intent cleanly")) return 19;

    state.confirm(QStringLiteral("effects.reverb.level"), 62,
                  K500CanonicalState::Evidence::SnapshotCaptured);
    if (!require(state.desiredCount() == 0 && state.divergentDesiredCount() == 0,
                 "matching authoritative confirmation did not reconcile DesiredState")) return 20;

    // A hardware mismatch remains explicit instead of being called success.
    if (!require(state.stageDesired(QStringLiteral("music.key"), 3, nullptr, &error),
                 "could not stage P3 divergence case")) return 21;
    QByteArray divergenceImage = reconcileImage;
    divergenceImage[0x11] = char(0x07);
    if (!require(state.reconcileSnapshot(divergenceImage, &error),
                 "P3 divergence snapshot rejected")) return 22;
    state.confirm(QStringLiteral("music.key"), 0,
                  K500CanonicalState::Evidence::SnapshotDerived);
    if (!require(state.desiredCount() == 1 && state.divergentDesiredCount() == 1
                 && state.divergentDesiredPaths() == QStringList{QStringLiteral("music.key")},
                 "P3 hardware mismatch was not retained as explicit divergence")) return 23;
    if (!require(!state.reconcileSnapshot(divergenceImage.left(divergenceImage.size() - 1), &error),
                 "short P3 reconciliation snapshot accepted")) return 24;

    // Clear the deliberate mismatch by matching hardware truth before testing
    // the reconnect/session invalidation rules from P1.
    state.confirm(QStringLiteral("music.key"), 3,
                  K500CanonicalState::Evidence::SnapshotDerived);
    if (!require(state.desiredCount() == 0, "P3 divergence cleanup failed")) return 25;

    quint64 staleRevision = 0;
    if (!require(state.stageDesired(QStringLiteral("music.key"), 3, &staleRevision, &error),
                 "could not stage pre-reconnect state")) return 26;
    const auto stalePlan = state.plan(QStringLiteral("music.key"), QStringLiteral("top:music"),
                                      QByteArray::fromHex("aa010003fc"), QStringLiteral("Music key"), &error);
    if (!require(stalePlan.has_value(), "could not create pre-reconnect plan")) return 27;

    state.endSession();
    if (!require(!state.sessionActive() && !state.snapshotReady()
                 && state.desiredCount() == 0 && state.inFlightCount() == 0
                 && state.snapshotGeneration() == 0,
                 "endSession did not invalidate session-bound state")) return 28;

    const quint64 epoch2 = state.beginSession();
    if (!require(epoch2 > epoch1, "session epoch did not advance")) return 29;
    if (!require(state.adoptSnapshot(snapshot, &error), "new-session snapshot rejected")) return 30;
    if (!require(!state.isCurrent(*stalePlan)
                 && !state.completeTransport(epoch1, stalePlan->token, true),
                 "stale prior-session command survived reconnect")) return 31;

    qInfo().noquote() << "P3 authoritative reconciliation self-test passed";
    qInfo().noquote() << "P1 canonical state self-test passed"
                      << "epoch" << epoch2
                      << "snapshotSha256" << state.snapshotSha256Hex();
    return 0;
}
