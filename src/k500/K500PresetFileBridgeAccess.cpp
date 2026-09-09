#include "K500DeviceManager.h"

#include "K500PresetFileBridge.h"

#include <QTimer>

QObject *K500DeviceManager::presetFileBridge() const
{
    if (m_presetFileBridge)
        return m_presetFileBridge;

    // Follow the same lazy QObject exposure pattern as presetManager. The file
    // bridge never owns or bypasses hardware I/O.
    auto *self = const_cast<K500DeviceManager *>(this);
    auto *bridge = new K500PresetFileBridge(self);
    self->m_presetFileBridge = bridge;

    // Offline Preview may deliberately enable controlled .k500 edit persistence.
    // As soon as real hardware connects, force tracking off at the backend edge.
    QObject::connect(self, &K500DeviceManager::statusChanged, bridge, [self, bridge] {
        if (self->connected())
            bridge->setEditTracking(false);
    });

    // OFFICIAL_PRESET_AUTO_SYNC_V1 — the user sees the bundled/cache library
    // immediately; shortly after the event loop starts we check GitHub once for
    // new/changed official presets. The direct K500PresetFileBridge unit tests do
    // not take this DeviceManager path, so they stay deterministic/offline.
    QTimer::singleShot(1200, bridge, [bridge] {
        bridge->syncOfficialPresets();
    });

    return bridge;
}
