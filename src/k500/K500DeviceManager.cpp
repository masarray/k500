#include "K500DeviceManager.h"

#include "K500Controller.h"
#include "K500Frame.h"
#include "K500Protocol.h"

#include <QSettings>

namespace {
constexpr quint16 K500UsbVendorId = 0x10C4;
constexpr quint16 K500UsbProductId = 0x0321;
constexpr int HeartbeatIntervalMs = 3200;
constexpr int ProbeTimeoutMs = 1350;
constexpr int HandshakeTimeoutMs = 2200;
constexpr int ReadbackTimeoutMs = 2600;
constexpr int ConnectionWatchdogMs = 12000;
}

K500DeviceManager::K500DeviceManager(K500Controller *controller, QObject *parent)
    : QObject(parent), m_controller(controller), m_io(this)
{
    m_responseTimer.setSingleShot(true);
    m_probeDelayTimer.setSingleShot(true);
    m_heartbeatTimer.setInterval(HeartbeatIntervalMs);

    connect(&m_io, &K500WinIo::bytesReceived,
            this, &K500DeviceManager::onBytesReceived);
    connect(&m_io, &K500WinIo::errorOccurred,
            this, &K500DeviceManager::onIoError);
    connect(&m_responseTimer, &QTimer::timeout,
            this, &K500DeviceManager::connectionTimeout);
    connect(&m_probeDelayTimer, &QTimer::timeout,
            this, &K500DeviceManager::sendProbeHeartbeat);
    connect(&m_heartbeatTimer, &QTimer::timeout,
            this, &K500DeviceManager::heartbeatTick);

    if (m_controller) {
        connect(m_controller, &K500Controller::frameReady,
                this, &K500DeviceManager::sendLiveFrame);
        connect(this, &K500DeviceManager::deviceScalarsReady,
                m_controller, &K500Controller::setDeviceScalars);
    }

    QSettings settings;
    const QString savedMode = settings.value(QStringLiteral("k500/transportMode"),
                                              QStringLiteral("bt")).toString().toLower();
    if (savedMode == QStringLiteral("usb"))
        m_transportMode = savedMode;
    m_lastKnownSerialPort = settings.value(QStringLiteral("k500/lastBtPort")).toString().toUpper();
}

void K500DeviceManager::setTransportMode(const QString &mode)
{
    const QString normalized = mode.trimmed().toLower() == QStringLiteral("usb")
        ? QStringLiteral("usb") : QStringLiteral("bt");
    if (m_transportMode == normalized)
        return;

    if (m_status != QStringLiteral("disconnected"))
        disconnectDevice();

    m_transportMode = normalized;
    QSettings().setValue(QStringLiteral("k500/transportMode"), m_transportMode);
    emit transportModeChanged();
}

void K500DeviceManager::toggleConnection()
{
    if (connected() || m_status == QStringLiteral("connecting")
        || m_status == QStringLiteral("syncing")) {
        disconnectDevice();
        return;
    }
    connectDevice();
}

void K500DeviceManager::connectDevice()
{
    resetConnectionState(false);
    setError({});
    setStatus(QStringLiteral("connecting"));

    if (m_transportMode == QStringLiteral("usb"))
        beginUsbProbe();
    else
        beginBluetoothScan();
}

void K500DeviceManager::disconnectDevice()
{
    resetConnectionState(false);
    setError({});
    setPortLabel(QStringLiteral("No device"));
    setStatus(QStringLiteral("disconnected"));
}

void K500DeviceManager::sendPlayerCommand(const QString &command)
{
    if (!connected())
        return;
    writeFrame(K500Protocol::playerCommand(command),
               QStringLiteral("Player %1").arg(command));
}

void K500DeviceManager::toggleMute()
{
    if (!connected())
        return;
    const bool next = !m_muted;
    if (writeFrame(K500Protocol::mute(next), next ? QStringLiteral("Mute ON")
                                                  : QStringLiteral("Mute OFF"))) {
        m_muted = next;
        emit mutedChanged();
    }
}

void K500DeviceManager::sendLiveFrame(const QByteArray &frame, const QString &label)
{
    if (!connected() || !m_liveEnabled || frame.isEmpty())
        return;
    writeFrame(frame, label);
}

void K500DeviceManager::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void K500DeviceManager::setPortLabel(const QString &label)
{
    if (m_portLabel == label)
        return;
    m_portLabel = label;
    emit portLabelChanged();
}

void K500DeviceManager::setError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}

void K500DeviceManager::setLiveEnabled(bool enabled)
{
    if (m_liveEnabled == enabled)
        return;
    m_liveEnabled = enabled;
    if (m_controller)
        m_controller->setLiveEnabled(enabled);
    emit liveEnabledChanged();
}

void K500DeviceManager::beginBluetoothScan()
{
    m_serialCandidates = K500WinIo::serialPorts();
    if (m_serialCandidates.isEmpty()) {
        setError(QStringLiteral("Tidak ada COM port Windows yang tersedia untuk Bluetooth SPP."));
        setPortLabel(QStringLiteral("No BT COM port"));
        setStatus(QStringLiteral("error"));
        return;
    }

    // Native reconnect path: try the last protocol-verified K500 COM port first.
    if (!m_lastKnownSerialPort.isEmpty()) {
        const int index = m_serialCandidates.indexOf(m_lastKnownSerialPort);
        if (index > 0)
            m_serialCandidates.move(index, 0);
    }

    m_serialCandidateIndex = -1;
    openNextBluetoothCandidate();
}

void K500DeviceManager::openNextBluetoothCandidate()
{
    m_responseTimer.stop();
    m_probeDelayTimer.stop();
    m_io.close();
    m_parser.reset();

    ++m_serialCandidateIndex;
    if (m_serialCandidateIndex >= m_serialCandidates.size()) {
        setError(QStringLiteral("K500 Bluetooth tidak ditemukan. Pastikan KTV_BT sudah paired di Windows dan port SPP tidak sedang dipakai aplikasi lain."));
        setPortLabel(QStringLiteral("K500 BT not found"));
        setStatus(QStringLiteral("error"));
        m_stage = Stage::Idle;
        return;
    }

    m_currentSerialPort = m_serialCandidates.at(m_serialCandidateIndex);
    setPortLabel(QStringLiteral("Scanning %1 · %2/%3")
                     .arg(m_currentSerialPort)
                     .arg(m_serialCandidateIndex + 1)
                     .arg(m_serialCandidates.size()));

    QString error;
    if (!m_io.openSerial(m_currentSerialPort, &error)) {
        QTimer::singleShot(0, this, &K500DeviceManager::openNextBluetoothCandidate);
        return;
    }

    m_stage = Stage::ProbeBluetooth;
    m_probeAttempt = 0;
    // Bluetooth RFCOMM can need a short warm-up after the COM handle opens.
    m_probeDelayTimer.start(m_currentSerialPort == m_lastKnownSerialPort ? 500 : 250);
}

void K500DeviceManager::sendProbeHeartbeat()
{
    if (m_stage != Stage::ProbeBluetooth && m_stage != Stage::ProbeUsb)
        return;

    ++m_probeAttempt;
    if (!writeFrame(K500Protocol::heartbeat(),
                    QStringLiteral("Probe heartbeat %1").arg(m_probeAttempt))) {
        if (m_stage == Stage::ProbeBluetooth)
            openNextBluetoothCandidate();
        else {
            setStatus(QStringLiteral("error"));
            m_stage = Stage::Idle;
        }
        return;
    }
    m_responseTimer.start(ProbeTimeoutMs);
}

void K500DeviceManager::beginUsbProbe()
{
    QString label;
    QString error;
    if (!m_io.openUsbHid(K500UsbVendorId, K500UsbProductId, &label, &error)) {
        setError(error);
        setPortLabel(QStringLiteral("USB HID DSP AUDIO not found"));
        setStatus(QStringLiteral("error"));
        return;
    }

    setPortLabel(QStringLiteral("%1 · 10C4:0321").arg(label));
    m_stage = Stage::ProbeUsb;
    m_probeAttempt = 0;
    m_probeDelayTimer.start(60);
}

void K500DeviceManager::beginSync()
{
    m_responseTimer.stop();
    m_stage = Stage::AwaitHandshake;
    setStatus(QStringLiteral("syncing"));
    setPortLabel(QStringLiteral("%1 · handshake").arg(m_io.label()));
    if (!writeFrame(K500Protocol::handshake(), QStringLiteral("Handshake 0x3F")))
        return;
    m_responseTimer.start(HandshakeTimeoutMs);
}

void K500DeviceManager::requestScalarReadback()
{
    m_responseTimer.stop();
    m_stage = Stage::AwaitScalars;
    setPortLabel(QStringLiteral("%1 · reading device").arg(m_io.label()));
    if (!writeFrame(K500Protocol::readBlock(0x0000, 0x0040),
                    QStringLiteral("Read scalars 0x00..0x3F")))
        return;
    m_responseTimer.start(ReadbackTimeoutMs);
}

void K500DeviceManager::finishConnected(const QByteArray &scalars)
{
    if (scalars.size() < 0x40) {
        setError(QStringLiteral("K500 scalar readback terlalu pendek (%1 byte).").arg(scalars.size()));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return;
    }

    m_responseTimer.stop();
    m_stage = Stage::Ready;
    emit deviceScalarsReady(scalars.left(0x40));
    setLiveEnabled(true);
    setError({});
    setStatus(QStringLiteral("connected"));

    if (m_io.kind() == K500WinIo::Kind::Serial) {
        m_lastKnownSerialPort = m_currentSerialPort;
        QSettings().setValue(QStringLiteral("k500/lastBtPort"), m_lastKnownSerialPort);
        setPortLabel(QStringLiteral("%1 · Bluetooth SPP · LIVE").arg(m_currentSerialPort));
    } else {
        setPortLabel(QStringLiteral("%1 · USB HID · LIVE").arg(m_io.label()));
    }

    m_lastValidRx.start();
    m_heartbeatTimer.start();
}

void K500DeviceManager::connectionTimeout()
{
    if (m_stage == Stage::ProbeBluetooth) {
        const int maxAttempts = m_currentSerialPort == m_lastKnownSerialPort ? 4 : 3;
        if (m_probeAttempt < maxAttempts) {
            m_probeDelayTimer.start(220);
            return;
        }
        openNextBluetoothCandidate();
        return;
    }

    if (m_stage == Stage::ProbeUsb) {
        if (m_probeAttempt < 2) {
            m_probeDelayTimer.start(120);
            return;
        }
        setError(QStringLiteral("USB HID DSP AUDIO terbuka tetapi tidak menjawab heartbeat K500 0x1C."));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return;
    }

    if (m_stage == Stage::AwaitHandshake) {
        // Some firmware revisions do not surface the 0xC0 handshake response
        // reliably even though direct block read is valid. Use the same safe
        // scalar read as a fallback rather than enabling LIVE prematurely.
        requestScalarReadback();
        return;
    }

    if (m_stage == Stage::AwaitScalars) {
        setError(QStringLiteral("Timeout membaca scalar K500 0x00..0x3F. LIVE tetap OFF untuk mencegah overwrite state device."));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
    }
}

void K500DeviceManager::heartbeatTick()
{
    if (m_stage != Stage::Ready || !connected())
        return;

    if (m_lastValidRx.isValid() && m_lastValidRx.elapsed() > ConnectionWatchdogMs) {
        setError(QStringLiteral("K500 heartbeat timeout; koneksi dilepas agar live write berhenti."));
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return;
    }

    writeFrame(K500Protocol::heartbeat(), QStringLiteral("Heartbeat 0x1C"));
}

void K500DeviceManager::onBytesReceived(const QByteArray &bytes)
{
    const auto responses = m_parser.feed(bytes);
    for (const K500Response &response : responses) {
        m_lastRx = K500Frame::hex(response.raw);
        emit lastRxChanged();
        emit logLine(QStringLiteral("RX"),
                     QStringLiteral("RSP 0x%1%2")
                         .arg(response.rsp, 2, 16, QLatin1Char('0'))
                         .arg(response.checksumOk ? QString{} : QStringLiteral(" BAD-CS"))
                         .toUpper(),
                     m_lastRx);
        if (response.checksumOk) {
            m_lastValidRx.start();
            handleResponse(response);
        }
    }
}

void K500DeviceManager::handleResponse(const K500Response &response)
{
    if ((m_stage == Stage::ProbeBluetooth || m_stage == Stage::ProbeUsb)
        && response.rsp == 0xE3) {
        beginSync();
        return;
    }

    if (m_stage == Stage::AwaitHandshake && response.rsp == 0xC0) {
        requestScalarReadback();
        return;
    }

    if (m_stage == Stage::AwaitScalars && response.rsp == 0xBF) {
        finishConnected(response.data);
        return;
    }

    // During steady state heartbeat/status replies only refresh liveness.
    if (m_stage == Stage::Ready && response.rsp == 0xE3)
        return;
}

void K500DeviceManager::onIoError(const QString &message)
{
    if (m_stage == Stage::Idle)
        return;

    if (m_stage == Stage::ProbeBluetooth) {
        // A Windows COM entry can be incoming-only, stale or disappear while
        // scanning. Treat that as a rejected candidate, not a fatal K500 error.
        setError({});
        openNextBluetoothCandidate();
        return;
    }

    setError(message);
    resetConnectionState(true);
    setStatus(QStringLiteral("error"));
}

bool K500DeviceManager::writeFrame(const QByteArray &frame, const QString &label)
{
    if (frame.isEmpty() || !m_io.isOpen())
        return false;

    QString error;
    if (!m_io.writeProtocolFrame(frame, &error)) {
        setError(error);
        if (m_stage == Stage::ProbeBluetooth) {
            // Preserve probe stage so the caller can move to the next COM port.
            m_io.close();
            return false;
        }
        resetConnectionState(true);
        setStatus(QStringLiteral("error"));
        return false;
    }

    const QByteArray wireFrame = m_io.kind() == K500WinIo::Kind::UsbHid
        ? K500Frame::toUsbFrame(frame) : frame;
    m_lastTx = K500Frame::hex(wireFrame);
    emit lastTxChanged();
    emit logLine(QStringLiteral("TX"), label, m_lastTx);
    return true;
}

void K500DeviceManager::resetConnectionState(bool keepError)
{
    m_responseTimer.stop();
    m_probeDelayTimer.stop();
    m_heartbeatTimer.stop();
    m_stage = Stage::Idle;
    m_parser.reset();
    m_io.close();
    setLiveEnabled(false);
    if (m_controller)
        m_controller->clearDeviceState();
    m_muted = false;
    emit mutedChanged();
    if (!keepError)
        setError({});
}
