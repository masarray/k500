from pathlib import Path
import re


def replace_once(text, old, new, label):
    if old not in text:
        raise SystemExit(f"P1 patch anchor missing: {label}")
    return text.replace(old, new, 1)


# ---- CMake: production state core + deterministic self-test ----
p = Path("CMakeLists.txt")
text = p.read_text(encoding="utf-8")
text = replace_once(
    text,
    "    src/k500/K500Controller.cpp\n    src/k500/K500Controller.h\n",
    "    src/k500/K500Controller.cpp\n    src/k500/K500Controller.h\n    src/k500/K500CanonicalState.cpp\n    src/k500/K500CanonicalState.h\n",
    "CMake production state sources",
)
anchor = "target_link_libraries(k500_p0_perf_selftest PRIVATE Qt6::Core)\nif(WIN32)\n    target_link_libraries(k500_p0_perf_selftest PRIVATE psapi)\nendif()\n"
addition = anchor + """
# P1_CANONICAL_DEVICE_STATE_V1 — hardware-free session/state qualification.
add_executable(k500_p1_state_selftest
    src/k500/P1CanonicalStateSelfTestMain.cpp
    src/k500/K500CanonicalState.cpp
    src/k500/K500CanonicalState.h
)
target_include_directories(k500_p1_state_selftest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(k500_p1_state_selftest PRIVATE Qt6::Core)
"""
text = replace_once(text, anchor, addition, "CMake P1 self-test target")
p.write_text(text, encoding="utf-8")


# ---- Controller header: canonical state + session-bound command envelope ----
p = Path("src/k500/K500Controller.h")
text = p.read_text(encoding="utf-8")
text = replace_once(
    text,
    '#include "K500Protocol.h"\n',
    '#include "K500Protocol.h"\n#include "K500CanonicalState.h"\n',
    "controller state include",
)
text = replace_once(
    text,
    "    Q_PROPERTY(bool deviceReadbackReady READ deviceReadbackReady NOTIFY deviceReadbackReadyChanged)\n",
    """    Q_PROPERTY(bool deviceReadbackReady READ deviceReadbackReady NOTIFY deviceReadbackReadyChanged)
    Q_PROPERTY(qulonglong sessionEpoch READ sessionEpoch NOTIFY canonicalStateChanged)
    Q_PROPERTY(bool canonicalStateReady READ canonicalStateReady NOTIFY canonicalStateChanged)
    Q_PROPERTY(QString canonicalSnapshotSha256 READ canonicalSnapshotSha256 NOTIFY canonicalStateChanged)
    Q_PROPERTY(int desiredStateCount READ desiredStateCount NOTIFY canonicalStateChanged)
    Q_PROPERTY(int inFlightStateCount READ inFlightStateCount NOTIFY canonicalStateChanged)
""",
    "controller canonical properties",
)
text = replace_once(
    text,
    "    bool liveEnabled() const { return m_liveEnabled; }\n    bool deviceReadbackReady() const { return m_deviceScalars.size() >= 0x40; }\n",
    """    bool liveEnabled() const { return m_liveEnabled; }
    bool deviceReadbackReady() const { return m_deviceScalars.size() >= 0x40; }
    qulonglong sessionEpoch() const { return m_canonicalState.sessionEpoch(); }
    bool canonicalStateReady() const { return m_canonicalState.snapshotReady(); }
    QString canonicalSnapshotSha256() const { return m_canonicalState.snapshotSha256Hex(); }
    int desiredStateCount() const { return m_canonicalState.desiredCount(); }
    int inFlightStateCount() const { return m_canonicalState.inFlightCount(); }
""",
    "controller canonical getters",
)
text = replace_once(
    text,
    "public slots:\n    void setLiveEnabled(bool enabled);\n",
    """public slots:
    void beginDeviceSession();
    void endDeviceSession();
    void setLiveEnabled(bool enabled);
""",
    "controller session slots",
)
text = replace_once(
    text,
    "    void handleStateEdit(const QString &path, const QVariant &value);\n",
    """    void handleStateEdit(const QString &path, const QVariant &value);
    void handleCommandDispatchResult(quint64 sessionEpoch, quint64 token,
                                     const QString &path, bool accepted,
                                     const QString &reason);
""",
    "controller dispatch result slot",
)
text = replace_once(
    text,
    "    void deviceReadbackReadyChanged();\n    void frameReady(const QByteArray &frame, const QString &label);\n",
    """    void deviceReadbackReadyChanged();
    void canonicalStateChanged();
    void commandReady(quint64 sessionEpoch, quint64 token,
                      const QByteArray &frame, const QString &label,
                      const QString &path, const QString &coalescingKey);
    // Compatibility/diagnostic signal. DeviceManager no longer transports this
    // directly; commandReady() is the authoritative session-bound envelope.
    void frameReady(const QByteArray &frame, const QString &label);
""",
    "controller command signal",
)
text = replace_once(
    text,
    """    struct PendingFrame {
        QByteArray frame;
        QString label;
    };
""",
    """    struct PendingFrame {
        K500CanonicalState::CommandPlan command;
    };
""",
    "controller pending command",
)
text = replace_once(
    text,
    "    void queueEqFrame(const QString &key, const QByteArray &frame, const QString &label);\n    void queueBlockFrame(const QString &key, const QByteArray &frame, const QString &label);\n",
    """    void queueEqFrame(const QString &key, const QString &path,
                      const QByteArray &frame, const QString &label);
    void queueBlockFrame(const QString &key, const QString &path,
                         const QByteArray &frame, const QString &label);
""",
    "controller queue signatures",
)
text = replace_once(
    text,
    "    bool updateOutputState(const QString &section, const QString &field, const QVariant &value);\n",
    """    bool updateOutputState(const QString &section, const QString &field, const QVariant &value);
    void rejectUnsupported(const QString &path);
    void deferWrite(const QString &path, const QString &reason);
    void recordConfirmedState(const QByteArray &memory);
""",
    "controller state helpers",
)
text = replace_once(
    text,
    "    bool m_liveEnabled = false;\n    QByteArray m_deviceScalars;\n    QByteArray m_activeMemory;\n",
    """    bool m_liveEnabled = false;
    K500CanonicalState m_canonicalState;
    QByteArray m_deviceScalars;
""",
    "controller canonical member",
)
p.write_text(text, encoding="utf-8")


# ---- Controller implementation ----
p = Path("src/k500/K500Controller.cpp")
text = p.read_text(encoding="utf-8")
constructor_end = """    connect(&m_eqTimer, &QTimer::timeout, this, &K500Controller::flushEqFrames);
    connect(&m_blockTimer, &QTimer::timeout, this, &K500Controller::flushBlockFrames);
}

void K500Controller::setLiveEnabled(bool enabled)
"""
constructor_new = """    connect(&m_eqTimer, &QTimer::timeout, this, &K500Controller::flushEqFrames);
    connect(&m_blockTimer, &QTimer::timeout, this, &K500Controller::flushBlockFrames);
}

void K500Controller::beginDeviceSession()
{
    setLiveEnabled(false);
    m_canonicalState.beginSession();
    emit canonicalStateChanged();
}

void K500Controller::endDeviceSession()
{
    setLiveEnabled(false);
    m_canonicalState.endSession();
    emit canonicalStateChanged();
}

void K500Controller::setLiveEnabled(bool enabled)
"""
text = replace_once(text, constructor_end, constructor_new, "controller session methods")

hydrate_old = """void K500Controller::hydrateFromDeviceMemory(const QByteArray &memory)
{
    if (memory.size() < 0x40)
        return;

    // P1_CONTROLLER_DEVICE_SEED_V1
    // Seed every complete-block write from device truth while LIVE is still OFF.
    // No P1 edit may invent neighbouring/reserved values.
    m_activeMemory = memory;
    setDeviceScalars(memory.left(0x40));
"""
hydrate_new = """void K500Controller::hydrateFromDeviceMemory(const QByteArray &memory)
{
    // P1_CANONICAL_SNAPSHOT_BARRIER_V1 — only an exact Retrieve-All image can
    // become hardware truth. No partial buffer may unlock native writes.
    if (memory.size() != K500CanonicalState::ActiveMemorySize)
        return;
    if (!m_canonicalState.sessionActive())
        m_canonicalState.beginSession(); // deterministic harness/recovery fallback
    QString snapshotError;
    if (!m_canonicalState.adoptSnapshot(memory, &snapshotError)) {
        emit writeDeferred(QStringLiteral("__snapshot__"), snapshotError);
        return;
    }
    emit canonicalStateChanged();

    // Seed every complete-block write from the immutable device snapshot while
    // LIVE is still OFF. Unknown/reserved neighbours remain device-owned.
    setDeviceScalars(memory.left(0x40));
"""
text = replace_once(text, hydrate_old, hydrate_new, "controller hydrate barrier")

crossover_tail = """    seedCrossover(QStringLiteral("reverb"), 0x00C0, 0x00C2, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    seedCrossover(QStringLiteral("echo"), 0x00C4, 0x00C6, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
}
"""
crossover_new = """    seedCrossover(QStringLiteral("reverb"), 0x00C0, 0x00C2, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    seedCrossover(QStringLiteral("echo"), 0x00C4, 0x00C6, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));

    recordConfirmedState(memory);
    emit canonicalStateChanged();
}
"""
text = replace_once(text, crossover_tail, crossover_new, "controller confirmed state record")

text = text.replace("    m_activeMemory.clear();\n", "")
text = replace_once(
    text,
    "void K500Controller::clearDeviceState()\n{\n    const bool wasReady = deviceReadbackReady();\n",
    "void K500Controller::clearDeviceState()\n{\n    const bool wasReady = deviceReadbackReady();\n    m_canonicalState.endSession();\n",
    "controller clear canonical state",
)
text = replace_once(
    text,
    "    if (wasReady)\n        emit deviceReadbackReadyChanged();\n}\n\nvoid K500Controller::handleStateEdit",
    "    if (wasReady)\n        emit deviceReadbackReadyChanged();\n    emit canonicalStateChanged();\n}\n\nvoid K500Controller::handleStateEdit",
    "controller clear signal",
)

handle_anchor = """void K500Controller::handleStateEdit(const QString &path, const QVariant &value)
{
    // P1_FULL_LIVE_ROUTING_V1
"""
handle_new = """void K500Controller::handleStateEdit(const QString &path, const QVariant &value)
{
    // P1_CANONICAL_EDIT_GATE_V1 — DesiredState is session-bound and can only
    // exist after the exact Retrieve-All snapshot has become authoritative.
    if (!m_liveEnabled)
        return;
    QString canonicalReason;
    if (!m_canonicalState.stageDesired(path, value, nullptr, &canonicalReason)) {
        emit writeDeferred(path, canonicalReason);
        return;
    }
    emit canonicalStateChanged();

    // P1_FULL_LIVE_ROUTING_V1
"""
text = replace_once(text, handle_anchor, handle_new, "controller edit gate")

# Never mutate the confirmed/raw snapshot optimistically.
text = re.sub(
    r"\n        if \(m_activeMemory\.size\(\) > 0x027F\) \{\n            m_activeMemory\[0x027D\] = char\(m_eqBypass\.m0\);\n            m_activeMemory\[0x027E\] = char\(m_eqBypass\.m1\);\n            m_activeMemory\[0x027F\] = char\(m_eqBypass\.m2\);\n        \}",
    "",
    text,
    count=1,
)

# Route unsupported/deferred exits through DesiredState cleanup helpers.
text = text.replace("emit unsupportedPath(path);", "rejectUnsupported(path);")
text = text.replace("emit writeDeferred(path,", "deferWrite(path,")

# EQ planner carries semantic path.
text = replace_once(
    text,
    'queueEqFrame(QStringLiteral("%1:%2").arg(section).arg(index), frame,\n',
    'queueEqFrame(QStringLiteral("%1:%2").arg(section).arg(index), path, frame,\n',
    "EQ queue semantic path",
)

# Every block planner receives the semantic path explicitly.
patterns = [
    ('queueBlockFrame(QStringLiteral("mic:eqLink"), K500Protocol::micEqLink(value.toBool()),',
     'queueBlockFrame(QStringLiteral("mic:eqLink"), path, K500Protocol::micEqLink(value.toBool()),'),
    ('queueBlockFrame(QStringLiteral("top:music"), K500Protocol::topMusicBlock(m_music, m_deviceScalars),',
     'queueBlockFrame(QStringLiteral("top:music"), path, K500Protocol::topMusicBlock(m_music, m_deviceScalars),'),
    ('queueBlockFrame(QStringLiteral("top:mic"), K500Protocol::topMicBlock(m_mic, scalars),',
     'queueBlockFrame(QStringLiteral("top:mic"), path, K500Protocol::topMicBlock(m_mic, scalars),'),
    ('queueBlockFrame(QStringLiteral("top:effect"), K500Protocol::topEffectBlock(m_effect, m_deviceScalars),',
     'queueBlockFrame(QStringLiteral("top:effect"), path, K500Protocol::topEffectBlock(m_effect, m_deviceScalars),'),
    ('queueBlockFrame(QStringLiteral("fx:reverb"), K500Protocol::reverbBlock(m_reverb, m_reverbRaw),',
     'queueBlockFrame(QStringLiteral("fx:reverb"), path, K500Protocol::reverbBlock(m_reverb, m_reverbRaw),'),
    ('queueBlockFrame(QStringLiteral("fx:echo"), K500Protocol::echoBlock(m_echo, m_echoRaw),',
     'queueBlockFrame(QStringLiteral("fx:echo"), path, K500Protocol::echoBlock(m_echo, m_echoRaw),'),
    ('queueBlockFrame(QStringLiteral("eq:bypass-global"), K500Protocol::eqBypassWrite(m_eqBypass),',
     'queueBlockFrame(QStringLiteral("eq:bypass-global"), path, K500Protocol::eqBypassWrite(m_eqBypass),'),
    ('queueBlockFrame(QStringLiteral("output:%1").arg(section), frame,',
     'queueBlockFrame(QStringLiteral("output:%1").arg(section), path, frame,'),
    ('queueBlockFrame(QStringLiteral("xover:%1:%2").arg(section, kind), frame,',
     'queueBlockFrame(QStringLiteral("xover:%1:%2").arg(section, kind), path, frame,'),
]
for old, new in patterns:
    text = replace_once(text, old, new, old[:45])

# Confirmed scalar bytes stay immutable. Overlay latest typed FBE only into the
# outgoing Mic image, never into m_deviceScalars.
old_fbe = """    QByteArray scalars = m_deviceScalars;
    if (path == QStringLiteral("mic.fbxLevel") && scalars.size() > 0x1B) {
        const char fbe = char(K500Frame::clampByte(qBound(0, m_mic.fbxLevel, 3)));
        scalars[0x1B] = fbe;
        // Optimistically keep device truth aligned so later Mic Volume/full-block
        // writes do not restore the pre-edit FBE value before the next readback.
        m_deviceScalars[0x1B] = fbe;
    }
"""
new_fbe = """    QByteArray scalars = m_deviceScalars;
    if (scalars.size() > 0x1B) {
        const char fbe = char(K500Frame::clampByte(qBound(0, m_mic.fbxLevel, 3)));
        scalars[0x1B] = fbe;
    }
"""
text = replace_once(text, old_fbe, new_fbe, "immutable FBE scalar seed")

helper_anchor = "void K500Controller::queueEqFrame(const QString &key, const QByteArray &frame, const QString &label)\n"
helpers = r'''void K500Controller::rejectUnsupported(const QString &path)
{
    m_canonicalState.discardDesired(path);
    emit canonicalStateChanged();
    emit unsupportedPath(path);
}

void K500Controller::deferWrite(const QString &path, const QString &reason)
{
    m_canonicalState.discardDesired(path);
    emit canonicalStateChanged();
    emit writeDeferred(path, reason);
}

void K500Controller::recordConfirmedState(const QByteArray &memory)
{
    using Evidence = K500CanonicalState::Evidence;
    const auto captured = [this](const QString &path, const QVariant &value) {
        m_canonicalState.confirm(path, value, Evidence::SnapshotCaptured);
    };
    const auto derived = [this](const QString &path, const QVariant &value) {
        m_canonicalState.confirm(path, value, Evidence::SnapshotDerived);
    };
    const auto assumed = [this](const QString &path, const QVariant &value) {
        m_canonicalState.confirm(path, value, Evidence::AssumedMetadata);
    };

    captured(QStringLiteral("system.topMusicVol"), m_music.topMusicVol);
    captured(QStringLiteral("system.topMicVol"), m_mic.topMicVol);
    captured(QStringLiteral("system.topEffectVol"), m_effect.topEffectVol);
    captured(QStringLiteral("system.effectInitLevel"), m_effect.effectInitLevel);
    captured(QStringLiteral("music.sourceRaw"), m_music.sourceRaw);
    derived(QStringLiteral("music.key"), m_music.key);
    derived(QStringLiteral("music.input1GainDb"), m_music.input1GainDb);
    derived(QStringLiteral("music.input2GainDb"), m_music.input2GainDb);
    derived(QStringLiteral("music.bluetoothGainDb"), m_music.bluetoothGainDb);
    derived(QStringLiteral("music.uDiskGainDb"), m_music.uDiskGainDb);
    derived(QStringLiteral("music.digitalGainDb"), m_music.digitalGainDb);

    captured(QStringLiteral("mic.micAVol"), m_mic.micAVol);
    captured(QStringLiteral("mic.micBVol"), m_mic.micBVol);
    captured(QStringLiteral("mic.fbxLevel"), m_mic.fbxLevel);
    derived(QStringLiteral("mic.compThresholdDb"), m_mic.compThresholdDb);
    captured(QStringLiteral("mic.compRatio"), m_mic.compRatio);
    captured(QStringLiteral("mic.attackMs"), m_mic.attackMs);
    derived(QStringLiteral("mic.releaseSec"), m_mic.releaseSec);
    captured(QStringLiteral("mic.eqLink"), fileU8(memory, 0x0092) == 1);

    captured(QStringLiteral("effects.reverb.level"), m_reverb.level);
    captured(QStringLiteral("effects.reverb.direct"), m_reverb.direct);
    captured(QStringLiteral("effects.reverb.hpfHz"), m_reverb.hpfHz);
    captured(QStringLiteral("effects.reverb.lpfHz"), m_reverb.lpfHz);
    captured(QStringLiteral("effects.reverb.decayMs"), m_reverb.decayMs);
    captured(QStringLiteral("effects.reverb.predelayMs"), m_reverb.predelayMs);
    captured(QStringLiteral("effects.echo.level"), m_echo.level);
    captured(QStringLiteral("effects.echo.repeat"), m_echo.repeat);
    captured(QStringLiteral("effects.echo.direct"), m_echo.direct);
    captured(QStringLiteral("effects.echo.rightDelayPercent"), m_echo.rightDelayPercent);
    captured(QStringLiteral("effects.echo.rightPredelayPercent"), m_echo.rightPredelayPercent);
    captured(QStringLiteral("effects.echo.hpfHz"), m_echo.hpfHz);
    captured(QStringLiteral("effects.echo.lpfHz"), m_echo.lpfHz);
    captured(QStringLiteral("effects.echo.leftDelayMs"), m_echo.leftDelayMs);
    captured(QStringLiteral("effects.echo.leftPredelayMs"), m_echo.leftPredelayMs);

    for (auto it = m_outputs.constBegin(); it != m_outputs.constEnd(); ++it) {
        const QString prefix = QStringLiteral("outputs.%1.").arg(it.key());
        const K500OutputBlockState &s = it.value();
        derived(prefix + QStringLiteral("lVolDb"), s.lVolDb);
        derived(prefix + QStringLiteral("rVolDb"), s.rVolDb);
        derived(prefix + QStringLiteral("outputVolDb"), s.outputVolDb);
        captured(prefix + QStringLiteral("micDirect"), s.micDirect);
        captured(prefix + QStringLiteral("musicLevel"), s.musicLevel);
        captured(prefix + QStringLiteral("reverbLevel"), s.reverbLevel);
        captured(prefix + QStringLiteral("echoLevel"), s.echoLevel);
        derived(prefix + QStringLiteral("compThresholdDb"), s.compThresholdDb);
        captured(prefix + QStringLiteral("compRatio"), s.compRatio);
        captured(prefix + QStringLiteral("attackMs"), s.attackMs);
        derived(prefix + QStringLiteral("releaseSec"), s.releaseSec);
        captured(prefix + QStringLiteral("lDelayMs"), s.lDelayMs);
        captured(prefix + QStringLiteral("rDelayMs"), s.rDelayMs);
    }

    for (auto it = m_crossovers.constBegin(); it != m_crossovers.constEnd(); ++it) {
        const QString prefix = QStringLiteral("eq.%1.crossover.").arg(it.key());
        captured(prefix + QStringLiteral("hpfHz"), it->hpfHz);
        captured(prefix + QStringLiteral("lpfHz"), it->lpfHz);
        assumed(prefix + QStringLiteral("hpType"), it->hpType);
        assumed(prefix + QStringLiteral("lpType"), it->lpType);
    }

    static const QStringList bypassSections{
        QStringLiteral("mic"), QStringLiteral("music"), QStringLiteral("main"),
        QStringLiteral("surround"), QStringLiteral("center"), QStringLiteral("sub"),
        QStringLiteral("reverb"), QStringLiteral("echo")};
    for (const QString &section : bypassSections) {
        captured(QStringLiteral("eq.%1.bypass").arg(section),
                 K500Protocol::eqBypassEnabled(m_eqBypass, section));
    }
}

void K500Controller::handleCommandDispatchResult(quint64 sessionEpoch, quint64 token,
                                                  const QString &path, bool accepted,
                                                  const QString &reason)
{
    if (!m_canonicalState.completeTransport(sessionEpoch, token, accepted))
        return;
    emit canonicalStateChanged();
    if (!accepted)
        deferWrite(path, reason.isEmpty() ? QStringLiteral("Native transport rejected command") : reason);
}

void K500Controller::queueEqFrame(const QString &key, const QString &path,
                                  const QByteArray &frame, const QString &label)
'''
text = replace_once(text, helper_anchor, helpers, "controller canonical helper insertion")

eq_body_pattern = re.compile(
    r"void K500Controller::queueEqFrame\(const QString &key, const QString &path,\n                                  const QByteArray &frame, const QString &label\)\n\{.*?\n\}\n\nvoid K500Controller::queueBlockFrame\(const QString &key, const QByteArray &frame, const QString &label\)",
    re.S,
)
eq_replacement = r'''void K500Controller::queueEqFrame(const QString &key, const QString &path,
                                  const QByteArray &frame, const QString &label)
{
    QString reason;
    const auto plan = m_canonicalState.plan(path, QStringLiteral("eq:%1").arg(key), frame, label, &reason);
    if (!plan) {
        deferWrite(path, reason);
        return;
    }
    m_pendingEqFrames.insert(key, PendingFrame{*plan});
    emit canonicalStateChanged();
    if (m_eqTimer.isActive())
        return;
    if (!m_lastEqFlush.isValid() || m_lastEqFlush.elapsed() >= EqSendIntervalMs) {
        flushEqFrames();
        return;
    }
    m_eqTimer.start(qMax(1, EqSendIntervalMs - static_cast<int>(m_lastEqFlush.elapsed())));
}

void K500Controller::queueBlockFrame(const QString &key, const QString &path,
                                     const QByteArray &frame, const QString &label)'''
text, n = eq_body_pattern.subn(eq_replacement, text, count=1)
if n != 1:
    raise SystemExit("P1 patch anchor missing: queueEqFrame body")

block_body_pattern = re.compile(
    r"void K500Controller::queueBlockFrame\(const QString &key, const QString &path,\n                                     const QByteArray &frame, const QString &label\)\n\{.*?\n\}\n\nvoid K500Controller::flushEqFrames",
    re.S,
)
block_replacement = r'''void K500Controller::queueBlockFrame(const QString &key, const QString &path,
                                     const QByteArray &frame, const QString &label)
{
    if (frame.isEmpty()) {
        rejectUnsupported(path);
        return;
    }
    QString reason;
    const auto plan = m_canonicalState.plan(path, key, frame, label, &reason);
    if (!plan) {
        deferWrite(path, reason);
        return;
    }
    m_pendingBlockFrames.insert(key, PendingFrame{*plan});
    emit canonicalStateChanged();
    if (m_blockTimer.isActive())
        return;
    if (!m_lastBlockFlush.isValid() || m_lastBlockFlush.elapsed() >= BlockSendIntervalMs) {
        flushBlockFrames();
        return;
    }
    m_blockTimer.start(qMax(1, BlockSendIntervalMs - static_cast<int>(m_lastBlockFlush.elapsed())));
}

void K500Controller::flushEqFrames'''
text, n = block_body_pattern.subn(block_replacement, text, count=1)
if n != 1:
    raise SystemExit("P1 patch anchor missing: queueBlockFrame body")

flush_eq_pattern = re.compile(r"void K500Controller::flushEqFrames\(\)\n\{.*?\n\}\n\nvoid K500Controller::flushBlockFrames", re.S)
flush_eq_replacement = r'''void K500Controller::flushEqFrames()
{
    m_eqTimer.stop();
    m_lastEqFlush.start();
    if (!m_liveEnabled) {
        m_pendingEqFrames.clear();
        return;
    }
    const auto frames = m_pendingEqFrames;
    m_pendingEqFrames.clear();
    for (const PendingFrame &pending : frames) {
        const auto &command = pending.command;
        if (!m_canonicalState.isCurrent(command) || !m_canonicalState.markDispatched(command.token))
            continue;
        emit canonicalStateChanged();
        emit commandReady(command.sessionEpoch, command.token, command.frame, command.label,
                          command.semanticPath, command.coalescingKey);
        emit frameReady(command.frame, command.label);
    }
}

void K500Controller::flushBlockFrames'''
text, n = flush_eq_pattern.subn(flush_eq_replacement, text, count=1)
if n != 1:
    raise SystemExit("P1 patch anchor missing: flushEqFrames")

flush_block_pattern = re.compile(r"void K500Controller::flushBlockFrames\(\)\n\{.*?\n\}\s*$", re.S)
flush_block_replacement = r'''void K500Controller::flushBlockFrames()
{
    m_blockTimer.stop();
    m_lastBlockFlush.start();
    if (!m_liveEnabled) {
        m_pendingBlockFrames.clear();
        return;
    }
    const auto frames = m_pendingBlockFrames;
    m_pendingBlockFrames.clear();
    for (const PendingFrame &pending : frames) {
        const auto &command = pending.command;
        if (!m_canonicalState.isCurrent(command) || !m_canonicalState.markDispatched(command.token))
            continue;
        emit canonicalStateChanged();
        emit commandReady(command.sessionEpoch, command.token, command.frame, command.label,
                          command.semanticPath, command.coalescingKey);
        emit frameReady(command.frame, command.label);
    }
}
'''
text, n = flush_block_pattern.subn(flush_block_replacement, text, count=1)
if n != 1:
    raise SystemExit("P1 patch anchor missing: flushBlockFrames")

p.write_text(text, encoding="utf-8")


# ---- DeviceManager: only transport session-bound command envelopes ----
p = Path("src/k500/K500DeviceManager.h")
text = p.read_text(encoding="utf-8")
text = replace_once(
    text,
    "public slots:\n    void sendLiveFrame(const QByteArray &frame, const QString &label);\n",
    """public slots:
    void sendLiveFrame(const QByteArray &frame, const QString &label);
    void sendPlannedCommand(quint64 sessionEpoch, quint64 token,
                            const QByteArray &frame, const QString &label,
                            const QString &path, const QString &coalescingKey);
""",
    "device manager planned command slot",
)
text = replace_once(
    text,
    "    void activeMemoryReady(const QByteArray &memory);\n    void logLine",
    """    void activeMemoryReady(const QByteArray &memory);
    void commandDispatchResult(quint64 sessionEpoch, quint64 token,
                               const QString &path, bool accepted,
                               const QString &reason);
    void logLine""",
    "device manager dispatch result signal",
)
p.write_text(text, encoding="utf-8")

p = Path("src/k500/K500DeviceManager.cpp")
text = p.read_text(encoding="utf-8")
text = replace_once(
    text,
    """        connect(m_controller, &K500Controller::frameReady,
                this, &K500DeviceManager::sendLiveFrame);
""",
    """        connect(m_controller, &K500Controller::commandReady,
                this, &K500DeviceManager::sendPlannedCommand);
        connect(this, &K500DeviceManager::commandDispatchResult,
                m_controller, &K500Controller::handleCommandDispatchResult);
""",
    "device manager command connection",
)
text = replace_once(
    text,
    "void K500DeviceManager::connectDevice()\n{\n    resetConnectionState(false);\n    setError({});\n",
    """void K500DeviceManager::connectDevice()
{
    resetConnectionState(false);
    if (m_controller)
        m_controller->beginDeviceSession();
    setError({});
""",
    "device manager session begin",
)
send_anchor = """void K500DeviceManager::sendLiveFrame(const QByteArray &frame, const QString &label)
{
    if (!connected() || !m_liveEnabled || frame.isEmpty())
        return;
    writeFrame(frame, label);
}
"""
send_new = send_anchor + r'''

void K500DeviceManager::sendPlannedCommand(quint64 sessionEpoch, quint64 token,
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
text = replace_once(text, send_anchor, send_new, "device manager planned send")
p.write_text(text, encoding="utf-8")


# ---- Mechanical contract assertions ----
controller = Path("src/k500/K500Controller.cpp").read_text(encoding="utf-8")
manager = Path("src/k500/K500DeviceManager.cpp").read_text(encoding="utf-8")
for token in (
    "P1_CANONICAL_SNAPSHOT_BARRIER_V1",
    "P1_CANONICAL_EDIT_GATE_V1",
    "recordConfirmedState(memory)",
    "emit commandReady(command.sessionEpoch",
    "m_canonicalState.plan(",
    "m_canonicalState.markDispatched(",
):
    if token not in controller:
        raise SystemExit(f"P1 integration token missing after patch: {token}")
for token in (
    "m_activeMemory[0x027D]",
    "m_deviceScalars[0x1B] = fbe",
    "connect(m_controller, &K500Controller::frameReady",
):
    if token in controller + manager:
        raise SystemExit(f"P1 forbidden legacy pattern remains: {token}")

print("P1 source integration patch completed")
