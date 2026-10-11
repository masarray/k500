#pragma once

#include <QByteArray>
#include <array>
#include <QString>

struct K500StoreChain
{
    quint8 first = 0x00;
    quint8 second = 0x00;
    quint8 third = 0x00;
};

namespace K500PresetProtocol {

constexpr quint8 DeviceRouteUsb = 0x01;
constexpr quint8 DeviceRouteBt = 0x02;
constexpr quint8 DeviceRouteAll = DeviceRouteUsb | DeviceRouteBt;
constexpr int DeviceSlotImageLength = 0x0290;
constexpr int DeviceSlotWriteChunk = 0x003C;
constexpr int DeviceSlotModeNameOffset = 0x0280;
constexpr int DeviceSlotModeNameLength = 0x0010;

QByteArray recallMode(int slotOneBased, quint8 routeMask = DeviceRouteAll);
QByteArray recallHandshake(quint8 routeMask = DeviceRouteAll);
QByteArray useInitVolume(bool enabled, quint8 routeMask = DeviceRouteAll);

quint8 imageChecksum8(const QByteArray &image);
QByteArray withModeName(const QByteArray &image, const QString &name, QString *error = nullptr);
// NATIVE_STORE_STAGED_VOLUME_PARITY_V1 — only six captured byte positions
// inside the original 0x0290 device image may be overridden. Mask bits:
// top Music/Mic/FX = 0/1/2, Init Music/Mic/FX = 3/4/5.
// Caller must supply explicit, session-valid LIVE intent, not offline defaults.
QByteArray withStartupVolumeOverrides(const QByteArray &image,
                                      const std::array<int, 6> &values,
                                      quint8 mask, QString *error = nullptr);
QByteArray storeBegin(const QByteArray &image, const K500StoreChain &chain = {});
QByteArray storeChunk(quint16 offset, const QByteArray &data);
QByteArray storeCommit(int slotOneBased, const QByteArray &image);
K500StoreChain nextStoreChain(const QByteArray &image, const QByteArray &commitFrame);

bool selfTest(QString *error = nullptr);

} // namespace K500PresetProtocol
