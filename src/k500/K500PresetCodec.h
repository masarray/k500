#pragma once

#include <QByteArray>
#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace K500PresetCodec {

constexpr int PresetFileLength = 0x0478;
constexpr int ChecksumOffset = 0x0475;
constexpr int NameOffset = 0x0454;
constexpr int NameLength = 0x21;
constexpr int DeviceSlotImageLength = 0x0290;
constexpr int LiveScalarEnd = 0x00e7;
constexpr int LiveScalarSplit = 0x008f;
constexpr int LiveScalarDeltaLow = 0x08;
constexpr int LiveScalarDeltaHigh = 0x09;

struct EqBand {
    quint16 typeRaw = 0;
    quint16 frequencyHz = 0;
    quint16 qRaw = 0;
    qint16 gainRaw = 0;
};

struct Crossover {
    quint16 lpTypeRaw = 0;
    quint16 lpfHz = 0;
    QByteArray reserved4;
    quint16 hpTypeRaw = 0;
    quint16 hpfHz = 0;
};

struct EqSection {
    QString key;
    int fileOffset = 0;
    int liveOffset = 0;
    int bandCount = 0;
    quint16 enabledFlag = 0;
    QVector<EqBand> bands;
    Crossover crossover;
};

struct BytePatch {
    int offset = 0;
    QByteArray bytes;
};

struct PatchResult {
    bool ok = false;
    QString error;
    QByteArray bytes;
    QVector<int> changedOffsets;
};

class Document {
public:
    explicit Document(QByteArray bytes = {});

    bool validSize() const;
    bool checksumOk() const;
    quint8 checksumByte() const;
    QString name() const;
    const QByteArray &bytes() const;

    quint8 u8(int offset) const;
    quint16 u16(int offset) const;
    qint16 i16(int offset) const;
    QVector<EqSection> eqSections() const;

    // Deliberately returns the exact original bytes. A no-op parse/serialize
    // cycle must never normalize aliases, padding, reserved bytes or mirrors.
    QByteArray serializeNoop() const;

private:
    QByteArray m_bytes;
};

quint8 additiveChecksum(const QByteArray &bytes);
bool validateChecksum(const QByteArray &bytes);
QByteArray updateChecksum(QByteArray bytes);

PatchResult applyWhitelistedPatches(const QByteArray &source,
                                    const QVector<BytePatch> &patches,
                                    const QSet<int> &allowedOffsets,
                                    bool recomputeChecksum = true);

// Convert a complete .k500 file into the native 0x0290 equipment-slot image.
// This is NOT source.left(0x0290): scalar offsets use the verified +8/+9 split
// and 8-byte file EQ bands are compacted into native 5-byte live records.
QByteArray buildDeviceSlotImage(const QByteArray &presetFile, QString *error = nullptr);

} // namespace K500PresetCodec
