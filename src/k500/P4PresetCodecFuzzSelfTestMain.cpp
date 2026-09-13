#include "K500PresetCodec.h"

#include <QByteArray>
#include <QDebug>
#include <QSet>
#include <QString>
#include <QVector>

#include <algorithm>

namespace {
constexpr int FuzzIterations = 40000;
constexpr quint32 InitialSeed = 0x50344B35u; // "P4K5"

class XorShift32 final
{
public:
    explicit XorShift32(quint32 seed) : m_state(seed ? seed : 1u) {}

    quint32 next()
    {
        quint32 x = m_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        m_state = x;
        return x;
    }

    int bounded(int exclusiveUpper)
    {
        return exclusiveUpper > 0
            ? static_cast<int>(next() % static_cast<quint32>(exclusiveUpper))
            : 0;
    }

private:
    quint32 m_state;
};

QByteArray randomBytes(XorShift32 &rng, int size)
{
    QByteArray out(size, Qt::Uninitialized);
    for (int i = 0; i < size; ++i)
        out[i] = static_cast<char>(rng.next() & 0xffu);
    return out;
}

bool validateSuccessfulPatch(const QByteArray &source,
                             const K500PresetCodec::PatchResult &result,
                             const QSet<int> &allowed,
                             bool recomputeChecksum,
                             QString *error)
{
    if (!result.ok)
        return true;
    if (result.bytes.size() != K500PresetCodec::PresetFileLength) {
        if (error) *error = QStringLiteral("successful patch returned wrong output size");
        return false;
    }

    QSet<int> effective = allowed;
    if (recomputeChecksum && result.bytes != source)
        effective.insert(K500PresetCodec::ChecksumOffset);

    for (const int offset : result.changedOffsets) {
        if (offset < 0 || offset >= result.bytes.size() || !effective.contains(offset)) {
            if (error) *error = QStringLiteral("successful patch reported non-whitelisted offset %1").arg(offset);
            return false;
        }
    }

    if (recomputeChecksum && result.bytes != source
        && !K500PresetCodec::validateChecksum(result.bytes)) {
        if (error) *error = QStringLiteral("successful recomputed patch has invalid checksum");
        return false;
    }
    return true;
}

bool deterministicPresetFuzz(QString *error)
{
    XorShift32 rng(InitialSeed);

    for (int iteration = 0; iteration < FuzzIterations; ++iteration) {
        int size = rng.bounded(1601);
        if ((iteration % 7) == 0)
            size = K500PresetCodec::PresetFileLength;
        else if ((iteration % 11) == 0)
            size = K500PresetCodec::PresetFileLength - 1;
        else if ((iteration % 13) == 0)
            size = K500PresetCodec::PresetFileLength + 1;

        QByteArray source = randomBytes(rng, size);
        K500PresetCodec::Document doc(source);

        // P4_PRESET_CODEC_FUZZ_V1
        // Exercise all public read paths using adversarial offsets and lengths.
        const bool validSize = doc.validSize();
        (void)doc.checksumOk();
        (void)doc.checksumByte();
        (void)doc.name();
        (void)doc.u8(-1 - rng.bounded(64));
        (void)doc.u8(size + rng.bounded(64));
        (void)doc.u16(-1 - rng.bounded(64));
        (void)doc.u16(size + rng.bounded(64));
        (void)doc.i16(-1 - rng.bounded(64));
        (void)doc.i16(size + rng.bounded(64));
        const auto sections = doc.eqSections();
        if (!validSize && !sections.isEmpty()) {
            if (error) *error = QStringLiteral("invalid-size document exposed EQ sections at iteration %1").arg(iteration);
            return false;
        }
        if (doc.serializeNoop() != source) {
            if (error) *error = QStringLiteral("no-op serialization changed bytes at iteration %1").arg(iteration);
            return false;
        }

        QString slotError;
        const QByteArray slot = K500PresetCodec::buildDeviceSlotImage(source, &slotError);
        if (validSize) {
            if (slot.size() != K500PresetCodec::DeviceSlotImageLength) {
                if (error) *error = QStringLiteral("valid preset did not produce 0x0290 slot image at iteration %1").arg(iteration);
                return false;
            }
        } else if (!slot.isEmpty() || slotError.isEmpty()) {
            if (error) *error = QStringLiteral("invalid preset size was accepted by slot conversion at iteration %1").arg(iteration);
            return false;
        }

        const QByteArray checksummed = K500PresetCodec::updateChecksum(source);
        if (validSize) {
            if (checksummed.size() != K500PresetCodec::PresetFileLength
                || !K500PresetCodec::validateChecksum(checksummed)) {
                if (error) *error = QStringLiteral("checksum normalization failed at iteration %1").arg(iteration);
                return false;
            }
        } else if (!checksummed.isEmpty()) {
            if (error) *error = QStringLiteral("checksum accepted invalid-size input at iteration %1").arg(iteration);
            return false;
        }

        QVector<K500PresetCodec::BytePatch> patches;
        QSet<int> allowed;
        const int patchCount = rng.bounded(7);
        for (int p = 0; p < patchCount; ++p) {
            int offset;
            switch ((iteration + p) % 5) {
            case 0: offset = -1 - rng.bounded(32); break;
            case 1: offset = K500PresetCodec::PresetFileLength + rng.bounded(64); break;
            default: offset = rng.bounded(K500PresetCodec::PresetFileLength + 1); break;
            }
            const QByteArray bytes = randomBytes(rng, rng.bounded(17));
            patches.push_back({offset, bytes});
            for (int i = 0; i < bytes.size(); ++i) {
                if ((rng.next() & 1u) != 0u)
                    allowed.insert(offset + i);
            }
        }

        const bool recompute = (rng.next() & 1u) != 0u;
        const auto result = K500PresetCodec::applyWhitelistedPatches(source, patches, allowed, recompute);
        if (!validateSuccessfulPatch(source, result, allowed, recompute, error)) {
            if (error) *error = QStringLiteral("iteration %1: %2").arg(iteration).arg(*error);
            return false;
        }
    }

    return true;
}
}

int main()
{
    QString error;
    if (!deterministicPresetFuzz(&error)) {
        qCritical().noquote() << "P4 preset codec fuzz failed:" << error;
        return 2;
    }

    qInfo() << "P4 preset codec fuzz passed" << FuzzIterations << "iterations";
    return 0;
}
