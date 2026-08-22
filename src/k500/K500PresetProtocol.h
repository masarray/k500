#pragma once

#include <QByteArray>
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

QByteArray recallMode(int slotOneBased, quint8 routeMask = DeviceRouteAll);
QByteArray recallHandshake(quint8 routeMask = DeviceRouteAll);
QByteArray useInitVolume(bool enabled, quint8 routeMask = DeviceRouteAll);

quint8 imageChecksum8(const QByteArray &image);
QByteArray storeBegin(const QByteArray &image, const K500StoreChain &chain = {});
QByteArray storeChunk(quint16 offset, const QByteArray &data);
QByteArray storeCommit(int slotOneBased, const QByteArray &image);
K500StoreChain nextStoreChain(const QByteArray &image, const QByteArray &commitFrame);

bool selfTest(QString *error = nullptr);

} // namespace K500PresetProtocol
