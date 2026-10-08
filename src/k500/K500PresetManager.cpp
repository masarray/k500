#include "K500PresetManager.h"

#include "K500DeviceManager.h"
#include "K500Protocol.h"

#include <QSet>
#include <QVariantMap>
#include <algorithm>

namespace {
constexpr int ActiveMemoryInterBlockMs = 35;
constexpr int ReadbackTimeoutMs = 2600;
constexpr int RecallHandshakeTimeoutMs = 3000;
constexpr int UseInitTimeoutMs = 2200;
constexpr int BtIdentityTimeoutMs = 2200;
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
    // ADJ_MANNER_VR_OFF_READBACK_20261004_V1 — hydrate the exact direct
    // active-memory flag on initial connect, Recall and authoritative reconciliation.
    connect(m_manager, &K500DeviceManager::activeMemoryReady,
            this, &K500PresetManager::hydrateAdjMannerFromMemory);
    connect(m_manager, &K500DeviceManager::reconciliationMemoryReady,
            this, &K500PresetManager::hydrateAdjMannerFromMemory);
    connect(m_manager, &K500DeviceManager::statusChanged, this, [this] {
        emit connectedChanged();
        if (!connected()) {
            // OFFLINE_DEVICE_SLOT_V1 — there is no such thing as an active K500
            // slot without a connected/handshaken device. Clear the last C0
            // result so System never paints a stale ACTIVE badge while offline.
            if (m_activeSlot != 0) {
                m_activeSlot = 0;
                emit activeSlotChanged();
            }

            // OFFLINE_DEVICE_TRUTH_INVALIDATION_V2
            // Disconnect removes hardware authority; it does NOT mean boolean
            // controls suddenly became false. Preserve the last accepted/read
            // payload and invalidate only its Known bit. The QML offline working
            // shadow already mirrors accepted online state and will continue from
            // that value. A future reconnect/readback replaces it authoritatively.
            bool useInitChanged = false;
            bool adjMannerChanged = false;

            // If transport disappears while a setter is still optimistic, roll
            // back to the last accepted value before dropping device authority.
            if (busy()) {
                clearTimeout();
                const Operation interruptedOperation = m_operation;
                if (interruptedOperation == Operation::UseInit
                    && (m_useInitVolume != m_previousUseInitVolume
                        || m_useInitVolumeKnown != m_previousUseInitVolumeKnown)) {
                    m_useInitVolume = m_previousUseInitVolume;
                    m_useInitVolumeKnown = m_previousUseInitVolumeKnown;
                    useInitChanged = true;
                }
                if (interruptedOperation == Operation::AdjManner
                    && (m_adjMannerVrOff != m_previousAdjMannerVrOff
                        || m_adjMannerVrOffKnown != m_previousAdjMannerVrOffKnown)) {
                    m_adjMannerVrOff = m_previousAdjMannerVrOff;
                    m_adjMannerVrOffKnown = m_previousAdjMannerVrOffKnown;
                    adjMannerChanged = true;
                }
                m_operation = Operation::None;
                m_step = Step::Idle;
                m_readbackPurpose = ReadbackPurpose::None;
                emit busyChanged();
            }

            if (m_useInitVolumeKnown) {
                m_useInitVolumeKnown = false;
                useInitChanged = true;
            }
            if (m_adjMannerVrOffKnown) {
                m_adjMannerVrOffKnown = false;
                adjMannerChanged = true;
            }
            if (useInitChanged)
                emit useInitVolumeChanged();
            if (adjMannerChanged)
                emit adjMannerVrOffChanged();
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
    case Operation::AdjManner: return QStringLiteral("Adj Manner VR OFF");
    case Operation::BtName: return QStringLiteral("BT Name");
    case Operation::Save: return QStringLiteral("Save");
    case Operation::Rename: return QStringLiteral("Rename Mode");
    case Operation::Upload: return QStringLiteral("Upload");
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

    if (failedOperation == Operation::UseInit
        && (m_useInitVolume != m_previousUseInitVolume
            || m_useInitVolumeKnown != m_previousUseInitVolumeKnown)) {
        m_useInitVolume = m_previousUseInitVolume;
        m_useInitVolumeKnown = m_previousUseInitVolumeKnown;
        emit useInitVolumeChanged();
    }
    if (failedOperation == Operation::AdjManner
        && (m_adjMannerVrOff != m_previousAdjMannerVrOff
            || m_adjMannerVrOffKnown != m_previousAdjMannerVrOffKnown)) {
        m_adjMannerVrOff = m_previousAdjMannerVrOff;
        m_adjMannerVrOffKnown = m_previousAdjMannerVrOffKnown;
        emit adjMannerVrOffChanged();
    }
    m_requestedModeName.clear();
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

void K500PresetManager::setMassUploadProgressPercent(int percent)
{
    // MASS_UPLOAD_ACK_PROGRESS_V1: percent is derived from ACKed 0x0290
    // payload bytes, never from frames merely sent or timer animation.
    const int bounded = qBound(0, percent, 100);
    if (m_massUploadProgressPercent == bounded)
        return;
    m_massUploadProgressPercent = bounded;
    emit massUploadProgressChanged();
}

void K500PresetManager::updateMassUploadAcknowledgedProgress()
{
    if (m_operation != Operation::MassUpload || m_massEntries.isEmpty())
        return;
    const qint64 bytesPerSlot = K500PresetProtocol::DeviceSlotImageLength;
    const qint64 totalBytes = m_massEntries.size() * bytesPerSlot;
    // The last 1% is reserved for final Recall/939-byte readback.
    const qint64 accepted = qBound<qint64>(0,
        qint64(m_massIndex) * bytesPerSlot + m_storeOffset, totalBytes);
    setMassUploadProgressPercent(static_cast<int>(99 * accepted / totalBytes));
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
    setProgress(QStringLiteral("%1 · slot %2 refresh handshake")
                    .arg(operationName(m_operation)).arg(m_requestedSlot));
    if (!send(K500PresetProtocol::recallHandshake(), QStringLiteral("Recall refresh handshake · mask 0x03")))
        return;
    armTimeout(RecallHandshakeTimeoutMs, operationName(m_operation),
               QStringLiteral("Timeout menunggu RSP 0xC0 setelah Recall."));
}

void K500PresetManager::setUseInitVolume(bool enabled)
{
    // USE_INIT_DEVICE_TRUTH_V1 + USE_INIT_DEVICE_TRUTH_V2
    // Never treat a host-side preference as K500 truth. C0 data[7] bit 0x04
    // hydrates the connect/Recall state; the setter/ACK pair remains:
    // OFF AA 03 00 12 00 03 E8, ON AA 03 00 12 01 03 E7, ACK RSP 0xED.
    // A local edit becomes current-session truth only after that valid ACK.
    if (!connected()) {
        const QString error = QStringLiteral("Use Init Volume memerlukan K500 connected; device state tidak boleh dipalsukan dari preference PC.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Use Init Volume"), error);
        return;
    }

    if (m_useInitVolumeKnown && enabled == m_useInitVolume && !busy())
        return;

    QString error;
    if (!beginOperation(Operation::UseInit, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Use Init Volume"), error);
        return;
    }

    m_previousUseInitVolume = m_useInitVolume;
    m_previousUseInitVolumeKnown = m_useInitVolumeKnown;
    m_useInitVolume = enabled;
    m_useInitVolumeKnown = false;
    emit useInitVolumeChanged();
    m_step = Step::AwaitUseInitAck;
    setProgress(QStringLiteral("Use init volume %1").arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF")));
    if (!send(K500PresetProtocol::useInitVolume(enabled),
              QStringLiteral("Use init vol %1 · mask 0x03").arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF"))))
        return;
    armTimeout(UseInitTimeoutMs, QStringLiteral("Use Init Volume"),
               QStringLiteral("Timeout menunggu RSP 0xED untuk Use Init Volume."));
}

void K500PresetManager::setAdjMannerVrOff(bool enabled)
{
    // ADJ_MANNER_VR_OFF_NATIVE_PARITY_V3 — physical captures prove:
    // runtime write = CMD 0x07 + route 0x03 -> RSP 0xF8 and stop.
    // Connect/reconnect/Recall truth = activeMemory[0x008C].
    // C0 data[19] remains intentionally non-authoritative.
    if (!connected()) {
        const QString error = QStringLiteral("Adj Manner VR OFF memerlukan K500 connected.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Adj Manner VR OFF"), error);
        return;
    }
    if (m_adjMannerVrOffKnown && enabled == m_adjMannerVrOff && !busy())
        return;

    QString error;
    if (!beginOperation(Operation::AdjManner, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Adj Manner VR OFF"), error);
        return;
    }

    m_previousAdjMannerVrOff = m_adjMannerVrOff;
    m_previousAdjMannerVrOffKnown = m_adjMannerVrOffKnown;
    m_adjMannerVrOff = enabled;
    m_adjMannerVrOffKnown = false;
    emit adjMannerVrOffChanged();
    m_step = Step::AwaitAdjMannerAck;
    setProgress(QStringLiteral("Adj Manner VR OFF %1").arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF")));
    if (!send(K500Protocol::adjMannerVrOff(enabled),
              QStringLiteral("Adj Manner VR OFF %1 · CMD 0x07").arg(enabled ? QStringLiteral("ON") : QStringLiteral("OFF"))))
        return;
    armTimeout(UseInitTimeoutMs, QStringLiteral("Adj Manner VR OFF"),
               QStringLiteral("Timeout menunggu RSP 0xF8 untuk Adj Manner VR OFF."));
}

void K500PresetManager::setBtName(const QString &name)
{
    if (!usbStoreAvailable()) {
        const QString error = QStringLiteral("BT Name rename hanya dipromosikan untuk USB HID yang tercapture.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("BT Name"), error);
        return;
    }
    const QByteArray frame = K500Protocol::btNameSet(name);
    if (frame.isEmpty()) {
        const QString error = QStringLiteral("BT Name harus 1..8 karakter ASCII printable.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("BT Name"), error);
        return;
    }

    QString error;
    if (!beginOperation(Operation::BtName, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("BT Name"), error);
        return;
    }

    m_step = Step::AwaitBtNameAck;
    setProgress(QStringLiteral("BT Name · writing '%1'").arg(name.trimmed()));
    if (!send(frame, QStringLiteral("BT Name SET · CMD 0x4E")))
        return;
    armTimeout(BtIdentityTimeoutMs, QStringLiteral("BT Name"),
               QStringLiteral("Timeout menunggu RSP 0xB1 untuk BT Name."));
}

void K500PresetManager::resetBtName()
{
    if (!usbStoreAvailable()) {
        const QString error = QStringLiteral("BT Name reset hanya dipromosikan untuk USB HID yang tercapture.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("BT Name"), error);
        return;
    }
    QString error;
    if (!beginOperation(Operation::BtName, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("BT Name"), error);
        return;
    }

    m_step = Step::AwaitBtNameAck;
    setProgress(QStringLiteral("BT Name · reset"));
    if (!send(K500Protocol::btNameReset(), QStringLiteral("BT Name RESET · CMD 0x4E")))
        return;
    armTimeout(BtIdentityTimeoutMs, QStringLiteral("BT Name"),
               QStringLiteral("Timeout menunggu RSP 0xB1 untuk BT Name reset."));
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

void K500PresetManager::renameActiveMode(const QString &name)
{
    if (!usbStoreAvailable()) {
        const QString error = QStringLiteral("Persistent Mode Rename hanya diaktifkan melalui USB HID.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Rename Mode"), error);
        return;
    }
    if (m_activeSlot < 1 || m_activeSlot > 10) {
        const QString error = QStringLiteral("Active device slot belum diketahui; Recall slot terlebih dahulu.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Rename Mode"), error);
        return;
    }

    QString nameError;
    const QByteArray probe = K500PresetProtocol::withModeName(
        QByteArray(K500PresetProtocol::DeviceSlotImageLength, char(0)), name, &nameError);
    if (probe.isEmpty()) {
        if (m_manager) m_manager->setError(nameError);
        emit operationFailed(QStringLiteral("Rename Mode"), nameError);
        return;
    }

    QString error;
    if (!beginOperation(Operation::Rename, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Rename Mode"), error);
        return;
    }

    m_requestedSlot = m_activeSlot;
    m_requestedModeName = name.trimmed();
    m_storeChain = {};
    setProgress(QStringLiteral("Rename slot %1 · fresh device readback").arg(m_requestedSlot));
    startReadback(ReadbackPurpose::RenamePrepare);
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

    setMassUploadProgressPercent(0);
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

void K500PresetManager::hydrateAdjMannerFromMemory(const QByteArray &memory)
{
    const int offset = K500Protocol::ReadbackOffset::AdjMannerVrOff;
    if (memory.size() <= offset)
        return;

    const quint8 raw = static_cast<quint8>(static_cast<unsigned char>(memory.at(offset)));
    if (raw > 1)
        return;

    const bool enabled = raw == 1;
    if (m_adjMannerVrOffKnown && m_adjMannerVrOff == enabled)
        return;

    m_adjMannerVrOff = enabled;
    m_adjMannerVrOffKnown = true;
    emit adjMannerVrOffChanged();
}

void K500PresetManager::onResponse(const K500Response &response)
{
    // The C0 handshake reports active slot zero-based. Capture it during both
    // initial connection and later Recall handshakes. Stage::Idle is the only
    // state where a C0 must never resurrect a stale offline ACTIVE badge.
    if (m_manager && m_manager->m_stage != K500DeviceManager::Stage::Idle
        && response.checksumOk && response.rsp == 0xC0 && !response.data.isEmpty()) {
        const int slot = qBound(1, static_cast<int>(static_cast<quint8>(response.data.at(0))) + 1, 10);
        if (m_activeSlot != slot) {
            m_activeSlot = slot;
            emit activeSlotChanged();
        }

        bool deviceUseInit = false;
        if (K500ResponseParser::tryDecodeUseInitVolume(response, &deviceUseInit)
            && (!m_useInitVolumeKnown || m_useInitVolume != deviceUseInit)) {
            m_useInitVolume = deviceUseInit;
            m_useInitVolumeKnown = true;
            emit useInitVolumeChanged();
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
        m_useInitVolumeKnown = true;
        emit useInitVolumeChanged();
        setProgress(QStringLiteral("Use init volume %1 · device acknowledged").arg(m_useInitVolume ? QStringLiteral("ON") : QStringLiteral("OFF")));
        finishOperation(QStringLiteral("Use Init Volume"));
        return;
    }

    if (m_step == Step::AwaitAdjMannerAck && response.rsp == 0xF8) {
        // ADJ_MANNER_ACK_SESSION_AUTHORITY_V1
        // Native runtime capture proves the transaction ends at F8:
        // CMD 0x07 -> RSP 0xF8, with no immediate CMD 0x40 readback.
        // F8 commits current-session state; activeMemory[0x008C] remains
        // authoritative only on real connect/reconnect/Recall readback.
        clearTimeout();
        m_adjMannerVrOffKnown = true;
        emit adjMannerVrOffChanged();
        if (m_manager)
            emit m_manager->adjMannerVrOffAccepted(m_adjMannerVrOff);
        setProgress(QStringLiteral("Adj Manner VR OFF %1 · device acknowledged")
                        .arg(m_adjMannerVrOff ? QStringLiteral("ON") : QStringLiteral("OFF")));
        finishOperation(QStringLiteral("Adj Manner VR OFF"));
        return;
    }

    if (m_step == Step::AwaitBtNameAck && response.rsp == 0xB1) {
        clearTimeout();
        setProgress(QStringLiteral("BT Name · device acknowledged · refreshing identity"));
        startReadback(ReadbackPurpose::BtIdentity);
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
        updateMassUploadAcknowledgedProgress();
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
    m_readbackMemory = QByteArray(K500Protocol::ActiveMemorySize, char(0));
    m_readOffset = 0;
    m_pendingReadLength = 0;
    m_step = Step::Readback;
    sendNextReadBlock();
}

void K500PresetManager::sendNextReadBlock()
{
    if (m_step != Step::Readback)
        return;
    if (m_readOffset >= K500Protocol::ActiveMemorySize) {
        finishReadback();
        return;
    }

    m_pendingReadLength = qMin(K500Protocol::ActiveMemoryBlockSize, K500Protocol::ActiveMemorySize - m_readOffset);
    const quint8 mode = m_manager->m_io.kind() == K500WinIo::Kind::UsbHid ? 0x00 : 0x63;
    setProgress(QStringLiteral("%1 · reading K500 %2/%3")
                    .arg(operationName(m_operation)).arg(m_readOffset).arg(K500Protocol::ActiveMemorySize));
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

    if (m_readOffset >= K500Protocol::ActiveMemorySize) {
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
    if (m_readbackMemory.size() != K500Protocol::ActiveMemorySize || m_readOffset < K500Protocol::ActiveMemorySize) {
        failOperation(operationName(m_operation), QStringLiteral("P2 full readback tidak lengkap."));
        return;
    }

    emit activeMemoryReady(m_readbackMemory);

    if (m_readbackPurpose == ReadbackPurpose::Recall) {
        const int resolvedSlot = m_activeSlot > 0 ? m_activeSlot : m_requestedSlot;
        if (m_operation == Operation::Upload) {
            setProgress(QStringLiteral("Upload slot %1 · device activated and 939-byte resync complete").arg(resolvedSlot));
            finishOperation(QStringLiteral("Upload"), resolvedSlot);
            return;
        }
        if (m_operation == Operation::MassUpload) {
            setMassUploadProgressPercent(100); // Only verified final readback.
            setProgress(QStringLiteral("Mass upload complete · slot 1 active · 939-byte resync complete"));
            finishOperation(QStringLiteral("Mass Upload"), resolvedSlot);
            return;
        }
        if (m_operation == Operation::Rename) {
            setProgress(QStringLiteral("Rename slot %1 · committed and 939-byte resync complete").arg(resolvedSlot));
            m_requestedModeName.clear();
            finishOperation(QStringLiteral("Rename Mode"), resolvedSlot);
            return;
        }

        setProgress(QStringLiteral("Recall slot %1 · 939-byte resync complete").arg(resolvedSlot));
        finishOperation(QStringLiteral("Recall"), resolvedSlot);
        return;
    }

    if (m_readbackPurpose == ReadbackPurpose::BtIdentity) {
        setProgress(QStringLiteral("BT Name · 939-byte identity refresh complete"));
        finishOperation(QStringLiteral("BT Name"));
        return;
    }

    if (m_readbackPurpose == ReadbackPurpose::RenamePrepare) {
        // MODE_NAME_STORE_CAPTURED_V1 — begin from exact current device truth
        // and patch only slot-image bytes 0x0280..0x028F before the native
        // 0x41/0x42/0x43 store sequence.
        QString renameError;
        m_storeImage = K500PresetProtocol::withModeName(
            m_readbackMemory.left(K500PresetProtocol::DeviceSlotImageLength),
            m_requestedModeName, &renameError);
        if (m_storeImage.isEmpty()) {
            failOperation(QStringLiteral("Rename Mode"), renameError);
            return;
        }
        m_storeChain = {};
        beginStoreSlot(false);
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
    const QString storeKind = m_operation == Operation::MassUpload
        ? QStringLiteral("Mass upload")
        : (m_operation == Operation::Upload ? QStringLiteral("Upload")
           : (m_operation == Operation::Rename ? QStringLiteral("Rename") : QStringLiteral("Save")));
    setProgress(QStringLiteral("%1 slot %2 · begin 0x41")
                    .arg(storeKind)
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

    // Native single-slot Save/Upload capture proceeds to CMD 0x42 after 80 ms
    // and does not wait for 0xBE. Only Mass Upload uses the begin ACK chain.
    m_step = Step::SingleStoreBeginDelay;
    QTimer::singleShot(SingleStoreBeginSettleMs, this, [this] {
        if ((m_operation == Operation::Save || m_operation == Operation::Rename
             || m_operation == Operation::Upload)
            && m_step == Step::SingleStoreBeginDelay)
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
        // Slot becomes durable only after 0xBC; never infer success from
        // sending 0x43. This also accounts for an ACKed final slot.
        m_storeOffset = K500PresetProtocol::DeviceSlotImageLength;
        updateMassUploadAcknowledgedProgress();
        if (m_massIndex + 1 < m_massEntries.size()) {
            startNextMassEntry();
            return;
        }

        const int count = m_massEntries.size();
        m_massEntries.clear();
        m_massIndex = -1;
        m_requestedSlot = 1;
        setProgress(QStringLiteral("%1 slot uploaded · recalling slot 1 before LIVE resumes").arg(count));
        m_step = Step::RecallDelay;
        if (!send(K500PresetProtocol::recallMode(m_requestedSlot),
                  QStringLiteral("Mass Upload final recall slot 1 · mask 0x03")))
            return;
        QTimer::singleShot(RecallSettleMs, this, [this] {
            if (m_operation == Operation::MassUpload && m_step == Step::RecallDelay)
                sendRecallHandshake();
        });
        return;
    }

    if (m_operation == Operation::Rename) {
        const int renamedSlot = m_requestedSlot;
        setProgress(QStringLiteral("Rename slot %1 committed · recalling same slot for verification").arg(renamedSlot));
        m_step = Step::RecallDelay;
        if (!send(K500PresetProtocol::recallMode(renamedSlot),
                  QStringLiteral("Rename final recall slot %1 · mask 0x03").arg(renamedSlot)))
            return;
        QTimer::singleShot(RecallSettleMs, this, [this] {
            if (m_operation == Operation::Rename && m_step == Step::RecallDelay)
                sendRecallHandshake();
        });
        return;
    }

    if (m_operation == Operation::Upload) {
        const int uploadedSlot = m_requestedSlot;
        setProgress(QStringLiteral("Upload slot %1 committed · recalling same slot before LIVE resumes").arg(uploadedSlot));
        m_step = Step::RecallDelay;
        if (!send(K500PresetProtocol::recallMode(uploadedSlot),
                  QStringLiteral("Upload final recall slot %1 · mask 0x03").arg(uploadedSlot)))
            return;
        QTimer::singleShot(RecallSettleMs, this, [this] {
            if (m_operation == Operation::Upload && m_step == Step::RecallDelay)
                sendRecallHandshake();
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
