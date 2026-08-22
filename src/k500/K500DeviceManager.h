#pragma once

#include "K500ResponseParser.h"
#include "K500WinIo.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

class K500Controller;
class K500PresetManager;

class K500DeviceManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString transportMode READ transportMode WRITE setTransportMode NOTIFY transportModeChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString portLabel READ portLabel NOTIFY portLabelChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastRx READ lastRx NOTIFY lastRxChanged)
    Q_PROPERTY(QString lastTx READ lastTx NOTIFY lastTxChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY statusChanged)
    Q_PROPERTY(bool liveEnabled READ liveEnabled NOTIFY liveEnabledChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
    Q_PROPERTY(QObject *presetManager READ presetManager CONSTANT)
    Q_PROPERTY(QObject *presetFileBridge READ presetFileBridge CONSTANT)

public:
    explicit K500DeviceManager(K500Controller *controller, QObject *parent = nullptr);

    QString transportMode() const { return m_transportMode; }
    QString status() const { return m_status; }
    QString portLabel() const { return m_portLabel; }
    QString lastError() const { return m_lastError; }
    QString lastRx() const { return m_lastRx; }
    QString lastTx() const { return m_lastTx; }
    bool connected() const { return m_status == QStringLiteral("connected"); }
    bool liveEnabled() const { return m_liveEnabled; }
    bool muted() const { return m_muted; }
    QObject *presetManager() const;
    QObject *presetFileBridge() const;

    Q_INVOKABLE void setTransportMode(const QString &mode);
    Q_INVOKABLE void toggleConnection();
    Q_INVOKABLE void connectDevice();
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void sendPlayerCommand(const QString &command);
    Q_INVOKABLE void toggleMute();

public slots:
    void sendLiveFrame(const QByteArray &frame, const QString &label);

signals:
    void transportModeChanged();
    void statusChanged();
    void portLabelChanged();
    void lastErrorChanged();
    void lastRxChanged();
    void lastTxChanged();
    void liveEnabledChanged();
    void mutedChanged();
    void deviceScalarsReady(const QByteArray &scalars);
    void activeMemoryReady(const QByteArray &memory);
    void logLine(const QString &direction, const QString &label, const QString &hex);

private:
    friend class K500PresetManager;

    enum class Stage {
        Idle,
        ProbeBluetooth,
        ProbeUsb,
        AwaitHandshake,
        AwaitMemoryBlock,
        Ready,
    };

    void setStatus(const QString &status);
    void setPortLabel(const QString &label);
    void setError(const QString &message);
    void setLiveEnabled(bool enabled);

    void beginBluetoothScan();
    void openNextBluetoothCandidate();
    void sendProbeHeartbeat();
    void beginUsbProbe();
    void beginSync();
    void requestActiveMemoryReadback();
    void requestNextMemoryBlock();
    void acceptMemoryBlock(const QByteArray &data);
    void finishConnected();
    void connectionTimeout();
    void heartbeatTick();

    void onBytesReceived(const QByteArray &bytes);
    void handleResponse(const K500Response &response);
    void onIoError(const QString &message);
    bool writeFrame(const QByteArray &frame, const QString &label);
    void resetConnectionState(bool keepError = false);

    K500Controller *m_controller = nullptr;
    K500WinIo m_io;
    K500ResponseParser m_parser;
    QObject *m_presetManager = nullptr;
    QObject *m_presetFileBridge = nullptr;

    QString m_transportMode = QStringLiteral("bt");
    QString m_status = QStringLiteral("disconnected");
    QString m_portLabel = QStringLiteral("No device");
    QString m_lastError;
    QString m_lastRx;
    QString m_lastTx;
    bool m_liveEnabled = false;
    bool m_muted = false;

    Stage m_stage = Stage::Idle;
    QStringList m_serialCandidates;
    int m_serialCandidateIndex = -1;
    QString m_currentSerialPort;
    QString m_lastKnownSerialPort;
    int m_probeAttempt = 0;

    QByteArray m_activeMemory;
    int m_memoryReadOffset = 0;
    int m_pendingReadLength = 0;

    QTimer m_responseTimer;
    QTimer m_probeDelayTimer;
    QTimer m_heartbeatTimer;
    QElapsedTimer m_lastValidRx;
};
