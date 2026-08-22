#include "K500PresetManager.h"

#include "K500DeviceManager.h"

#include <QtGlobal>

void K500PresetManager::uploadSlotImage(int slotOneBased, const QByteArray &image)
{
    // P4_PC_PRESET_UPLOAD_V1
    // The P3 file bridge supplies an already validated/converted native 0x0290
    // image. Unlike saveCurrentToSlot(), this path MUST NOT perform a fresh
    // device readback because that would replace the PC preset being uploaded.
    if (!usbStoreAvailable()) {
        const QString error = QStringLiteral("Upload preset PC hanya diaktifkan melalui USB HID.");
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Upload"), error);
        return;
    }

    if (image.size() != K500PresetProtocol::DeviceSlotImageLength) {
        const QString error = QStringLiteral("Upload membutuhkan native slot image 0x0290 (%1) byte, bukan %2 byte.")
                                  .arg(K500PresetProtocol::DeviceSlotImageLength)
                                  .arg(image.size());
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Upload"), error);
        return;
    }

    QString error;
    // Donor savePresetToSlot uses the same single-store transaction as the P2
    // Save path: Store Begin, settle delay (no 0xBE wait), chunk ACKs, Commit.
    if (!beginOperation(Operation::Save, &error)) {
        if (m_manager) m_manager->setError(error);
        emit operationFailed(QStringLiteral("Upload"), error);
        return;
    }

    m_requestedSlot = qBound(1, slotOneBased, 10);
    m_storeImage = image;
    m_storeChain = {};
    setProgress(QStringLiteral("Upload PC preset → slot %1 · verified 0x0290 image").arg(m_requestedSlot));
    beginStoreSlot(false);
}
