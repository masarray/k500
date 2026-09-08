#include "K500DeviceManager.h"

#include "K500PresetFileBridge.h"

QObject *K500DeviceManager::presetFileBridge() const
{
    if (m_presetFileBridge)
        return m_presetFileBridge;

    // P3_3_FILE_BRIDGE_QML_ACCESS_V1
    // Follow the same lazy QObject exposure pattern as P2 presetManager.
    // The file bridge never owns or bypasses hardware I/O; QML supplies the
    // existing StudioEngine object for explicit offline preview/edit only.
    auto *self = const_cast<K500DeviceManager *>(this);
    auto *bridge = new K500PresetFileBridge(self);
    self->m_presetFileBridge = bridge;

    // DEVICE_TRUTH_EDIT_ISOLATION_V1
    // Offline Preview may deliberately enable controlled .k500 edit persistence.
    // As soon as a real K500 session reaches connected state, force that tracking
    // off at the backend boundary as well as in QML. This prevents any later LIVE
    // fader/PEQ edit from mutating a staged PC document behind the user's back.
    QObject::connect(self, &K500DeviceManager::statusChanged, bridge, [self, bridge] {
        if (self->connected())
            bridge->setEditTracking(false);
    });

    return bridge;
}
