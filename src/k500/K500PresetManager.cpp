#include "K500PresetManager.h"

#include "K500DeviceManager.h"
#include "K500Protocol.h"

#include <QSet>
#include <QVariantMap>
#include <algorithm>

namespace {
constexpr int ActiveMemorySize = 0x03AB;
constexpr int ActiveMemoryBlockSize = 0x003A;
constexpr int ActiveMemoryInterBlockMs = 35;
constexpr int ReadbackTimeoutMs = 2600;
constexpr int RecallHandshakeTimeoutMs = 3000;
constexpr int UseInitTimeoutMs = 2200;
constexpr int StoreAckTimeoutMs = 3500;
constexpr int RecallSettleMs = 80;
constexpr int SingleStoreBeginSettleMs = 80;
}

K500PresetManager::K500PresetManager(K500DeviceManager *manager, QObject *parent)
    : QObject(parent), m_manager(manager)
{
    m_timeout.setSingleShot(true);

    if (!m_manager)
        return;

    // P2_TRANSACTION_COORDINATOR_V1
    // The coordinator observes the same native RX stream as DeviceManager with
    // an independent parser. Connection-stage responses remain owned by
    // DeviceManager; P2 responses are consumed only while its stage is Ready.
    connect(&m_manager->m_io, &K500WinIo::bytesReceived,
            this, &K500PresetManager::onBytesReceived);
    connect(m_manager, &K500DeviceManager::statusChanged, this, [this] {
        emit connectedChanged();
        if (!connected() && busy()) {
            clearTimeout();
            m_operation = Operation::None;
            m_step = Step::Idle;
            m_readbackPurpose = ReadbackPurpose::None;
            emit busyChanged();
        }
    });
    connect(m_manager, &K500DeviceManager::transportModeChanged,
            this, &K500PresetManager::connectedChanged);
}

bool K500PresetManager::connected() const
{
    return m_manager && m_manager->connected();
}

bool K500PresetManager::usbStoreAvailable() const
{
    return connected() && m_manager->transportMode() == QStringLiteral("usb");
}

QString K500PresetManager::operationName(Operation operation)
{
    switch (operation) {
    case Operation::Recall: return QStringLiteral("Recall");
    case Operation::UseInit: return QStringLiteral("Use Init Volume");
    case Operation::Save: return QStringLiteral("Save");
    case Operation::MassUpload: return QStringLiteral("Mass Upload");
    case Operation::None: break;
    }
    return QStringLiteral("Preset operation");
}

bool K500PresetManager::beginOperation(Operation operation, QString *error)
{
    if (!m_manager || !m_manager->connected() || m_manager->m_stage != K500DeviceManager::Stage::Ready) {
        if (error) *error = QStringLiteral("K500 belum berada pada state Ready/LIVE.");
        return false;
    }
    if (busy()) {
        if (error) *error = QStringLiteral("K500 sedang menjalankan operasi preset lain.");
        return false;
    }

    m_parser.reset();
    m_operation = operation;
    m_step = Step::Idle;
    m_readbackPurpose = ReadbackPurpose::None;
    m_manager->m_heartbeatTimer.stop();
    m_manager->setLiveEnabled(false); // also clears pending Controller live frames
    m_manager->setError({});
    emit busyChanged();
    return true;
}

void K500PresetManager::finishOperation(const QString &kind, int slotOneBased)
{
    clearTimeout();
    m_step = Step::Idle;
    m_readbackPurpose = ReadbackPurpose::None;
    m_operation = Operation::None;
    m_pendingReadLength = 0;
    m_pendingStoreLength = 0;
    emit busyChanged();

    if (m_manager && m_manager->connected() && m_manager->m_stage == K500DeviceManager::Stage::Ready) {
        m_manager->setError({});
        m_manager->setLiveEnabled(true);
        m_manager->m_lastValidRx.start();
        m_manager->m_heartbeatTimer.start();
    }
    emit operationCompleted(kind, slotOneBased);
}

void K500PresetManager::failOperation(const QString &kind, const QString &message)
{
    clearTimeout();
    const Operation failedOperation = m_operation;
    m_step = Step::Idle;
    m_readbackPurpose = ReadbackPurpose::None;
    m_operation = Operation::None;
    m_pendingReadLength = 0;
    m_pendingStoreLength = 0;

    if (failedOperation == Operation::UseInit && m_useInitVolume != m_previousUseInitVolume) {
        m_useInitVolume = m_previousUseInitVolume;
        emit useInitVolumeChanged();
    }
    setProgress(QStringLiteral("%1 failed").arg(kind));
    emit busyChanged();
    emit operationFailed(kind, message);

    // Recall/store failure leaves device state uncertain. Fail closed: drop the
    // native transport and keep LIVE OFF until a fresh full reconnect/readback.
    if (m_manager) {
        m_manager->setError(message);
        m_manager->resetConnectionState(true);
        m_manager->setStatus(QStringLiteral("error"));
    }
}

void K500PresetManager::setProgress(const QString &progress)
{
    if (m_progress == progress)
        return;
    m_progress = progress;
    emit progressChanged();
}

bool K500PresetManager::send(const QByteArray &frame, const QString &label)
{
    if (!m_manager || frame.isEmpty())
        return false;
    if (!m_manager->writeFrame(frame, label)) {
        failOperation(operationName(m_operation),
                      QStringLiteral("Gagal mengirim %1.").arg(label));
        return false;
    }
    return true;
}

void K500PresetManager::armTimeout(int ms, const QString &kind, const QString &message)
{
    m_timeout.stop();
    QObject::disconnect(&m_timeout, nullptr, this, nullptr);
    connect(&m_timeout, &QTimer::timeout, this, [this, kind, message] {
        failOperation(kind, message);
    });
    m_timeout.start(ms);
}

void K500PresetManager::clearTimeout()
{
    m_timeout.stop();
    QObject::disconnect(&m_timeout, nullptr, this, nullptr);
}

void K500PresetManager::recallMode(int slotOneBased)
{
    QString error;
    if (!beginOperation(Operation::Recall, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Recall"), error);
        return;
    }

    m_requestedSlot = qBound(1, slotOneBased, 10);
    setProgress(QStringLiteral("Recall slot %1 · switching device mode").arg(m_requestedSlot));
    m_step = Step::RecallDelay;
    if (!send(K500PresetProtocol::recallMode(m_requestedSlot),
              QStringLiteral("Recall slot %1 · mask 0x03").arg(m_requestedSlot)))
        return;

    QTimer::singleShot(RecallSettleMs, this, [this] {
        if (m_operation == Operation::Recall && m_step == Step::RecallDelay)
            sendRecallHandshake();
    });
}

void K500PresetManager::sendRecallHandshake()
{
    m_step = Step::AwaitRecallHandshake;
    setProgress(QStringLiteral("Recall slot %1 · refresh handshake").arg(m_requestedSlot));
    if (!send(K500PresetProtocol::recallHandshake(), QStringLiteral("Recall refresh handshake · mask 0x03")))
        return;
    armTimeout(RecallHandshakeTimeoutMs, QStringLiteral("Recall"),
               QStringLiteral("Timeout menunggu RSP 0xC0 setelah Recall."));
}

void K500PresetManager::setUseInitVolume(bool enabled)
{
    if (enabled == m_useInitVolume && !busy())
        return;

    QString error;
    if (!beginOperation(Operation::UseInit, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Use Init Volume"), error);
        return;
    }

    m_previousUseInitVolume = m_useInitVolume;
    m_useInitVolume = enabled;
    emit useInitVolumeChanged();
    m_step = Step::AwaitUseInitAck;
    setProgress(QStringLiteral("Use init volume %1").arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF")));
    if (!send(K500PresetProtocol::useInitVolume(enabled),
              QStringLiteral("Use init vol %1 · mask 0x03").arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF"))))
        return;
    armTimeout(UseInitTimeoutMs, QStringLiteral("Use Init Volume"),
               QStringLiteral("Timeout menunggu RSP 0xED untuk Use Init Volume."));
}

void K500PresetManager::saveCurrentToSlot(int slotOneBased)
{
    if (!usbStoreAvailable()) {
        const QString error = QStringLiteral("Permanent Save hanya diaktifkan melalui USB HID; capture native Store tersedia untuk USB.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Save"), error);
        return;
    }

    QString error;
    if (!beginOperation(Operation::Save, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Save"), error);
        return;
    }

    m_requestedSlot = qBound(1, slotOneBased, 10);
    m_storeChain = {};
    setProgress(QStringLiteral("Preparing slot %1 · fresh device readback").arg(m_requestedSlot));
    startReadback(ReadbackPurpose::SavePrepare);
}

void K500PresetManager::massUploadSlotImages(const QVariantList &entries)
{
    if (!usbStoreAvailable()) {
        const QString error = QStringLiteral("Mass Upload hanya diaktifkan melalui USB HID.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Mass Upload"), error);
        return;
    }

    QVector<MassEntry> normalized;
    QSet<int> seen;
    for (const QVariant &entryValue : entries) {
        const QVariantMap entry = entryValue.toMap();
        const int slot = qBound(1, entry.value(QStringLiteral("slot"), 1).toInt(), 10);
        const QByteArray image = entry.value(QStringLiteral("image")).toByteArray();
        if (seen.contains(slot))
            continue;
        if (image.size() != K500PresetProtocol::DeviceSlotImageLength) {
            const QString error = QStringLiteral("Mass Upload slot %1 harus membawa image 0x0290 (%2) byte, bukan %3 byte.")
                                      .arg(slot).arg(K500PresetProtocol::DeviceSlotImageLength).arg(image.size());
            if (m_manager) m_manager->setError(error);
            emit operationFailed(QStringLiteral("Mass Upload"), error);
            return;
        }
        seen.insert(slot);
        normalized.append(MassEntry{slot, image});
    }

    if (normalized.isEmpty()) {
        const QString error = QStringLiteral("Tidak ada slot image valid untuk Mass Upload.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Mass Upload"), error);
        return;
    }

    std::sort(normalized.begin(), normalized.end(), [](const MassEntry &a, const MassEntry &b) {
        return a.slot > b.slot; // exact donor/native mass-upload order
    });

    QString error;
    if (!beginOperation(Operation::MassUpload, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Mass Upload"), error);
        return;
    }

    m_massEntries = normalized;
    m_massIndex = -1;
    m_storeChain = {};
    setProgress(QStringLiteral("Mass upload · %1 slot").arg(m_massEntries.size()));
    startNextMassEntry();
}

void K500PresetManager::onBytesReceived(const QByteArray &bytes)
{
    const auto responses = m_parser.feed(bytes);
    for (const K500Response &response : responses)
        onResponse(response);
}

void K500PresetManager::onResponse(const K500Response &response)
{
    // The C0 handshake reports active slot zero-based. Capture it even outside
    // P2 operations so System UI has an authoritative slot when available.
    if (response.checksumOk && response.rsp == 0xC0 && !response.data.isEmpty()) {
        const int slot = qBound(1, static_cast<int>(static_cast<quint8>(response.data.at(0))) + 1, 10);
        if (m_activeSlot != slot) {
            m_activeSlot = slot;
            emit activeSlotChanged();
        }
    }

    if (!busy() || !response.checksumOk)
        return;

    if (m_step == Step::AwaitRecallHandshake && response.rsp == 0xC0) {
        clearTimeout();
        if (!response.data.isEmpty()) {
            const int slot = qBound(1, static_cast<int>(static_cast<quint8>(response.data.at(0))) + 1, 10);
            if (m_activeSlot != slot) {
                m_activeSlot = slot;
                emit activeSlotChanged();
            }
        } else if (m_activeSlot != m_requestedSlot) {
            m_activeSlot = m_requestedSlot;
            emit activeSlotChanged();
        }
        startReadback(ReadbackPurpose::Recall);
        return;
    }

    if (m_step == Step::Readback && response.rsp == 0xBF) {
        acceptReadBlock(response.data);
        return;
    }

    if (m_step == Step::AwaitUseInitAck && response.rsp == 0xED) {
        clearTimeout();
        setProgress(QStringLiteral("Use init volume %1 · acknowledged").arg(m_useInitVolume ? QStringLiteral("ON") : QStringLiteral("OFF")));
        finishOperation(QStringLiteral("Use Init Volume"));
        return;
    }

    if (m_step == Step::AwaitMassBeginAck && response.rsp == 0xBE) {
        clearTimeout();
        sendNextStoreChunk();
        return;
    }

    if (m_step == Step::AwaitChunkAck && response.rsp == 0xBD) {
        clearTimeout();
        m_storeOffset += m_pendingStoreLength;
        m_pendingStoreLength = 0;
        sendNextStoreChunk();
        return;
    }

    if (m_step == Step::AwaitCommitAck && response.rsp == 0xBC) {
        clearTimeout();
        acceptStoreCommit();
    }
}

void K500PresetManager::startReadback(ReadbackPurpose purpose)
{
    m_readbackPurpose = purpose;
    m_readbackMemory = QByteArray(ActiveMemorySize, char(0));
    m_readOffset = 0;
    m_pendingReadLength = 0;
    m_step = Step::Readback;
    sendNextReadBlock();
}

void K500PresetManager::sendNextReadBlock()
{
    if (m_step != Step::Readback)
        return;
    if (m_readOffset >= ActiveMemorySize) {
        finishReadback();
        return;
    }

    m_pendingReadLength = qMin(ActiveMemoryBlockSize, ActiveMemorySize - m_readOffset);
    const quint8 mode = m_manager->m_io.kind() == K500WinIo::Kind::UsbHid ? 0x00 : 0x63;
    setProgress(QStringLiteral("%1 · reading K500 %2/%3")
                    .arg(operationName(m_operation)).arg(m_readOffset).arg(ActiveMemorySize));
    if (!send(K500Protocol::readBlock(static_cast<quint16>(m_readOffset),
                                      static_cast<quint16>(m_pendingReadLength), mode),
              QStringLiteral("P2 read 0x%1 len %2")
                  .arg(m_readOffset, 4, 16, QLatin1Char('0')).arg(m_pendingReadLength)))
        return;
    armTimeout(ReadbackTimeoutMs, operationName(m_operation),
               QStringLiteral("Timeout P2 readback di 0x%1; LIVE tetap OFF.")
                   .arg(m_readOffset, 4, 16, QLatin1Char('0')));
}

void K500PresetManager::acceptReadBlock(const QByteArray &data)
{
    clearTimeout();
    if (m_pendingReadLength <= 0 || data.size() < m_pendingReadLength) {
        failOperation(operationName(m_operation),
                      QStringLiteral("P2 readback block 0x%1 terlalu pendek: %2/%3 byte.")
                          .arg(m_readOffset, 4, 16, QLatin1Char('0'))
                          .arg(data.size()).arg(m_pendingReadLength));
        return;
    }

    m_readbackMemory.replace(m_readOffset, m_pendingReadLength, data.left(m_pendingReadLength));
    m_readOffset += m_pendingReadLength;
    m_pendingReadLength = 0;

    if (m_readOffset >= ActiveMemorySize) {
        finishReadback();
        return;
    }
    QTimer::singleShot(ActiveMemoryInterBlockMs, this, [this] {
        if (m_step == Step::Readback)
            sendNextReadBlock();
    });
}

void K500PresetManager::finishReadback()
{
    if (m_readbackMemory.size() != ActiveMemorySize || m_readOffset < ActiveMemorySize) {
        failOperation(operationName(m_operation), QStringLiteral("P2 full readback tidak lengkap."));
        return;
    }

    emit activeMemoryReady(m_readbackMemory);

    if (m_readbackPurpose == ReadbackPurpose::Recall) {
        setProgress(QStringLiteral("Recall slot %1 · 939-byte resync complete").arg(m_activeSlot > 0 ? m_activeSlot : m_requestedSlot));
        finishOperation(QStringLiteral("Recall"), m_activeSlot > 0 ? m_activeSlot : m_requestedSlot);
        return;
    }

    if (m_readbackPurpose == ReadbackPurpose::SavePrepare) {
        // P2_SAVE_FROM_DEVICE_TRUTH_V1
        // The slot image is exactly the first 0x0290 bytes of freshly-read
        // active memory. No .k500 serializer is needed and no stale editor
        // defaults can contaminate permanent storage.
        m_storeImage = m_readbackMemory.left(K500PresetProtocol::DeviceSlotImageLength);
        m_storeChain = {};
        beginStoreSlot(false);
        return;
    }
}

void K500PresetManager::beginStoreSlot(bool waitForBeginAck)
{
    if (m_storeImage.size() != K500PresetProtocol::DeviceSlotImageLength) {
        failOperation(operationName(m_operation), QStringLiteral("Store image bukan 0x0290 byte."));
        return;
    }

    m_storeOffset = 0;
    m_pendingStoreLength = 0;
    m_commitFrame.clear();
    setProgress(QStringLiteral("%1 slot %2 · begin 0x41")
                    .arg(m_operation == Operation::MassUpload ? QStringLiteral("Mass upload") : QStringLiteral("Save"))
                    .arg(m_requestedSlot));
    if (!send(K500PresetProtocol::storeBegin(m_storeImage, m_storeChain),
              QStringLiteral("Store begin slot %1 · %2 bytes")
                  .arg(m_requestedSlot).arg(K500PresetProtocol::DeviceSlotImageLength)))
        return;

    if (waitForBeginAck) {
        m_step = Step::AwaitMassBeginAck;
        armTimeout(StoreAckTimeoutMs, operationName(m_operation),
                   QStringLiteral("Timeout menunggu Store begin RSP 0xBE slot %1.").arg(m_requestedSlot));
        return;
    }

    // Native single-slot Save capture proceeds to CMD 0x42 after 80 ms and
    // does not wait for 0xBE. Only Mass Upload uses the begin ACK chain.
    m_step = Step::SingleStoreBeginDelay;
    QTimer::singleShot(SingleStoreBeginSettleMs, this, [this] {
        if (m_operation == Operation::Save && m_step == Step::SingleStoreBeginDelay)
            sendNextStoreChunk();
    });
}

void K500PresetManager::sendNextStoreChunk()
{
    if (m_storeOffset >= K500PresetProtocol::DeviceSlotImageLength) {
        sendStoreCommit();
        return;
    }

    m_pendingStoreLength = qMin(K500PresetProtocol::DeviceSlotWriteChunk,
                                K500PresetProtocol::DeviceSlotImageLength - m_storeOffset);
    const QByteArray data = m_storeImage.mid(m_storeOffset, m_pendingStoreLength);
    const int blockIndex = m_storeOffset / K500PresetProtocol::DeviceSlotWriteChunk + 1;
    const int totalBlocks = (K500PresetProtocol::DeviceSlotImageLength
                             + K500PresetProtocol::DeviceSlotWriteChunk - 1)
                            / K500PresetProtocol::DeviceSlotWriteChunk;
    setProgress(QStringLiteral("Slot %1 · block %2/%3")
                    .arg(m_requestedSlot).arg(blockIndex).arg(totalBlocks));
    if (!send(K500PresetProtocol::storeChunk(static_cast<quint16>(m_storeOffset), data),
              QStringLiteral("Store slot %1 · 0x%2 · %3 bytes")
                  .arg(m_requestedSlot)
                  .arg(m_storeOffset, 4, 16, QLatin1Char('0'))
                  .arg(m_pendingStoreLength)))
        return;

    m_step = Step::AwaitChunkAck;
    armTimeout(StoreAckTimeoutMs, operationName(m_operation),
               QStringLiteral("Timeout menunggu Store chunk RSP 0xBD slot %1 offset 0x%2.")
                   .arg(m_requestedSlot).arg(m_storeOffset, 4, 16, QLatin1Char('0')));
}

void K500PresetManager::sendStoreCommit()
{
    m_commitFrame = K500PresetProtocol::storeCommit(m_requestedSlot, m_storeImage);
    setProgress(QStringLiteral("Slot %1 · commit 0x43").arg(m_requestedSlot));
    if (!send(m_commitFrame, QStringLiteral("Store commit slot %1").arg(m_requestedSlot)))
        return;
    m_step = Step::AwaitCommitAck;
    armTimeout(StoreAckTimeoutMs, operationName(m_operation),
               QStringLiteral("Timeout menunggu Store commit RSP 0xBC slot %1.").arg(m_requestedSlot));
}

void K500PresetManager::acceptStoreCommit()
{
    m_storeChain = K500PresetProtocol::nextStoreChain(m_storeImage, m_commitFrame);
    setProgress(QStringLiteral("Slot %1 saved · commit acknowledged").arg(m_requestedSlot));

    if (m_operation == Operation::MassUpload) {
        if (m_massIndex + 1 < m_massEntries.size()) {
            startNextMassEntry();
            return;
        }

        const int count = m_massEntries.size();
        m_massEntries.clear();
        m_massIndex = -1;
        setProgress(QStringLiteral("%1 slot uploaded · refreshing slot 1").arg(count));
        finishOperation(QStringLiteral("Mass Upload"));
        QTimer::singleShot(0, this, [this] {
            if (connected() && !busy())
                recallMode(1);
        });
        return;
    }

    finishOperation(QStringLiteral("Save"), m_requestedSlot);
}

void K500PresetManager::startNextMassEntry()
{
    ++m_massIndex;
    if (m_massIndex < 0 || m_massIndex >= m_massEntries.size()) {
        failOperation(QStringLiteral("Mass Upload"), QStringLiteral("Mass Upload index internal invalid."));
        return;
    }

    m_requestedSlot = m_massEntries.at(m_massIndex).slot;
    m_storeImage = m_massEntries.at(m_massIndex).image;
    setProgress(QStringLiteral("Mass upload %1/%2 · slot %3")
                    .arg(m_massIndex + 1).arg(m_massEntries.size()).arg(m_requestedSlot));
    beginStoreSlot(true);
}
