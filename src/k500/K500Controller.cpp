#include "K500Controller.h"

#include "K500Frame.h"

#include <QRegularExpression>
#include <QtMath>

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

void K500Controller::clearDeviceState()
{
    const bool wasReady = deviceReadbackReady();
    m_deviceScalars.clear();
    m_pendingEqFrames.clear();
    m_pendingBlockFrames.clear();
    m_eqTimer.stop();
    m_blockTimer.stop();
    if (wasReady)
        emit deviceReadbackReadyChanged();
}

void K500Controller::handleStateEdit(const QString &path, const QVariant &value)
{
    static const QRegularExpression musicBandPath(
        QStringLiteral(R"(^eq\.music\.bands\.(\d+)$)"));

    if (const auto match = musicBandPath.match(path); match.hasMatch()) {
        if (!m_liveEnabled)
            return;
        const QVariantMap map = value.toMap();
        K500EqBand band;
        band.frequencyHz = map.value(QStringLiteral("frequency"), 1000.0).toDouble();
        band.gainDb = map.value(QStringLiteral("gain"), 0.0).toDouble();
        band.q = map.value(QStringLiteral("q"), 1.0).toDouble();
        band.type = map.value(QStringLiteral("type"), QStringLiteral("BELL")).toString();
        const int index = match.captured(1).toInt();
        const QByteArray frame = K500Protocol::eqWrite(QStringLiteral("music"), index, band);
        if (!frame.isEmpty()) {
            queueEqFrame(QStringLiteral("music:%1").arg(index), frame,
                         QStringLiteral("Music EQ B%1 · %2Hz %3dB Q%4")
                             .arg(index + 1)
                             .arg(qRound(band.frequencyHz))
                             .arg(band.gainDb, 0, 'f', 1)
                             .arg(band.q, 0, 'f', 1));
        }
        return;
    }

    if (path == QStringLiteral("eq.music.crossover.hpfHz")) {
        m_musicHpfHz = value.toDouble();
        queueMusicCrossover(path, QStringLiteral("hpf"));
        return;
    }
    if (path == QStringLiteral("eq.music.crossover.lpfHz")) {
        m_musicLpfHz = value.toDouble();
        queueMusicCrossover(path, QStringLiteral("lpf"));
        return;
    }
    if (path == QStringLiteral("eq.music.crossover.hpType")) {
        m_musicHpType = value.toString();
        queueMusicCrossover(path, QStringLiteral("hpf"));
        return;
    }
    if (path == QStringLiteral("eq.music.crossover.lpType")) {
        m_musicLpType = value.toString();
        queueMusicCrossover(path, QStringLiteral("lpf"));
        return;
    }

    bool isTopMusicPath = true;
    if (path == QStringLiteral("system.topMusicVol"))
        m_music.topMusicVol = qRound(value.toDouble());
    else if (path == QStringLiteral("music.key"))
        m_music.key = value.toInt();
    else if (path == QStringLiteral("music.input1GainDb"))
        m_music.input1GainDb = value.toDouble();
    else if (path == QStringLiteral("music.input2GainDb"))
        m_music.input2GainDb = value.toDouble();
    else if (path == QStringLiteral("music.bluetoothGainDb") || path == QStringLiteral("music.btGainDb"))
        m_music.bluetoothGainDb = value.toDouble();
    else if (path == QStringLiteral("music.uDiskGainDb"))
        m_music.uDiskGainDb = value.toDouble();
    else if (path == QStringLiteral("music.digitalGainDb"))
        m_music.digitalGainDb = value.toDouble();
    else
        isTopMusicPath = false;

    if (isTopMusicPath) {
        queueTopMusic(path);
        return;
    }

    // These controls exist in the new QML but their native live command is not
    // yet sufficiently verified in the donor repository. Keep them editable in
    // the model, but never guess a hardware write.
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
    queueBlockFrame(QStringLiteral("top:music"),
                    K500Protocol::topMusicBlock(m_music, m_deviceScalars),
                    QStringLiteral("Top Music · %1").arg(path));
}

void K500Controller::queueMusicCrossover(const QString &path, const QString &kind)
{
    if (!m_liveEnabled)
        return;
    if (!deviceReadbackReady()) {
        emit writeDeferred(path, QStringLiteral("Music crossover requires scalar 0x1B readback"));
        return;
    }

    const bool hpf = kind == QStringLiteral("hpf");
    const double frequency = hpf ? m_musicHpfHz : m_musicLpfHz;
    const QString filter = hpf ? m_musicHpType : m_musicLpType;
    const quint8 stateByte = static_cast<quint8>(static_cast<unsigned char>(m_deviceScalars.at(0x1B)));
    const QByteArray frame = K500Protocol::crossoverWrite(
        QStringLiteral("music"), kind, frequency, filter, stateByte);
    if (!frame.isEmpty()) {
        queueBlockFrame(QStringLiteral("xover:music:%1").arg(kind), frame,
                        QStringLiteral("Music %1 · %2Hz · %3")
                            .arg(kind.toUpper())
                            .arg(qRound(frequency))
                            .arg(filter));
    }
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
