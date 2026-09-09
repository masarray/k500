#include "K500DeviceManager.h"

#include "K500PresetFileBridge.h"

#include <QTimer>

QObject *K500DeviceManager::presetFileBridge() const
{
    if (m_presetFileBridge)
        return m_presetFileBridge;

    // P3_3_FILE_BRIDGE_QML_ACCESS_V1
    // Follow the established lazy QObject exposure pattern. The file bridge
    // never owns or bypasses K500 hardware I/O.
    auto *self = const_cast<K500DeviceManager *>(this);
    auto *bridge = new K500PresetFileBridge(self);
    self->m_presetFileBridge = bridge;

    // DEVICE_TRUTH_EDIT_ISOLATION_V1
    // Offline Preview may deliberately enable controlled .k500 edit persistence.
    // As soon as real hardware connects, force tracking off at the backend edge.
    QObject::connect(self, &K500DeviceManager::statusChanged, bridge, [self, bridge] {
        if (self->connected())
            bridge->setEditTracking(false);
    });

    // OFFICIAL_PRESET_AUTO_SYNC_V1
    // Bundled/cache presets are visible immediately. Then check GitHub once for
    // new/changed official files without blocking UI startup. Direct bridge unit
    // tests do not take this DeviceManager path and therefore stay deterministic.
    QTimer::singleShot(1200, bridge, [bridge] {
        bridge->syncOfficialPresets();
    });

    return bridge;
}
