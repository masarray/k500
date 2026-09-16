#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>

#include <optional>

// P1_CANONICAL_DEVICE_STATE_V1
// Session-bound source of truth between user intent and the native transport.
// The 939-byte Retrieve-All snapshot is immutable after adoption; UI edits are
// tracked separately as DesiredState and never rewrite ConfirmedState.
class K500CanonicalState final
{
public:
    static constexpr int ActiveMemorySize = 0x03AB;

    enum class Evidence : quint8 {
        Unknown = 0,
        SnapshotCaptured,
        SnapshotDerived,
        AssumedMetadata,
        UserIntent,
    };

    enum class CommandPhase : quint8 {
        Queued = 0,
        Dispatched,
    };

    struct ValueState {
        QVariant value;
        Evidence evidence = Evidence::Unknown;
        quint64 sessionEpoch = 0;
        quint64 revision = 0;
    };

    struct CommandPlan {
        quint64 sessionEpoch = 0;
        quint64 revision = 0;
        quint64 token = 0;
        QString semanticPath;
        QString coalescingKey;
        QByteArray frame;
        QString label;

        bool valid() const
        {
            return sessionEpoch != 0 && revision != 0 && token != 0
                && !semanticPath.isEmpty() && !coalescingKey.isEmpty()
                && !frame.isEmpty();
        }
    };

    struct InFlightState {
        CommandPlan command;
        CommandPhase phase = CommandPhase::Queued;
    };

    quint64 beginSession();
    void endSession();

    bool sessionActive() const { return m_sessionActive; }
    quint64 sessionEpoch() const { return m_sessionEpoch; }

    bool adoptSnapshot(const QByteArray &memory, QString *error = nullptr);
    // P3_AUTHORITATIVE_RECONCILIATION_V1 — replace hardware truth after a
    // deliberate readback barrier while preserving unresolved DesiredState.
    bool reconcileSnapshot(const QByteArray &memory, QString *error = nullptr);
    bool snapshotReady() const { return m_snapshotReady; }
    quint64 snapshotGeneration() const { return m_snapshotGeneration; }
    QByteArray rawSnapshot() const { return m_rawSnapshot; }
    QByteArray snapshotSha256() const { return m_snapshotSha256; }
    QString snapshotSha256Hex() const { return QString::fromLatin1(m_snapshotSha256.toHex()); }

    void confirm(const QString &path, const QVariant &value, Evidence evidence);
    std::optional<ValueState> confirmed(const QString &path) const;
    std::optional<ValueState> desired(const QString &path) const;

    bool stageDesired(const QString &path, const QVariant &value,
                      quint64 *revision = nullptr, QString *reason = nullptr);
    void discardDesired(const QString &path);

    std::optional<CommandPlan> plan(const QString &path,
                                    const QString &coalescingKey,
                                    const QByteArray &frame,
                                    const QString &label,
                                    QString *reason = nullptr);
    bool isCurrent(const CommandPlan &plan) const;
    bool markDispatched(quint64 token);
    bool completeTransport(quint64 sessionEpoch, quint64 token, bool accepted);

    int confirmedCount() const { return m_confirmed.size(); }
    int desiredCount() const { return m_desired.size(); }
    int inFlightCount() const { return m_inFlightByKey.size(); }
    int divergentDesiredCount() const;
    QStringList divergentDesiredPaths() const;

    static QString evidenceName(Evidence evidence);

private:
    void resetSessionPayload();

    bool m_sessionActive = false;
    bool m_snapshotReady = false;
    quint64 m_sessionEpoch = 0;
    quint64 m_nextRevision = 1;
    quint64 m_nextToken = 1;

    QByteArray m_rawSnapshot;
    QByteArray m_snapshotSha256;
    quint64 m_snapshotGeneration = 0;
    QHash<QString, ValueState> m_confirmed;
    QHash<QString, ValueState> m_desired;
    QHash<QString, InFlightState> m_inFlightByKey;
    QHash<quint64, QString> m_keyByToken;
};
