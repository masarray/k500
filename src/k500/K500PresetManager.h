#pragma once

#include "K500PresetProtocol.h"
#include "K500ResponseParser.h"

#include <QByteArray>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVector>

class K500DeviceManager;

class K500PresetManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool recallBusy READ recallBusy NOTIFY busyChanged)
    Q_PROPERTY(bool storeBusy READ storeBusy NOTIFY busyChanged)
    Q_PROPERTY(bool useInitBusy READ useInitBusy NOTIFY busyChanged)
    Q_PROPERTY(bool adjMannerBusy READ adjMannerBusy NOTIFY busyChanged)
    Q_PROPERTY(bool useInitVolume READ useInitVolume NOTIFY useInitVolumeChanged)
    Q_PROPERTY(bool useInitVolumeKnown READ useInitVolumeKnown NOTIFY useInitVolumeChanged)
    Q_PROPERTY(bool adjMannerVrOff READ adjMannerVrOff NOTIFY adjMannerVrOffChanged)
    Q_PROPERTY(bool adjMannerVrOffKnown READ adjMannerVrOffKnown NOTIFY adjMannerVrOffChanged)
    Q_PROPERTY(bool usbStoreAvailable READ usbStoreAvailable NOTIFY connectedChanged)
    Q_PROPERTY(int activeSlot READ activeSlot NOTIFY activeSlotChanged)
    Q_PROPERTY(int lastKnownSlot READ lastKnownSlot NOTIFY lastKnownSlotChanged)
    Q_PROPERTY(QString progress READ progress NOTIFY progressChanged)

public:
    explicit K500PresetManager(K500DeviceManager *manager, QObject *parent = nullptr);

    bool connected() const;
    bool busy() const { return m_operation != Operation::None; }
    bool recallBusy() const { return m_operation == Operation::Recall; }
    bool storeBusy() const { return m_operation == Operation::Save || m_operation == Operation::Rename || m_operation == Operation::Upload || m_operation == Operation::MassUpload; }
    bool useInitBusy() const { return m_operation == Operation::UseInit; }
    bool adjMannerBusy() const { return m_operation == Operation::AdjManner; }
    bool useInitVolume() const { return m_useInitVolume; }
    bool useInitVolumeKnown() const { return m_useInitVolumeKnown; }
    bool adjMannerVrOff() const { return m_adjMannerVrOff; }
    bool adjMannerVrOffKnown() const { return m_adjMannerVrOffKnown; }
    bool usbStoreAvailable() const;
    int activeSlot() const { return m_activeSlot; }
    int lastKnownSlot() const { return m_lastKnownSlot; }
    QString progress() const { return m_progress; }

    Q_INVOKABLE void recallMode(int slotOneBased);
    Q_INVOKABLE void setUseInitVolume(bool enabled);
    Q_INVOKABLE void setAdjMannerVrOff(bool enabled);
    Q_INVOKABLE void saveCurrentToSlot(int slotOneBased);
    Q_INVOKABLE void renameActiveMode(const QString &name);
    Q_INVOKABLE void setBtName(const QString &name);
    Q_INVOKABLE void resetBtName();

    // P4_PC_PRESET_UPLOAD_V1 — a validated 0x0290 image produced by the P3
    // codec can be stored directly without replacing it with fresh device
    // readback. After commit, Upload recalls that same slot and performs a full
    // 939-byte resync so the editor changes only after the K500 has changed.
    Q_INVOKABLE void uploadSlotImage(int slotOneBased, const QByteArray &image);

    // P2 mass-upload engine accepts pre-built 0x0290 slot images. P4 preset
    // library feeds this after every file passes the same P3 validation path.
    Q_INVOKABLE void massUploadSlotImages(const QVariantList &entries);

signals:
    void connectedChanged();
    void busyChanged();
    void useInitVolumeChanged();
    void adjMannerVrOffChanged();
    void activeSlotChanged();
    void lastKnownSlotChanged();
    void progressChanged();
    void activeMemoryReady(const QByteArray &memory);
    void operationCompleted(const QString &kind, int slotOneBased);
    void operationFailed(const QString &kind, const QString &message);

private:
    enum class Operation { None, Recall, UseInit, AdjManner, BtName, Save, Rename, Upload, MassUpload };
    enum class Step {
        Idle,
        RecallDelay,
        AwaitRecallHandshake,
        Readback,
        SingleStoreBeginDelay,
        AwaitMassBeginAck,
        AwaitChunkAck,
        AwaitCommitAck,
        AwaitUseInitAck,
        AwaitAdjMannerAck,
        AwaitBtNameAck,
    };
    enum class ReadbackPurpose { None, Recall, SavePrepare, RenamePrepare, BtIdentity };

    struct MassEntry {
        int slot = 1;
        QByteArray image;
    };

    bool beginOperation(Operation operation, QString *error = nullptr);
    void finishOperation(const QString &kind, int slotOneBased = 0);
    void failOperation(const QString &kind, const QString &message);
    void setProgress(const QString &progress);

    bool send(const QByteArray &frame, const QString &label);
    void armTimeout(int ms, const QString &kind, const QString &message);
    void clearTimeout();
    void onBytesReceived(const QByteArray &bytes);
    void onResponse(const K500Response &response);

    void hydrateAdjMannerFromMemory(const QByteArray &memory);

    void sendRecallHandshake();
    void startReadback(ReadbackPurpose purpose);
    void sendNextReadBlock();
    void acceptReadBlock(const QByteArray &data);
    void finishReadback();

    void beginStoreSlot(bool waitForBeginAck);
    void sendNextStoreChunk();
    void sendStoreCommit();
    void acceptStoreCommit();
    void startNextMassEntry();

    static QString operationName(Operation operation);

    K500DeviceManager *m_manager = nullptr;
    K500ResponseParser m_parser;
    QTimer m_timeout;

    Operation m_operation = Operation::None;
    Step m_step = Step::Idle;
    ReadbackPurpose m_readbackPurpose = ReadbackPurpose::None;
    bool m_useInitVolume = false;
    bool m_useInitVolumeKnown = false;
    bool m_previousUseInitVolume = false;
    bool m_previousUseInitVolumeKnown = false;
    bool m_adjMannerVrOff = false;
    bool m_adjMannerVrOffKnown = false;
    bool m_previousAdjMannerVrOff = false;
    bool m_previousAdjMannerVrOffKnown = false;
    int m_requestedSlot = 1;
    QString m_requestedModeName;
    int m_activeSlot = 0;
    int m_lastKnownSlot = 0;
    QString m_progress;

    QByteArray m_readbackMemory;
    int m_readOffset = 0;
    int m_pendingReadLength = 0;

    QByteArray m_storeImage;
    int m_storeOffset = 0;
    int m_pendingStoreLength = 0;
    QByteArray m_commitFrame;
    K500StoreChain m_storeChain;

    QVector<MassEntry> m_massEntries;
    int m_massIndex = -1;
};
