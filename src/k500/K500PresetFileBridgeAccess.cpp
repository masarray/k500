#include "K500DeviceManager.h"

#include "K500PresetFileBridge.h"

QObject *K500DeviceManager::presetFileBridge() const
{
    if (m_presetFileBridge)
        return m_presetFileBridge;

    // P3_3_FILE_BRIDGE_QML_ACCESS_V1
    // Follow the same lazy QObject exposure pattern as P2 presetManager.
    // The file bridge never owns or bypasses hardware I/O; QML supplies the
    // existing StudioEngine object for offline hydration only.
    auto *self = const_cast<K500DeviceManager *>(this);
    auto *bridge = new K500PresetFileBridge(self);
    self->m_presetFileBridge = bridge;
    return bridge;
}
