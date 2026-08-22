#include "K500DeviceManager.h"

#include "K500PresetManager.h"

QObject *K500DeviceManager::presetManager() const
{
    if (m_presetManager)
        return m_presetManager;

    auto *self = const_cast<K500DeviceManager *>(this);
    auto *preset = new K500PresetManager(self, self);
    self->m_presetManager = preset;

    // Reuse the already-proven hydration fan-out. P2 recall/save readbacks feed
    // the same Controller + StudioEngine consumers as the P0 connect readback.
    QObject::connect(preset, &K500PresetManager::activeMemoryReady,
                     self, [self](const QByteArray &memory) {
        emit self->deviceScalarsReady(memory.left(0x40));
        emit self->activeMemoryReady(memory);
    });
    return preset;
}
