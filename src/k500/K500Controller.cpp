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
    if (memory.size() < 0x40)
        return;

    // P1_CONTROLLER_DEVICE_SEED_V1
    // Seed every complete-block write from device truth while LIVE is still OFF.
    // No P1 edit may invent neighbouring/reserved values.
    m_activeMemory = memory;
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

    m_mic.topMicVol = fileU8(memory, 0x0009, 35);
    m_mic.micInitVol = fileU8(memory, 0x0012, 25);
    m_mic.micAVol = fileU8(memory, 0x0014, 96);
    m_mic.micBVol = fileU8(memory, 0x0015, 96);
    m_mic.fbxLevel = qRound((fileU8(memory, 0x001B, 7) + fileU8(memory, 0x001C, 7)) / 2.0);
    m_mic.compThresholdDb = static_cast<int>(fileU8(memory, 0x0017, 38)) - 50;
    m_mic.compRatio = fileU8(memory, 0x0018, 3);
    m_mic.attackMs = fileU8(memory, 0x0019, 10);
    m_mic.releaseSec = fileU8(memory, 0x001A, 2) / 10.0;

    m_effect.topEffectVol = fileU8(memory, 0x000A, 35);
    m_effect.effectInitLevel = fileU8(memory, 0x001D, 25);

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
}

void K500Controller::clearDeviceState()
{
    const bool wasReady = deviceReadbackReady();
    m_deviceScalars.clear();
    m_activeMemory.clear();
    m_outputs.clear();
    m_outputRaw.clear();
    m_crossovers.clear();
    m_pendingEqFrames.clear();
    m_pendingBlockFrames.clear();
    m_eqTimer.stop();
    m_blockTimer.stop();
    if (wasReady)
        emit deviceReadbackReadyChanged();
}

void K500Controller::handleStateEdit(const QString &path, const QVariant &value)
{
    // P1_FULL_LIVE_ROUTING_V1
    static const QRegularExpression eqBandPath(QStringLiteral(R"(^eq\.([^.]+)\.bands\.(\d+)$)"));
    static const QRegularExpression crossoverPath(QStringLiteral(R"(^eq\.([^.]+)\.crossover\.(hpfHz|lpfHz|hpType|lpType)$)"));
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
            queueEqFrame(QStringLiteral("%1:%2").arg(section).arg(index), frame,
                         QStringLiteral("%1 EQ B%2 · %3Hz %4dB Q%5")
                             .arg(section).arg(index + 1).arg(qRound(band.frequencyHz))
                             .arg(band.gainDb, 0, 'f', 1).arg(band.q, 0, 'f', 1));
        } else {
            emit unsupportedPath(path);
        }
        return;
    }

    if (const auto match = crossoverPath.match(path); match.hasMatch()) {
        const QString section = canonicalCrossoverSection(match.captured(1));
        const QString field = match.captured(2);
        if (!m_crossovers.contains(section)) {
            emit unsupportedPath(path);
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
    if (path == QStringLiteral("effects.reverb.hpfHz") || path == QStringLiteral("effects.reverb.lpfHz")
        || path == QStringLiteral("effects.echo.hpfHz") || path == QStringLiteral("effects.echo.lpfHz")) {
        const QString section = path.startsWith(QStringLiteral("effects.reverb")) ? QStringLiteral("reverb") : QStringLiteral("echo");
        CrossoverState &state = m_crossovers[section];
        const bool hpf = path.endsWith(QStringLiteral("hpfHz"));
        if (hpf) state.hpfHz = value.toDouble(); else state.lpfHz = value.toDouble();
        queueCrossover(section, path, hpf ? QStringLiteral("hpf") : QStringLiteral("lpf"));
        return;
    }

    bool isTopMusicPath = true;
    if (path == QStringLiteral("system.topMusicVol")) m_music.topMusicVol = qRound(value.toDouble());
    else if (path == QStringLiteral("music.key")) m_music.key = value.toInt();
    else if (path == QStringLiteral("music.input1GainDb")) m_music.input1GainDb = value.toDouble();
    else if (path == QStringLiteral("music.input2GainDb")) m_music.input2GainDb = value.toDouble();
    else if (path == QStringLiteral("music.bluetoothGainDb") || path == QStringLiteral("music.btGainDb")) m_music.bluetoothGainDb = value.toDouble();
    else if (path == QStringLiteral("music.uDiskGainDb")) m_music.uDiskGainDb = value.toDouble();
    else if (path == QStringLiteral("music.digitalGainDb")) m_music.digitalGainDb = value.toDouble();
    else isTopMusicPath = false;
    if (isTopMusicPath) { queueTopMusic(path); return; }

    bool isTopMicPath = true;
    if (path == QStringLiteral("system.topMicVol")) m_mic.topMicVol = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.micAVol")) m_mic.micAVol = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.micBVol")) m_mic.micBVol = qRound(value.toDouble());
    else if (path == QStringLiteral("mic.fbxLevel")) m_mic.fbxLevel = qRound(value.toDouble());
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
        queueBlockFrame(QStringLiteral("mic:eqLink"), K500Protocol::micEqLink(value.toBool()),
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
    emit unsupportedPath(path);
}

void K500Controller::queueTopMusic(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!deviceReadbackReady()) {
        emit writeDeferred(path, QStringLiteral("Top Music write requires device scalar readback 0x00..0x3F"));
        return;
    }
    queueBlockFrame(QStringLiteral("top:music"), K500Protocol::topMusicBlock(m_music, m_deviceScalars),
                    QStringLiteral("Top Music · %1").arg(path));
}

void K500Controller::queueTopMic(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!deviceReadbackReady()) {
        emit writeDeferred(path, QStringLiteral("Top Mic write requires device scalar readback 0x00..0x3F"));
        return;
    }
    queueBlockFrame(QStringLiteral("top:mic"), K500Protocol::topMicBlock(m_mic, m_deviceScalars),
                    QStringLiteral("Top Mic · %1").arg(path));
}

void K500Controller::queueTopEffect(const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!deviceReadbackReady()) {
        emit writeDeferred(path, QStringLiteral("Top Effect write requires device scalar readback 0x00..0x3F"));
        return;
    }
    queueBlockFrame(QStringLiteral("top:effect"), K500Protocol::topEffectBlock(m_effect, m_deviceScalars),
                    QStringLiteral("Top Effect · %1").arg(path));
}

void K500Controller::queueOutput(const QString &section, const QString &path)
{
    if (!m_liveEnabled)
        return;
    if (!m_outputs.contains(section) || !m_outputRaw.contains(section)
        || m_outputRaw.value(section).size() < K500Protocol::OutputDataLength) {
        emit writeDeferred(path, QStringLiteral("Output block requires full device readback seed"));
        return;
    }
    const QByteArray frame = K500Protocol::outputBlock(section, m_outputs.value(section), m_outputRaw.value(section));
    if (frame.isEmpty()) {
        emit unsupportedPath(path);
        return;
    }
    queueBlockFrame(QStringLiteral("output:%1").arg(section), frame,
                    QStringLiteral("Output %1 · %2").arg(section, path));
}

void K500Controller::queueCrossover(const QString &section, const QString &path, const QString &kind)
{
    if (!m_liveEnabled)
        return;
    if (!m_crossovers.contains(section)) {
        emit unsupportedPath(path);
        return;
    }
    if (section == QStringLiteral("music") && !deviceReadbackReady()) {
        emit writeDeferred(path, QStringLiteral("Music crossover requires scalar 0x1B readback"));
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
        emit unsupportedPath(path);
        return;
    }
    queueBlockFrame(QStringLiteral("xover:%1:%2").arg(section, kind), frame,
                    QStringLiteral("%1 %2 · %3Hz · %4").arg(section, kind.toUpper()).arg(qRound(frequency)).arg(filter));
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

void K500Controller::queueEqFrame(const QString &key, const QByteArray &frame, const QString &label)
{
    m_pendingEqFrames.insert(key, PendingFrame{frame, label});
    if (m_eqTimer.isActive())
        return;
    if (!m_lastEqFlush.isValid() || m_lastEqFlush.elapsed() >= EqSendIntervalMs) {
        flushEqFrames();
        return;
    }
    m_eqTimer.start(qMax(1, EqSendIntervalMs - static_cast<int>(m_lastEqFlush.elapsed())));
}

void K500Controller::queueBlockFrame(const QString &key, const QByteArray &frame, const QString &label)
{
    if (frame.isEmpty())
        return;
    m_pendingBlockFrames.insert(key, PendingFrame{frame, label});
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
    for (const PendingFrame &pending : frames)
        emit frameReady(pending.frame, pending.label);
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
    for (const PendingFrame &pending : frames)
        emit frameReady(pending.frame, pending.label);
}
