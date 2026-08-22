#include "K500PresetProtocol.h"

#include "K500Frame.h"

#include <QtGlobal>
#include <initializer_list>

namespace {
QByteArray bytes(std::initializer_list<int> values)
{
    QByteArray out;
    out.reserve(static_cast<qsizetype>(values.size()));
    for (const int value : values)
        out.append(char(value & 0xFF));
    return out;
}

quint8 routeMaskByte(quint8 mask)
{
    return static_cast<quint8>(mask & K500PresetProtocol::DeviceRouteAll);
}

void appendU16Le(QByteArray &out, quint16 value)
{
    out.append(char(value & 0xFF));
    out.append(char((value >> 8) & 0xFF));
}

bool validImage(const QByteArray &image)
{
    return image.size() == K500PresetProtocol::DeviceSlotImageLength;
}
}

namespace K500PresetProtocol {

QByteArray recallMode(int slotOneBased, quint8 routeMask)
{
    const int slotZeroBased = qBound(1, slotOneBased, 10) - 1;
    return K500Frame::build(bytes({0x03, 0x01, slotZeroBased, routeMaskByte(routeMask)}));
}

QByteArray recallHandshake(quint8 routeMask)
{
    return K500Frame::build(bytes({0x03, 0x3F, 0x00, routeMaskByte(routeMask)}));
}

QByteArray useInitVolume(bool enabled, quint8 routeMask)
{
    return K500Frame::build(bytes({0x03, 0x12, enabled ? 0x01 : 0x00, routeMaskByte(routeMask)}));
}

quint8 imageChecksum8(const QByteArray &image)
{
    quint8 sum = 0;
    for (const char ch : image)
        sum = static_cast<quint8>(sum + static_cast<quint8>(static_cast<unsigned char>(ch)));
    return static_cast<quint8>(0u - sum);
}

QByteArray storeBegin(const QByteArray &image, const K500StoreChain &chain)
{
    if (!validImage(image))
        return {};
    return K500Frame::build(bytes({
        0x08, 0x41, 0x90, 0x02, imageChecksum8(image), 0x00,
        chain.first, chain.second, chain.third,
    }));
}

QByteArray storeChunk(quint16 offset, const QByteArray &data)
{
    if (data.isEmpty() || data.size() > DeviceSlotWriteChunk)
        return {};

    const int bodyLength = 1 + 2 + 2 + data.size() + 4;
    QByteArray body;
    body.reserve(bodyLength + 1);
    body.append(char(bodyLength & 0xFF));
    body.append(char(0x42));
    appendU16Le(body, offset);
    appendU16Le(body, static_cast<quint16>(data.size()));
    body.append(data);
    body.append(QByteArray(4, char(0x00)));
    return K500Frame::build(body);
}

QByteArray storeCommit(int slotOneBased, const QByteArray &image)
{
    if (!validImage(image))
        return {};

    const int slotZeroBased = qBound(1, slotOneBased, 10) - 1;
    const int finalLength = DeviceSlotImageLength % DeviceSlotWriteChunk
        ? DeviceSlotImageLength % DeviceSlotWriteChunk : DeviceSlotWriteChunk;
    const int finalOffset = DeviceSlotImageLength - finalLength;

    return K500Frame::build(bytes({
        0x07, 0x43, slotZeroBased, 0x00,
        finalLength & 0xFF, (finalLength >> 8) & 0xFF,
        static_cast<quint8>(static_cast<unsigned char>(image.at(finalOffset))),
        static_cast<quint8>(static_cast<unsigned char>(image.at(finalOffset + 1))),
    }));
}

K500StoreChain nextStoreChain(const QByteArray &image, const QByteArray &commitFrame)
{
    K500StoreChain chain;
    if (!validImage(image) || commitFrame.isEmpty())
        return chain;

    const int finalLength = DeviceSlotImageLength % DeviceSlotWriteChunk
        ? DeviceSlotImageLength % DeviceSlotWriteChunk : DeviceSlotWriteChunk;
    const int finalOffset = DeviceSlotImageLength - finalLength;
    chain.first = static_cast<quint8>(static_cast<unsigned char>(image.at(finalOffset)));
    chain.second = static_cast<quint8>(static_cast<unsigned char>(image.at(finalOffset + 1)));
    chain.third = static_cast<quint8>(static_cast<unsigned char>(commitFrame.back()));
    return chain;
}

bool selfTest(QString *error)
{
    // P2_PRESET_PROTOCOL_GOLDEN_VECTORS_V1
    const auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    const auto expect = [&fail](const QByteArray &actual, std::initializer_list<int> expected,
                                const QString &label) {
        if (actual != bytes(expected))
            return fail(label + QStringLiteral(" frame mismatch"));
        return true;
    };

    if (!expect(recallMode(1), {0xAA,0x03,0x01,0x00,0x03,0xF9}, QStringLiteral("recall slot 1")))
        return false;
    if (!expect(K500Frame::toUsbFrame(recallMode(4, DeviceRouteUsb)),
                {0xAA,0x03,0x00,0x01,0x03,0x01,0xF8}, QStringLiteral("USB recall slot 4")))
        return false;
    if (!expect(recallHandshake(), {0xAA,0x03,0x3F,0x00,0x03,0xBB}, QStringLiteral("recall handshake")))
        return false;
    if (!expect(useInitVolume(false), {0xAA,0x03,0x12,0x00,0x03,0xE8}, QStringLiteral("use init off")))
        return false;
    if (!expect(useInitVolume(true), {0xAA,0x03,0x12,0x01,0x03,0xE7}, QStringLiteral("use init on")))
        return false;

    QByteArray zeroImage(DeviceSlotImageLength, char(0));
    if (!expect(storeBegin(zeroImage),
                {0xAA,0x08,0x41,0x90,0x02,0x00,0x00,0x00,0x00,0x00,0x25},
                QStringLiteral("store begin zero image")))
        return false;
    if (storeChunk(0, {}).size() != 0 || storeChunk(0, QByteArray(DeviceSlotWriteChunk + 1, char(0))).size() != 0)
        return fail(QStringLiteral("invalid store chunk length must be rejected"));

    const QByteArray firstChunk = storeChunk(0, QByteArray(DeviceSlotWriteChunk, char(0)));
    if (firstChunk.size() != 72 || static_cast<quint8>(firstChunk.at(1)) != 0x45
        || static_cast<quint8>(firstChunk.back()) != 0x3D)
        return fail(QStringLiteral("60-byte store chunk framing mismatch"));

    const QByteArray finalChunk = storeChunk(0x0258, QByteArray(0x38, char(0)));
    if (finalChunk.size() != 68 || static_cast<quint8>(finalChunk.at(1)) != 0x41
        || static_cast<quint8>(finalChunk.back()) != 0xEB)
        return fail(QStringLiteral("final 56-byte store chunk framing mismatch"));

    if (!expect(storeCommit(1, zeroImage),
                {0xAA,0x07,0x43,0x00,0x00,0x38,0x00,0x00,0x00,0x7E},
                QStringLiteral("store commit slot 1")))
        return false;

    QByteArray customImage(DeviceSlotImageLength, char(0));
    customImage[0x0258] = char(0x12);
    customImage[0x0259] = char(0x34);
    const K500StoreChain inputChain{0x12, 0x34, 0x56};
    if (!expect(storeBegin(customImage, inputChain),
                {0xAA,0x08,0x41,0x90,0x02,0xBA,0x00,0x12,0x34,0x56,0xCF},
                QStringLiteral("store begin chained")))
        return false;
    const QByteArray commit = storeCommit(1, customImage);
    if (!expect(commit,
                {0xAA,0x07,0x43,0x00,0x00,0x38,0x00,0x12,0x34,0x38},
                QStringLiteral("store commit carries final image bytes")))
        return false;
    const K500StoreChain outputChain = nextStoreChain(customImage, commit);
    if (outputChain.first != 0x12 || outputChain.second != 0x34 || outputChain.third != 0x38)
        return fail(QStringLiteral("mass-upload chain derivation mismatch"));

    if (!storeBegin(QByteArray(DeviceSlotImageLength - 1, char(0))).isEmpty())
        return fail(QStringLiteral("short slot image must be rejected"));
    if (!storeCommit(1, QByteArray(DeviceSlotImageLength + 1, char(0))).isEmpty())
        return fail(QStringLiteral("long slot image must be rejected"));

    return true;
}

} // namespace K500PresetProtocol
