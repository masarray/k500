#include "K500CanonicalState.h"

#include <QCryptographicHash>

namespace {
void setReason(QString *reason, const QString &text)
{
    if (reason)
        *reason = text;
}
}

quint64 K500CanonicalState::beginSession()
{
    ++m_sessionEpoch;
    if (m_sessionEpoch == 0)
        ++m_sessionEpoch;
    m_sessionActive = true;
    resetSessionPayload();
    return m_sessionEpoch;
}

void K500CanonicalState::endSession()
{
    m_sessionActive = false;
    resetSessionPayload();
}

void K500CanonicalState::resetSessionPayload()
{
    m_snapshotReady = false;
    m_rawSnapshot.clear();
    m_snapshotSha256.clear();
    m_snapshotGeneration = 0;
    m_confirmed.clear();
    m_desired.clear();
    m_inFlightByKey.clear();
    m_keyByToken.clear();
    m_nextRevision = 1;
    m_nextToken = 1;
}

bool K500CanonicalState::adoptSnapshot(const QByteArray &memory, QString *error)
{
    if (!m_sessionActive) {
        setReason(error, QStringLiteral("No active device session"));
        return false;
    }
    if (memory.size() != ActiveMemorySize) {
        setReason(error, QStringLiteral("Retrieve-All snapshot must be exactly %1 bytes, got %2")
                             .arg(ActiveMemorySize).arg(memory.size()));
        return false;
    }

    // QByteArray is implicitly shared. detach() guarantees this store owns a
    // private immutable copy even if the transport reuses/modifies its buffer.
    m_rawSnapshot = memory;
    m_rawSnapshot.detach();
    m_snapshotSha256 = QCryptographicHash::hash(m_rawSnapshot, QCryptographicHash::Sha256);
    m_snapshotGeneration = 1;
    m_snapshotReady = true;

    // A complete initial/Recall readback is an authority barrier. Any
    // pre-readback intent or queued command is stale relative to hardware truth.
    m_confirmed.clear();
    m_desired.clear();
    m_inFlightByKey.clear();
    m_keyByToken.clear();
    m_nextRevision = 1;
    m_nextToken = 1;
    setReason(error, {});
    return true;
}

bool K500CanonicalState::reconcileSnapshot(const QByteArray &memory, QString *error)
{
    // P3_AUTHORITATIVE_RECONCILIATION_V1
    if (!m_sessionActive) {
        setReason(error, QStringLiteral("No active device session"));
        return false;
    }
    if (!m_snapshotReady) {
        setReason(error, QStringLiteral("Initial authoritative snapshot has not been adopted"));
        return false;
    }
    if (memory.size() != ActiveMemorySize) {
        setReason(error, QStringLiteral("Reconciliation snapshot must be exactly %1 bytes, got %2")
                             .arg(ActiveMemorySize).arg(memory.size()));
        return false;
    }
    if (!m_inFlightByKey.isEmpty()) {
        setReason(error, QStringLiteral("Cannot reconcile while canonical commands are still in flight"));
        return false;
    }

    m_rawSnapshot = memory;
    m_rawSnapshot.detach();
    m_snapshotSha256 = QCryptographicHash::hash(m_rawSnapshot, QCryptographicHash::Sha256);
    ++m_snapshotGeneration;
    if (m_snapshotGeneration == 0)
        m_snapshotGeneration = 1;

    // ConfirmedState is rebuilt from this authoritative image by Controller.
    // DesiredState intentionally survives until confirm() sees the same value.
    m_confirmed.clear();
    setReason(error, {});
    return true;
}

void K500CanonicalState::confirm(const QString &path, const QVariant &value, Evidence evidence)
{
    if (!m_sessionActive || !m_snapshotReady || path.trimmed().isEmpty())
        return;

    const QString key = path.trimmed();
    ValueState state;
    state.value = value;
    state.evidence = evidence;
    state.sessionEpoch = m_sessionEpoch;
    state.revision = 0;
    m_confirmed.insert(key, state);

    const auto desiredIt = m_desired.constFind(key);
    if (desiredIt != m_desired.constEnd() && desiredIt->sessionEpoch == m_sessionEpoch
        && desiredIt->value == value) {
        m_desired.remove(key);
    }
}

std::optional<K500CanonicalState::ValueState> K500CanonicalState::confirmed(const QString &path) const
{
    const auto it = m_confirmed.constFind(path.trimmed());
    if (it == m_confirmed.constEnd())
        return std::nullopt;
    return *it;
}

std::optional<K500CanonicalState::ValueState> K500CanonicalState::desired(const QString &path) const
{
    const auto it = m_desired.constFind(path.trimmed());
    if (it == m_desired.constEnd())
        return std::nullopt;
    return *it;
}

bool K500CanonicalState::stageDesired(const QString &path, const QVariant &value,
                                      quint64 *revision, QString *reason)
{
    const QString key = path.trimmed();
    if (!m_sessionActive) {
        setReason(reason, QStringLiteral("Device session is not active"));
        return false;
    }
    if (!m_snapshotReady) {
        setReason(reason, QStringLiteral("Canonical state has no complete Retrieve-All snapshot"));
        return false;
    }
    if (key.isEmpty()) {
        setReason(reason, QStringLiteral("Semantic path is empty"));
        return false;
    }

    ValueState state;
    state.value = value;
    state.evidence = Evidence::UserIntent;
    state.sessionEpoch = m_sessionEpoch;
    state.revision = m_nextRevision++;
    if (m_nextRevision == 0)
        m_nextRevision = 1;
    m_desired.insert(key, state);
    if (revision)
        *revision = state.revision;
    setReason(reason, {});
    return true;
}

void K500CanonicalState::discardDesired(const QString &path)
{
    m_desired.remove(path.trimmed());
}

std::optional<K500CanonicalState::CommandPlan> K500CanonicalState::plan(
    const QString &path,
    const QString &coalescingKey,
    const QByteArray &frame,
    const QString &label,
    QString *reason)
{
    const QString semanticPath = path.trimmed();
    const QString key = coalescingKey.trimmed();
    if (!m_sessionActive || !m_snapshotReady) {
        setReason(reason, QStringLiteral("Canonical session is not writable"));
        return std::nullopt;
    }
    if (semanticPath.isEmpty() || key.isEmpty() || frame.isEmpty()) {
        setReason(reason, QStringLiteral("Command plan is incomplete"));
        return std::nullopt;
    }

    const auto desiredIt = m_desired.constFind(semanticPath);
    if (desiredIt == m_desired.constEnd() || desiredIt->sessionEpoch != m_sessionEpoch) {
        setReason(reason, QStringLiteral("No current DesiredState for %1").arg(semanticPath));
        return std::nullopt;
    }

    CommandPlan plan;
    plan.sessionEpoch = m_sessionEpoch;
    plan.revision = desiredIt->revision;
    plan.token = m_nextToken++;
    if (m_nextToken == 0)
        m_nextToken = 1;
    plan.semanticPath = semanticPath;
    plan.coalescingKey = key;
    plan.frame = frame;
    plan.label = label;

    // Latest-wins replacement is explicit in state even before P2 introduces
    // the transport scheduler. The old token can no longer become current.
    const auto old = m_inFlightByKey.constFind(key);
    if (old != m_inFlightByKey.constEnd())
        m_keyByToken.remove(old->command.token);

    m_inFlightByKey.insert(key, InFlightState{plan, CommandPhase::Queued});
    m_keyByToken.insert(plan.token, key);
    setReason(reason, {});
    return plan;
}

bool K500CanonicalState::isCurrent(const CommandPlan &plan) const
{
    if (!plan.valid() || !m_sessionActive || !m_snapshotReady
        || plan.sessionEpoch != m_sessionEpoch)
        return false;

    const auto desiredIt = m_desired.constFind(plan.semanticPath);
    if (desiredIt == m_desired.constEnd()
        || desiredIt->sessionEpoch != m_sessionEpoch
        || desiredIt->revision != plan.revision)
        return false;

    const auto inFlightIt = m_inFlightByKey.constFind(plan.coalescingKey);
    return inFlightIt != m_inFlightByKey.constEnd()
        && inFlightIt->command.token == plan.token
        && inFlightIt->command.sessionEpoch == m_sessionEpoch;
}

bool K500CanonicalState::markDispatched(quint64 token)
{
    const auto keyIt = m_keyByToken.constFind(token);
    if (keyIt == m_keyByToken.constEnd())
        return false;
    auto inFlightIt = m_inFlightByKey.find(*keyIt);
    if (inFlightIt == m_inFlightByKey.end() || !isCurrent(inFlightIt->command))
        return false;
    inFlightIt->phase = CommandPhase::Dispatched;
    return true;
}

bool K500CanonicalState::completeTransport(quint64 sessionEpoch, quint64 token, bool accepted)
{
    if (!m_sessionActive || sessionEpoch != m_sessionEpoch)
        return false;
    const auto keyIt = m_keyByToken.constFind(token);
    if (keyIt == m_keyByToken.constEnd())
        return false;
    const QString key = *keyIt;
    auto inFlightIt = m_inFlightByKey.find(key);
    if (inFlightIt == m_inFlightByKey.end()
        || inFlightIt->command.token != token
        || inFlightIt->command.sessionEpoch != sessionEpoch)
        return false;

    // Transport acceptance is not hardware confirmation. DesiredState remains
    // until a later authoritative readback confirms the same semantic value.
    Q_UNUSED(accepted);
    m_inFlightByKey.erase(inFlightIt);
    m_keyByToken.remove(token);
    return true;
}

QStringList K500CanonicalState::divergentDesiredPaths() const
{
    QStringList paths;
    for (auto it = m_desired.constBegin(); it != m_desired.constEnd(); ++it) {
        if (it->sessionEpoch != m_sessionEpoch)
            continue;
        const auto confirmedIt = m_confirmed.constFind(it.key());
        if (confirmedIt == m_confirmed.constEnd()
            || confirmedIt->sessionEpoch != m_sessionEpoch)
            continue;
        if (confirmedIt->value != it->value)
            paths.append(it.key());
    }
    paths.sort(Qt::CaseSensitive);
    return paths;
}

int K500CanonicalState::divergentDesiredCount() const
{
    return divergentDesiredPaths().size();
}

QString K500CanonicalState::evidenceName(Evidence evidence)
{
    switch (evidence) {
    case Evidence::SnapshotCaptured: return QStringLiteral("snapshot-captured");
    case Evidence::SnapshotDerived: return QStringLiteral("snapshot-derived");
    case Evidence::AssumedMetadata: return QStringLiteral("assumed-metadata");
    case Evidence::UserIntent: return QStringLiteral("user-intent");
    case Evidence::Unknown:
    default:
        return QStringLiteral("unknown");
    }
}
