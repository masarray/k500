#include "K500CanonicalState.h"

#include <QCoreApplication>
#include <QDebug>

namespace {
bool require(bool condition, const char *message)
{
    if (condition)
        return true;
    qCritical().noquote() << "P3 authoritative reconciliation self-test failed:" << message;
    return false;
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    K500CanonicalState state;
    QString error;

    QByteArray initial(K500CanonicalState::ActiveMemorySize, char(0));
    initial[0x10] = char(0x55);
    const quint64 epoch = state.beginSession();
    if (!require(epoch != 0 && state.adoptSnapshot(initial, &error),
                 "initial authoritative snapshot rejected")) return 1;
    if (!require(state.snapshotGeneration() == 1,
                 "initial snapshot generation must be one")) return 2;

    state.confirm(QStringLiteral("effects.reverb.level"), 55,
                  K500CanonicalState::Evidence::SnapshotCaptured);
    quint64 revision = 0;
    if (!require(state.stageDesired(QStringLiteral("effects.reverb.level"), 61,
                                    &revision, &error),
                 "could not stage desired value")) return 3;
    const auto plan = state.plan(QStringLiteral("effects.reverb.level"),
                                 QStringLiteral("fx:reverb"),
                                 QByteArray::fromHex("aa010001fe"),
                                 QStringLiteral("reverb 61"), &error);
    if (!require(plan.has_value(),
                 "could not build reconciliation test command")) return 4;

    QByteArray readback = initial;
    readback[0x10] = char(0x61);
    if (!require(!state.reconcileSnapshot(readback, &error),
                 "reconciliation crossed an in-flight command barrier")) return 5;
    if (!require(state.markDispatched(plan->token)
                 && state.completeTransport(epoch, plan->token, true),
                 "transport completion failed")) return 6;
    if (!require(state.desiredCount() == 1,
                 "transport acceptance incorrectly confirmed DesiredState")) return 7;

    if (!require(state.reconcileSnapshot(readback, &error),
                 "authoritative reconciliation snapshot rejected")) return 8;
    if (!require(state.snapshotGeneration() == 2 && state.desiredCount() == 1,
                 "reconciliation did not preserve unresolved intent")) return 9;
    if (!require(state.confirmedCount() == 0,
                 "old ConfirmedState survived a new authoritative image")) return 10;

    state.confirm(QStringLiteral("effects.reverb.level"), 61,
                  K500CanonicalState::Evidence::SnapshotCaptured);
    if (!require(state.desiredCount() == 0 && state.divergentDesiredCount() == 0,
                 "matching hardware truth did not converge DesiredState")) return 11;

    if (!require(state.stageDesired(QStringLiteral("music.key"), 3, nullptr, &error),
                 "could not stage divergence case")) return 12;
    QByteArray mismatch = readback;
    mismatch[0x11] = char(0x07);
    if (!require(state.reconcileSnapshot(mismatch, &error),
                 "second reconciliation snapshot rejected")) return 13;
    state.confirm(QStringLiteral("music.key"), 0,
                  K500CanonicalState::Evidence::SnapshotDerived);
    const QStringList divergent = state.divergentDesiredPaths();
    if (!require(state.desiredCount() == 1 && state.divergentDesiredCount() == 1
                 && divergent == QStringList{QStringLiteral("music.key")},
                 "authoritative mismatch was not retained as explicit divergence")) return 14;

    if (!require(!state.reconcileSnapshot(mismatch.left(mismatch.size() - 1), &error),
                 "short reconciliation snapshot accepted")) return 15;

    qInfo().noquote() << "P3 authoritative reconciliation self-test passed"
                      << "generation" << state.snapshotGeneration()
                      << "divergent" << state.divergentDesiredCount();
    return 0;
}
