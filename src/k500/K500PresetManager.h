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
    Q_PROPERTY(bool useInitVolume READ useInitVolume NOTIFY useInitVolumeChanged)
    Q_PROPERTY(bool usbStoreAvailable READ usbStoreAvailable NOTIFY connectedChanged)
    Q_PROPERTY(int activeSlot READ activeSlot NOTIFY activeSlotChanged)
    Q_PROPERTY(QString progress READ progress NOTIFY progressChanged)

public:
    explicit K500PresetManager(K500DeviceManager *manager, QObject *parent = nullptr);

    bool connected() const;
    bool busy() const { return m_operation != Operation::None; }
    bool recallBusy() const { return m_operation == Operation::Recall; }
    bool storeBusy() const { return m_operation == Operation::Save || m_operation == Operation::MassUpload; }
    bool useInitVolume() const { return m_useInitVolume; }
    bool usbStoreAvailable() const;
    int activeSlot() const { return m_activeSlot; }
    QString progress() const { return m_progress; }

    Q_INVOKABLE void recallMode(int slotOneBased);
    Q_INVOKABLE void setUseInitVolume(bool enabled);
    Q_INVOKABLE void saveCurrentToSlot(int slotOneBased);

    // P2 mass-upload engine accepts pre-built 0x0290 slot images. P3/P4 will
    // supply these from the bit-perfect .k500 codec / preset library.
    Q_INVOKABLE void massUploadSlotImages(const QVariantList &entries);

signals:
    void connectedChanged();
    void busyChanged();
    void useInitVolumeChanged();
    void activeSlotChanged();
    void progressChanged();
    void activeMemoryReady(const QByteArray &memory);
    void operationCompleted(const QString &kind, int slotOneBased);
    void operationFailed(const QString &kind, const QString &message);

private:
    enum class Operation { None, Recall, UseInit, Save, MassUpload };
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
    };
    enum class ReadbackPurpose { None, Recall, SavePrepare };

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
    bool m_previousUseInitVolume = false;
    int m_requestedSlot = 1;
    int m_activeSlot = 0;
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
