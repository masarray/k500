#include "K500Controller.h"

#include "K500Frame.h"

#include <QRegularExpression>
#include <QtMath>

namespace {
quint8 byteAt(const QByteArray &bytes, int offset, quint8 fallback = 0)
{
    if (offset < 0 || offset >= bytes.size())
        return fallback;
    return static_cast<quint8>(static_cast<unsigned char>(bytes.at(offset)));
}

int liveOffsetForFileScalar(int fileOffset)
{
    if (fileOffset >= 0x0008 && fileOffset <= 0x0096)
        return fileOffset - 0x08;
    if (fileOffset >= 0x0098 && fileOffset <= 0x00EF)
        return fileOffset - 0x09;
    return -1;
}

quint8 fileU8(const QByteArray &memory, int fileOffset, quint8 fallback = 0)
{
    return byteAt(memory, liveOffsetForFileScalar(fileOffset), fallback);
}

quint16 fileU16(const QByteArray &memory, int fileOffset, quint16 fallback = 0)
{
    const int offset = liveOffsetForFileScalar(fileOffset);
    if (offset < 0 || offset + 1 >= memory.size())
        return fallback;
    return static_cast<quint16>(byteAt(memory, offset)
        | (static_cast<quint16>(byteAt(memory, offset + 1)) << 8));
}

double outputDb(quint8 raw)
{
    return (static_cast<int>(raw) - 75) / 2.0;
}

QByteArray outputSeed(const QByteArray &memory, int fileBase)
{
    QByteArray out;
    out.reserve(K500Protocol::OutputDataLength);
    for (int i = 0; i < K500Protocol::OutputDataLength; ++i)
        out.append(char(fileU8(memory, fileBase + i, 0)));
    return out;
}

QByteArray reverbSeed(const QByteArray &memory)
{
    // REVERB_CMD0B_DEVICE_SEED_V1
    // Native CMD 0x0B is a 15-byte image. The first six bytes align with the
    // contiguous Reverb scalar block file[0x0074..0x0079], while file[0x007A]
    // is the final preserved neighbour before Echo begins at file[0x007B].
    QByteArray data(K500Protocol::ReverbDataLength, char(0));
    for (int i = 0; i < 6; ++i)
        data[i] = char(fileU8(memory, 0x0074 + i, 0));

    const auto putU16 = [&data](int offset, quint16 value) {
        data[offset] = char(value & 0xFF);
        data[offset + 1] = char((value >> 8) & 0xFF);
    };
    putU16(6, fileU16(memory, 0x00C0, 220));
    putU16(8, fileU16(memory, 0x00C2, 15800));
    putU16(10, fileU16(memory, 0x00C8, 1680));
    putU16(12, fileU16(memory, 0x00CA, 42));
    data[14] = char(fileU8(memory, 0x007A, 0));
    return data;
}

QByteArray echoSeed(const QByteArray &memory)
{
    // ECHO_CMD0D_DEVICE_SEED_V1 — native CMD 0x0D is a 22-byte full image.
    // Preserve all unproven neighbours from device readback instead of inventing
    // values. The verified scalar/timing fields line up with the preset map.
    QByteArray data(K500Protocol::EchoDataLength, char(0));
    for (int i = 0; i < 9; ++i)
        data[i] = char(fileU8(memory, 0x007A + i, 0));

    const auto putU16 = [&data](int offset, quint16 value) {
        data[offset] = char(value & 0xFF);
        data[offset + 1] = char((value >> 8) & 0xFF);
    };
    putU16(9, fileU16(memory, 0x00C4, 550));
    putU16(11, fileU16(memory, 0x00C6, 4200));
    putU16(13, fileU16(memory, 0x00CC, 300));
    putU16(15, fileU16(memory, 0x00CE, 100));
    putU16(17, fileU16(memory, 0x00D0, 0));
    data[19] = char(fileU8(memory, 0x00D2, 0));
    data[20] = char(fileU8(memory, 0x00D3, 0));
    data[21] = char(fileU8(memory, 0x00D4, 0));
    return data;
}

QString canonicalCrossoverSection(const QString &section)
{
    if (section == QStringLiteral("micA") || section == QStringLiteral("micB"))
        return QStringLiteral("mic");
    return section;
}
}

K500Controller::K500Controller(QObject *parent)
    : QObject(parent)
{
    m_eqTimer.setSingleShot(true);
    m_blockTimer.setSingleShot(true);
    connect(&m_eqTimer, &QTimer::timeout, this, &K500Controller::flushEqFrames);
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
{
    if (m_liveEnabled == enabled)
        return;
    m_liveEnabled = enabled;
    if (!enabled) {
        m_eqTimer.stop();
        m_blockTimer.stop();
        m_pendingEqFrames.clear();
        m_pendingBlockFrames.clear();
    }
    emit liveEnabledChanged();
}

void K500Controller::setDeviceScalars(const QByteArray &scalars)
{
    const bool wasReady = deviceReadbackReady();
    m_deviceScalars = scalars.left(0x40);
    if (wasReady != deviceReadbackReady())
        emit deviceReadbackReadyChanged();
}

void K500Controller::hydrateFromDeviceMemory(const QByteArray &memory)
{
    applyDeviceMemory(memory, false);
}

void K500Controller::reconcileFromDeviceMemory(const QByteArray &memory)
{
    applyDeviceMemory(memory, true);
}

void K500Controller::applyDeviceMemory(const QByteArray &memory, bool preserveDesiredIntent)
{
    // P1_CANONICAL_SNAPSHOT_BARRIER_V1 + P3_AUTHORITATIVE_RECONCILIATION_V1
    // Initial/Recall hydration resets intent; verification hydration preserves
    // unresolved DesiredState until semantic confirmation rebuilds hardware truth.
    if (memory.size() != K500CanonicalState::ActiveMemorySize)
        return;
    if (!m_canonicalState.sessionActive())
        m_canonicalState.beginSession(); // deterministic harness/recovery fallback
    QString snapshotError;
    const bool accepted = preserveDesiredIntent
        ? m_canonicalState.reconcileSnapshot(memory, &snapshotError)
        : m_canonicalState.adoptSnapshot(memory, &snapshotError);
    if (!accepted) {
        emit writeDeferred(preserveDesiredIntent ? QStringLiteral("__reconcile__")
                                                 : QStringLiteral("__snapshot__"),
                           snapshotError);
        return;
    }
    emit canonicalStateChanged();

    // Seed every complete-block write from the immutable device snapshot while
    // LIVE is still OFF. Unknown/reserved neighbours remain device-owned.
    setDeviceScalars(memory.left(0x40));

    m_music.topMusicVol = fileU8(memory, 0x0008, 35);
    m_music.musicInitVol = fileU8(memory, 0x000B, 25);
    m_music.sourceRaw = fileU8(memory, 0x000E, 2);
    m_music.key = static_cast<int>(fileU8(memory, 0x0011, 7)) - 7;
    m_music.input1GainDb = static_cast<int>(fileU8(memory, 0x001E, 9)) - 12;
    m_music.input2GainDb = static_cast<int>(fileU8(memory, 0x001F, 9)) - 12;
    m_music.bluetoothGainDb = static_cast<int>(fileU8(memory, 0x0020, 9)) - 12;
    m_music.uDiskGainDb = static_cast<int>(fileU8(memory, 0x0021, 8)) - 12;
    m_music.digitalGainDb = static_cast<int>(fileU8(memory, 0x0022, 8)) - 12;
    // Music Noise Gate readback offset is not yet capture-proven. Preserve the
    // device scalar for unrelated Top-Music writes until this session explicitly
    // edits the gate; do not pretend 0x001B is authoritative gate truth.
    m_music.noiseGateRaw = -1;

    m_mic.topMicVol = fileU8(memory, 0x0009, 35);
    m_mic.micInitVol = fileU8(memory, 0x0012, 25);
    m_mic.micAVol = fileU8(memory, 0x0014, 96);
    m_mic.micBVol = fileU8(memory, 0x0015, 96);
    // FBE_NATIVE_LEVEL_V1 — native delta capture proves FBE is file 0x001B only.
    // File 0x001C is an independent neighbour and must never be averaged into FBE.
    m_mic.fbxLevel = fileU8(memory, 0x001B, 0);
    m_mic.compThresholdDb = static_cast<int>(fileU8(memory, 0x0017, 38)) - 50;
    m_mic.compRatio = fileU8(memory, 0x0018, 3);
    m_mic.attackMs = fileU8(memory, 0x0019, 10);
    m_mic.releaseSec = fileU8(memory, 0x001A, 2) / 10.0;

    m_effect.topEffectVol = fileU8(memory, 0x000A, 35);
    m_effect.effectInitLevel = fileU8(memory, 0x001D, 25);

    // REVERB_CMD0B_CAPTURED_V1 — state and raw image are hydrated before LIVE.
    m_reverb.level = fileU8(memory, 0x0074, 100);
    m_reverb.direct = fileU8(memory, 0x0076, 100);
    m_reverb.hpfHz = fileU16(memory, 0x00C0, 220);
    m_reverb.lpfHz = fileU16(memory, 0x00C2, 15800);
    m_reverb.decayMs = fileU16(memory, 0x00C8, 1680);
    m_reverb.predelayMs = fileU16(memory, 0x00CA, 42);
    m_reverbRaw = reverbSeed(memory);

    // ECHO_CMD0D_CAPTURED_V1 — all supplied native Echo delta captures resolve
    // to fields in one full CMD 0x0D image.
    m_echo.level = fileU8(memory, 0x007B, 100);
    m_echo.repeat = fileU8(memory, 0x007C, 2);
    m_echo.direct = fileU8(memory, 0x0080, 100);
    m_echo.rightDelayPercent = static_cast<int>(fileU8(memory, 0x0081, 50)) - 50;
    m_echo.rightPredelayPercent = static_cast<int>(fileU8(memory, 0x0082, 50)) - 50;
    m_echo.hpfHz = fileU16(memory, 0x00C4, 550);
    m_echo.lpfHz = fileU16(memory, 0x00C6, 4200);
    m_echo.leftDelayMs = fileU16(memory, 0x00CC, 300);
    m_echo.leftPredelayMs = fileU16(memory, 0x00CE, 100);
    m_echoRaw = echoSeed(memory);

    // EQ_BYPASS_24BIT_CAPTURED_V2 — Retrieve All contains the authoritative
    // shared bypass image at active-memory offsets 0x027D..0x027F. Hydrate it
    // before LIVE is enabled so the first toggle can always be a safe RMW.
    m_eqBypass = K500EqBypassImage{byteAt(memory, 0x027D), byteAt(memory, 0x027E), byteAt(memory, 0x027F)};
    m_eqBypassReady = memory.size() > 0x027F;

    K500OutputBlockState main;
    main.lVolDb = outputDb(fileU8(memory, 0x0024, 75));
    main.rVolDb = outputDb(fileU8(memory, 0x0026, 75));
    main.micDirect = fileU8(memory, 0x0028, 0);
    main.musicLevel = fileU8(memory, 0x002A, 0);
    main.reverbLevel = fileU8(memory, 0x002C, 0);
    main.echoLevel = fileU8(memory, 0x002E, 0);
    main.compThresholdDb = static_cast<int>(fileU8(memory, 0x0030, 30)) - 50;
    main.compRatio = fileU8(memory, 0x0031, 1);
    main.attackMs = fileU8(memory, 0x0032, 10);
    main.releaseSec = fileU8(memory, 0x0033, 1) / 10.0;
    m_outputs.insert(QStringLiteral("main"), main);
    m_outputRaw.insert(QStringLiteral("main"), outputSeed(memory, 0x0024));

    K500OutputBlockState surround;
    surround.lVolDb = outputDb(fileU8(memory, 0x0038, 75));
    surround.rVolDb = outputDb(fileU8(memory, 0x003A, 75));
    surround.micDirect = fileU8(memory, 0x003C, 0);
    surround.musicLevel = fileU8(memory, 0x003E, 0);
    surround.reverbLevel = fileU8(memory, 0x0040, 0);
    surround.echoLevel = fileU8(memory, 0x0042, 0);
    surround.compThresholdDb = static_cast<int>(fileU8(memory, 0x0044, 30)) - 50;
    surround.compRatio = fileU8(memory, 0x0045, 1);
    surround.attackMs = fileU8(memory, 0x0046, 10);
    surround.releaseSec = fileU8(memory, 0x0047, 1) / 10.0;
    surround.lDelayMs = fileU16(memory, 0x00D8, 0);
    surround.rDelayMs = fileU16(memory, 0x00DA, 0);
    m_outputs.insert(QStringLiteral("surround"), surround);
    m_outputRaw.insert(QStringLiteral("surround"), outputSeed(memory, 0x0038));

    K500OutputBlockState center;
    center.outputVolDb = outputDb(fileU8(memory, 0x004C, 75));
    center.micDirect = fileU8(memory, 0x0050, 0);
    center.musicLevel = fileU8(memory, 0x0052, 0);
    center.reverbLevel = fileU8(memory, 0x0054, 0);
    center.echoLevel = fileU8(memory, 0x0056, 0);
    center.compThresholdDb = static_cast<int>(fileU8(memory, 0x0058, 30)) - 50;
    center.compRatio = fileU8(memory, 0x0059, 1);
    center.attackMs = fileU8(memory, 0x005A, 10);
    center.releaseSec = fileU8(memory, 0x005B, 1) / 10.0;
    m_outputs.insert(QStringLiteral("center"), center);
    m_outputRaw.insert(QStringLiteral("center"), outputSeed(memory, 0x004C));

    K500OutputBlockState sub;
    sub.outputVolDb = outputDb(fileU8(memory, 0x0060, 75));
    sub.micDirect = fileU8(memory, 0x0064, 0);
    sub.musicLevel = fileU8(memory, 0x0066, 0);
    sub.reverbLevel = fileU8(memory, 0x0068, 0);
    sub.echoLevel = fileU8(memory, 0x006A, 0);
    sub.compThresholdDb = static_cast<int>(fileU8(memory, 0x006C, 30)) - 50;
    sub.compRatio = fileU8(memory, 0x006D, 1);
    sub.attackMs = fileU8(memory, 0x006E, 10);
    sub.releaseSec = fileU8(memory, 0x006F, 1) / 10.0;
    m_outputs.insert(QStringLiteral("sub"), sub);
    m_outputRaw.insert(QStringLiteral("sub"), outputSeed(memory, 0x0060));

    const auto seedCrossover = [this, &memory](const QString &key, int hpfOffset, int lpfOffset,
                                               const QString &hpType, const QString &lpType) {
        CrossoverState state;
        state.hpfHz = fileU16(memory, hpfOffset, 20);
        state.lpfHz = fileU16(memory, lpfOffset, 20000);
        state.hpType = hpType;
        state.lpType = lpType;
        m_crossovers.insert(key, state);
    };
    seedCrossover(QStringLiteral("mic"), 0x0098, 0x009A, QStringLiteral("HP LR 24"), QStringLiteral("LP LR 24"));
    seedCrossover(QStringLiteral("music"), 0x009C, 0x009E, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    seedCrossover(QStringLiteral("main"), 0x00A0, 0x00A4, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    seedCrossover(QStringLiteral("surround"), 0x00A8, 0x00AC, QStringLiteral("HP Bessel 12"), QStringLiteral("LP Bessel 12"));
    seedCrossover(QStringLiteral("center"), 0x00B0, 0x00B4, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    seedCrossover(QStringLiteral("sub"), 0x00B8, 0x00BC, QStringLiteral("HP Butter 24"), QStringLiteral("LP Butter 24"));
    seedCrossover(QStringLiteral("reverb"), 0x00C0, 0x00C2, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    seedCrossover(QStringLiteral("echo"), 0x00C4, 0x00C6, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));

    recordConfirmedState(memory);
    emit canonicalStateChanged();
}

void K500Controller::clearDeviceState()
{
    const bool wasReady = deviceReadbackReady();
    m_canonicalState.endSession();
    m_deviceScalars.clear();
    m_reverb = K500ReverbBlockState{};
    m_reverbRaw.clear();
    m_echo = K500EchoBlockState{};
    m_echoRaw.clear();
    m_eqBypass = K500EqBypassImage{};
    m_eqBypassReady = false;
    m_outputs.clear();
    m_outputRaw.clear();
    m_crossovers.clear();
    m_pendingEqFrames.clear();
    m_pendingBlockFrames.clear();
    m_eqTimer.stop();
    m_blockTimer.stop();
    if (wasReady)
        emit deviceReadbackReadyChanged();
    emit canonicalStateChanged();
}

void K500Controller::handleStateEdit(const QString &path, const QVariant &value)
{
    // P1_CANONICAL_EDIT_GATE_V1 — DesiredState is session-bound and can only
    // exist after the exact Retrieve-All snapshot has become authoritative.
    if (!m_liveEnabled)
        return;
    QString canonicalReason;
    if (!m_canonicalState.stageDesired(path, value, nullptr, &canonicalReason)) {
        deferWrite(path, canonicalReason);
        return;
    }
    emit canonicalStateChanged();

    // P1_FULL_LIVE_ROUTING_V1
    static const QRegularExpression eqBandPath(QStringLiteral(R"(^eq\.([^.]+)\.bands\.(\d+)$)"));
    static const QRegularExpression crossoverPath(QStringLiteral(R"(^eq\.([^.]+)\.crossover\.(hpfHz|lpfHz|hpType|lpType)$)"));
    static const QRegularExpression reverbPath(QStringLiteral(R"(^effects\.reverb\.(level|direct|hpfHz|lpfHz|decayMs|predelayMs)$)"));
    static const QRegularExpression echoPath(QStringLiteral(R"(^effects\.echo\.(level|repeat|direct|rightDelayPercent|rightPredelayPercent|hpfHz|lpfHz|leftDelayMs|leftPredelayMs)$)"));
    static const QRegularExpression outputPath(QStringLiteral(R"(^outputs\.(main|surround|center|sub)\.([^.]+)$)"));

    if (const auto match = eqBandPath.match(path); match.hasMatch()) {
        if (!m_liveEnabled)
            return;
        const QString section = match.captured(1);
        const QVariantMap map = value.toMap();
        K500EqBand band;
        band.frequencyHz = map.value(QStringLiteral("frequency"), 1000.0).toDouble();
        band.gainDb = map.value(QStringLiteral("gain"), 0.0).toDouble();
        band.q = map.value(QStringLiteral("q"), 1.0).toDouble();
        band.type = map.value(QStringLiteral("type"), QStringLiteral("BELL")).toString();
        const int index = match.captured(2).toInt();
        const QByteArray frame = K500Protocol::eqWrite(section, index, band);
        if (!frame.isEmpty()) {
            queueEqFrame(QStringLiteral("%1:%2").arg(section).arg(index), path, frame,
                         QStringLiteral("%1 EQ B%2 · %3Hz %4dB Q%5")
                             .arg(section).arg(index + 1).arg(qRound(band.frequencyHz))
                             .arg(band.gainDb, 0, 'f', 1).arg(band.q, 0, 'f', 1));
        } else {
            rejectUnsupported(path);
        }
        return;
    }

    if (const auto match = crossoverPath.match(path); match.hasMatch()) {
        const QString section = canonicalCrossoverSection(match.captured(1));
        const QString field = match.captured(2);
        if (!m_crossovers.contains(section)) {
            rejectUnsupported(path);
            return;
        }

        // Native Reverb/Echo cutoff deltas use complete effect images, not the
        // generic CMD 0x11 selector family. Filter type remains evidence-gated.
        if (section == QStringLiteral("reverb") || section == QStringLiteral("echo")) {
            if (field == QStringLiteral("hpfHz") || field == QStringLiteral("lpfHz")) {
                const bool hpf = field == QStringLiteral("hpfHz");
                if (hpf) m_crossovers[section].hpfHz = value.toDouble();
                else m_crossovers[section].lpfHz = value.toDouble();
                if (section == QStringLiteral("reverb")) {
                    if (updateReverbState(field, value))
                        queueReverb(path);
                } else {
                    if (updateEchoState(field, value))
                        queueEcho(path);
                }
            } else {
                rejectUnsupported(path);
            }
            return;
        }

        CrossoverState &state = m_crossovers[section];
        const QString kind = (field == QStringLiteral("hpfHz") || field == QStringLiteral("hpType"))
            ? QStringLiteral("hpf") : QStringLiteral("lpf");
        if (field == QStringLiteral("hpfHz")) state.hpfHz = value.toDouble();
        else if (field == QStringLiteral("lpfHz")) state.lpfHz = value.toDouble();
        else if (field == QStringLiteral("hpType")) state.hpType = value.toString();
        else state.lpType = value.toString();
        queueCrossover(section, path, kind);
        return;
    }

    if (path == QStringLiteral("mic.hpfHz") || path == QStringLiteral("mic.lpfHz")) {
        CrossoverState &state = m_crossovers[QStringLiteral("mic")];
        const bool hpf = path.endsWith(QStringLiteral("hpfHz"));
        if (hpf) state.hpfHz = value.toDouble(); else state.lpfHz = value.toDouble();
        queueCrossover(QStringLiteral("mic"), path, hpf ? QStringLiteral("hpf") : QStringLiteral("lpf"));
        return;
    }
    if (path == QStringLiteral("outputs.sub.hpfHz") || path == QStringLiteral("outputs.sub.lpfHz")) {
        CrossoverState &state = m_crossovers[QStringLiteral("sub")];
        const bool hpf = path.endsWith(QStringLiteral("hpfHz"));
        if (hpf) state.hpfHz = value.toDouble(); else state.lpfHz = value.toDouble();
        queueCrossover(QStringLiteral("sub"), path, hpf ? QStringLiteral("hpf") : QStringLiteral("lpf"));
        return;
    }
    if (path == QStringLiteral("effects.echo.hpfHz") || path == QStringLiteral("effects.echo.lpfHz")) {
        const QString field = path.endsWith(QStringLiteral("hpfHz")) ? QStringLiteral("hpfHz") : QStringLiteral("lpfHz");
        if (updateEchoState(field, value))
            queueEcho(path);
        return;
    }

    if (const auto match = reverbPath.match(path); match.hasMatch()) {
        if (updateReverbState(match.captured(1), value)) {
            queueReverb(path);
            return;
        }
    }
    if (const auto match = echoPath.match(path); match.hasMatch()) {
        if (updateEchoState(match.captured(1), value)) {
            queueEcho(path);
            return;
        }
    }

    // EQ_BYPASS_24BIT_CAPTURED_V2 — every PEQ bypass button edits one bit-group
    // inside the same device-owned 24-bit image. Never synthesize a mask from UI
    // defaults; only mutate the Retrieve-All snapshot and preserve all other bits.
    static const QRegularExpression eqBypassPath(QStringLiteral(R"(^eq\.(mic|music|main|surround|center|sub|reverb|echo)\.bypass$)"));
    if (const auto match = eqBypassPath.match(path); match.hasMatch()) {
        if (!m_liveEnabled || !m_eqBypassReady) {
            deferWrite(path, QStringLiteral("EQ bypass write requires complete device Retrieve All hydration"));
            return;
        }
        const QString section = match.captured(1);
        if (!K500Protocol::setEqBypass(m_eqBypass, section, value.toBool())) {
            rejectUnsupported(path);
            return;
        }
        queueEqBypass(path);
        return;
    }

    bool isTopMusicPath = true;
    if (path == QStringLiteral("system.topMusicVol")) m_music.topMusicVol = qRound(value.toDouble());
    else if (path == QStringLiteral("music.sourceRaw")) m_music.sourceRaw = qBound(0, value.toInt(), 5);
    else if (path == QStringLiteral("music.key")) m_music.key = value.toInt();
    else if (path == QStringLiteral("music.input1GainDb")) m_music.input1GainDb = value.toDouble();
    else if (path == QStringLiteral("music.input2GainDb")) m_music.input2GainDb = value.toDouble();
    else if (path == QStringLiteral("music.bluetoothGainDb") || path == QStringLiteral("music.btGainDb")) m_music.bluetoothGainDb = value.toDouble();
    else if (path == QStringLiteral("music.uDiskGainDb")) m_music.uDiskGainDb = value.toDouble();
    else if (path == QStringLiteral("music.digitalGainDb")) m_music.digitalGainDb = value.toDouble();
    else if (path == QStringLiteral("music.noiseGateDb")) {
        m_music.noiseGateRaw = K500Protocol::musicNoiseGateRaw(value.toDouble());
    }
    else isTopMusicPath = false;
    if (isTopMusicPath) { queueTopMusic(path); return; }

    if (path == QStringLiteral("music.bassDb")) {
        queueBlockFrame(QStringLiteral("music:bass"), path, K500Protocol::musicBass(value.toDouble()),
                        QStringLiteral("Music Bass %1 dB").arg(value.toDouble(), 0, 'f', 1));
        return;
    }

    bool isTopMicPath = true;
    if (path == QStringLiteral("system.topMicVol")) m_mic.topMicVol = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.micAVol")) m_mic.micAVol = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.micBVol")) m_mic.micBVol = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.fbxLevel")) m_mic.fbxLevel = qBound(0, qRound(value.toDouble()), 3);
    else if (path == QStringLiteral("mic.compThresholdDb")) m_mic.compThresholdDb = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.compRatio")) m_mic.compRatio = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.attackMs")) m_mic.attackMs = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.releaseSec")) m_mic.releaseSec = value.toDouble();
    else isTopMicPath = false;
    if (isTopMicPath) { queueTopMic(path); return; }

    if (path == QStringLiteral("system.topEffectVol")) {
        m_effect.topEffectVol = qRound(value.toDouble());
        queueTopEffect(path);
        return;
    }
    if (path == QStringLiteral("system.effectInitLevel")) {
        m_effect.effectInitLevel = qRound(value.toDouble());
        queueTopEffect(path);
        return;
    }

    if (path == QStringLiteral("mic.eqLink")) {
        if (!m_liveEnabled)
            return;
        queueBlockFrame(QStringLiteral("mic:eqLink"), path, K500Protocol::micEqLink(value.toBool()),
                        QStringLiteral("Mic EQ Link %1").arg(value.toBool() ? QStringLiteral("ON") : QStringLiteral("OFF")));
        return;
    }

    if (const auto match = outputPath.match(path); match.hasMatch()) {
        const QString section = match.captured(1);
        const QString field = match.captured(2);
        if (updateOutputState(section, field, value)) {
            queueOutput(section, path);
            return;
        }
    }

    // Controls without a byte-verified native write remain non-destructive.
    rejectUnsupported(path);
}

void K500Controller::queueTopMusic(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!deviceReadbackReady()) {
        deferWrite(path, QStringLiteral("Top Music write requires device scalar readback 0x00..0x3F"));
        return;
    }
    queueBlockFrame(QStringLiteral("top:music"), path, K500Protocol::topMusicBlock(m_music, m_deviceScalars),
                    QStringLiteral("Top Music · %1").arg(path));
}

void K500Controller::queueTopMic(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!deviceReadbackReady()) {
        deferWrite(path, QStringLiteral("Top Mic write requires device scalar readback 0x00..0x3F"));
        return;
    }

    // FBE_NATIVE_LEVEL_V1 — native 3→2→1→0 capture changes only CMD 0x05
    // payload byte mapped to live scalar 0x1B; neighbour 0x1C stays untouched.
    QByteArray scalars = m_deviceScalars;
    if (scalars.size() > 0x1B) {
        const char fbe = char(K500Frame::clampByte(qBound(0, m_mic.fbxLevel, 3)));
        scalars[0x1B] = fbe;
    }

    queueBlockFrame(QStringLiteral("top:mic"), path, K500Protocol::topMicBlock(m_mic, scalars),
                    QStringLiteral("Top Mic · %1").arg(path));
}

void K500Controller::queueTopEffect(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!deviceReadbackReady()) {
        deferWrite(path, QStringLiteral("Top Effect write requires device scalar readback 0x00..0x3F"));
        return;
    }
    queueBlockFrame(QStringLiteral("top:effect"), path, K500Protocol::topEffectBlock(m_effect, m_deviceScalars),
                    QStringLiteral("Top Effect · %1").arg(path));
}

void K500Controller::queueReverb(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (m_reverbRaw.size() < K500Protocol::ReverbDataLength) {
        deferWrite(path, QStringLiteral("Reverb CMD 0x0B requires full device readback seed"));
        return;
    }
    queueBlockFrame(QStringLiteral("fx:reverb"), path, K500Protocol::reverbBlock(m_reverb, m_reverbRaw),
                    QStringLiteral("Reverb · %1").arg(path));
}

void K500Controller::queueEcho(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (m_echoRaw.size() < K500Protocol::EchoDataLength) {
        deferWrite(path, QStringLiteral("Echo CMD 0x0D requires full device readback seed"));
        return;
    }
    queueBlockFrame(QStringLiteral("fx:echo"), path, K500Protocol::echoBlock(m_echo, m_echoRaw),
                    QStringLiteral("Echo · %1").arg(path));
}

void K500Controller::queueEqBypass(const QString &path)
{
    if (!m_liveEnabled || !m_eqBypassReady)
        return;
    queueBlockFrame(QStringLiteral("eq:bypass-global"), path, K500Protocol::eqBypassWrite(m_eqBypass),
                    QStringLiteral("EQ Bypass · %1 · %2 %3 %4")
                        .arg(path)
                        .arg(m_eqBypass.m0, 2, 16, QLatin1Char('0'))
                        .arg(m_eqBypass.m1, 2, 16, QLatin1Char('0'))
                        .arg(m_eqBypass.m2, 2, 16, QLatin1Char('0')));
}

void K500Controller::queueOutput(const QString &section, const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!m_outputs.contains(section) || !m_outputRaw.contains(section)
        || m_outputRaw.value(section).size() < K500Protocol::OutputDataLength) {
        deferWrite(path, QStringLiteral("Output block requires full device readback seed"));
        return;
    }
    const QByteArray frame = K500Protocol::outputBlock(section, m_outputs.value(section), m_outputRaw.value(section));
    if (frame.isEmpty()) {
        rejectUnsupported(path);
        return;
    }
    queueBlockFrame(QStringLiteral("output:%1").arg(section), path, frame,
                    QStringLiteral("Output %1 · %2").arg(section, path));
}

void K500Controller::queueCrossover(const QString &section, const QString &path, const QString &kind)
{
    if (!m_liveEnabled)
        return;
    if (section == QStringLiteral("reverb") || section == QStringLiteral("echo")) {
        rejectUnsupported(path);
        return;
    }
    if (!m_crossovers.contains(section)) {
        rejectUnsupported(path);
        return;
    }
    if (section == QStringLiteral("music") && !deviceReadbackReady()) {
        deferWrite(path, QStringLiteral("Music crossover requires scalar 0x1B readback"));
        return;
    }

    const CrossoverState state = m_crossovers.value(section);
    const bool hpf = kind == QStringLiteral("hpf");
    const double frequency = hpf ? state.hpfHz : state.lpfHz;
    const QString filter = hpf ? state.hpType : state.lpType;
    const quint8 stateByte = section == QStringLiteral("music") && m_deviceScalars.size() > 0x1B
        ? byteAt(m_deviceScalars, 0x1B, 0x32) : 0x00;
    const QByteArray frame = K500Protocol::crossoverWrite(section, kind, frequency, filter, stateByte);
    if (frame.isEmpty()) {
        rejectUnsupported(path);
        return;
    }
    queueBlockFrame(QStringLiteral("xover:%1:%2").arg(section, kind), path, frame,
                    QStringLiteral("%1 %2 · %3Hz · %4").arg(section, kind.toUpper()).arg(qRound(frequency)).arg(filter));
}

bool K500Controller::updateReverbState(const QString &field, const QVariant &value)
{
    if (field == QStringLiteral("level")) m_reverb.level = qBound(K500Protocol::NativeRange::ReverbLevelMin, qRound(value.toDouble()), K500Protocol::NativeRange::ReverbLevelMax);
    else if (field == QStringLiteral("direct")) m_reverb.direct = qBound(K500Protocol::NativeRange::ReverbDirectMin, qRound(value.toDouble()), K500Protocol::NativeRange::ReverbDirectMax);
    else if (field == QStringLiteral("hpfHz")) {
        m_reverb.hpfHz = qBound(K500Protocol::NativeRange::FxHpfMinHz, qRound(value.toDouble()), K500Protocol::NativeRange::FxHpfMaxHz);
        m_crossovers[QStringLiteral("reverb")].hpfHz = m_reverb.hpfHz;
    }
    else if (field == QStringLiteral("lpfHz")) {
        m_reverb.lpfHz = qBound(K500Protocol::NativeRange::FxLpfMinHz, qRound(value.toDouble()), K500Protocol::NativeRange::FxLpfMaxHz);
        m_crossovers[QStringLiteral("reverb")].lpfHz = m_reverb.lpfHz;
    }
    else if (field == QStringLiteral("decayMs")) m_reverb.decayMs = qBound(K500Protocol::NativeRange::ReverbDecayMinMs, qRound(value.toDouble()), K500Protocol::NativeRange::ReverbDecayMaxMs);
    else if (field == QStringLiteral("predelayMs")) m_reverb.predelayMs = qBound(K500Protocol::NativeRange::ReverbPredelayMinMs, qRound(value.toDouble()), K500Protocol::NativeRange::ReverbPredelayMaxMs);
    else return false;
    return true;
}

bool K500Controller::updateEchoState(const QString &field, const QVariant &value)
{
    if (field == QStringLiteral("level")) m_echo.level = qBound(K500Protocol::NativeRange::EchoLevelMin, qRound(value.toDouble()), K500Protocol::NativeRange::EchoLevelMax);
    else if (field == QStringLiteral("repeat")) m_echo.repeat = qBound(K500Protocol::NativeRange::EchoRepeatMin, qRound(value.toDouble()), K500Protocol::NativeRange::EchoRepeatMax);
    else if (field == QStringLiteral("direct")) m_echo.direct = qBound(K500Protocol::NativeRange::EchoDirectMin, qRound(value.toDouble()), K500Protocol::NativeRange::EchoDirectMax);
    else if (field == QStringLiteral("rightDelayPercent")) m_echo.rightDelayPercent = qBound(-50, qRound(value.toDouble()), 50);
    else if (field == QStringLiteral("rightPredelayPercent")) m_echo.rightPredelayPercent = qBound(-50, qRound(value.toDouble()), 50);
    else if (field == QStringLiteral("hpfHz")) {
        m_echo.hpfHz = qBound(K500Protocol::NativeRange::FxHpfMinHz, qRound(value.toDouble()), K500Protocol::NativeRange::FxHpfMaxHz);
        m_crossovers[QStringLiteral("echo")].hpfHz = m_echo.hpfHz;
    }
    else if (field == QStringLiteral("lpfHz")) {
        m_echo.lpfHz = qBound(K500Protocol::NativeRange::FxLpfMinHz, qRound(value.toDouble()), K500Protocol::NativeRange::FxLpfMaxHz);
        m_crossovers[QStringLiteral("echo")].lpfHz = m_echo.lpfHz;
    }
    else if (field == QStringLiteral("leftDelayMs")) m_echo.leftDelayMs = qBound(K500Protocol::NativeRange::EchoDelayMinMs, qRound(value.toDouble()), K500Protocol::NativeRange::EchoDelayMaxMs);
    else if (field == QStringLiteral("leftPredelayMs")) m_echo.leftPredelayMs = qBound(0, qRound(value.toDouble()), 65535);
    else return false;
    return true;
}

bool K500Controller::updateOutputState(const QString &section, const QString &field, const QVariant &value)
{
    if (!m_outputs.contains(section))
        return false;
    K500OutputBlockState &state = m_outputs[section];
    if (field == QStringLiteral("lVolDb")) state.lVolDb = value.toDouble();
    else if (field == QStringLiteral("rVolDb")) state.rVolDb = value.toDouble();
    else if (field == QStringLiteral("outputVolDb")) state.outputVolDb = value.toDouble();
    else if (field == QStringLiteral("micDirect")) state.micDirect = qRound(value.toDouble());
    else if (field == QStringLiteral("musicLevel")) state.musicLevel = qRound(value.toDouble());
    else if (field == QStringLiteral("reverbLevel")) state.reverbLevel = qRound(value.toDouble());
    else if (field == QStringLiteral("echoLevel")) state.echoLevel = qRound(value.toDouble());
    else if (field == QStringLiteral("compThresholdDb")) state.compThresholdDb = qRound(value.toDouble());
    else if (field == QStringLiteral("compRatio")) state.compRatio = qRound(value.toDouble());
    else if (field == QStringLiteral("attackMs")) state.attackMs = qRound(value.toDouble());
    else if (field == QStringLiteral("releaseSec")) state.releaseSec = value.toDouble();
    else if (field == QStringLiteral("lDelayMs")) state.lDelayMs = qRound(value.toDouble());
    else if (field == QStringLiteral("rDelayMs")) state.rDelayMs = qRound(value.toDouble());
    else return false;
    return true;
}

void K500Controller::rejectUnsupported(const QString &path)
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

bool K500Controller::markCommandDispatched(quint64 sessionEpoch, quint64 token)
{
    // P2_ACTUAL_DISPATCH_BARRIER_V1 — Queued remains Queued while the
    // transaction waits inside the deterministic scheduler. Only the exact
    // release to the asynchronous transport worker advances it to Dispatched.
    if (sessionEpoch != m_canonicalState.sessionEpoch())
        return false;
    if (!m_canonicalState.markDispatched(token))
        return false;
    emit canonicalStateChanged();
    return true;
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

void K500Controller::flushEqFrames()
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
        if (!m_canonicalState.isCurrent(command))
            continue;
        emit commandReady(command.sessionEpoch, command.token, command.frame, command.label,
                          command.semanticPath, command.coalescingKey);
        emit frameReady(command.frame, command.label);
    }
}

void K500Controller::flushBlockFrames()
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
        if (!m_canonicalState.isCurrent(command))
            continue;
        emit commandReady(command.sessionEpoch, command.token, command.frame, command.label,
                          command.semanticPath, command.coalescingKey);
        emit frameReady(command.frame, command.label);
    }
}
