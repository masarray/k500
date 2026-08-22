#include "K500PresetCodec.h"

#include <QtEndian>
#include <algorithm>
#include <iterator>

namespace K500PresetCodec {
namespace {
struct EqDescriptor { const char *key; int fileOffset; int liveOffset; int bands; };
constexpr EqDescriptor EqMap[] = {
    {"micA",        0x00f0, 0x00e7, 10},
    {"micB",        0x0150, 0x0119, 10},
    {"music",       0x01b0, 0x014b, 7},
    {"main",        0x01f8, 0x016e, 7},
    {"mainAlt",     0x0240, 0x0191, 7},
    {"surround",    0x0288, 0x01b4, 5},
    {"surroundAlt", 0x02c0, 0x01cd, 5},
    {"center",      0x02f8, 0x01e6, 5},
    {"centerAlt",   0x0330, 0x01ff, 5},
    {"sub",         0x0368, 0x0218, 5},
    {"subAlt",      0x03a0, 0x0231, 5},
    {"reverb",      0x03d8, 0x024a, 5},
    {"echo",        0x0410, 0x0263, 5},
};

bool inRange(const QByteArray &bytes, int offset, int length = 1)
{
    return offset >= 0 && length >= 0 && offset + length <= bytes.size();
}

quint16 readU16(const QByteArray &bytes, int offset)
{
    if (!inRange(bytes, offset, 2)) return 0;
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(bytes.constData() + offset));
}

qint16 readI16(const QByteArray &bytes, int offset)
{
    return static_cast<qint16>(readU16(bytes, offset));
}

quint8 compactTypeNibble(quint16 typeRaw)
{
    if (typeRaw == 0x0100) return 0x10;
    if (typeRaw == 0x0200) return 0x20;
    return 0x00;
}
}

Document::Document(QByteArray bytes) : m_bytes(std::move(bytes)) {}

bool Document::validSize() const { return m_bytes.size() == PresetFileLength; }
bool Document::checksumOk() const { return validSize() && validateChecksum(m_bytes); }
quint8 Document::checksumByte() const { return u8(ChecksumOffset); }
const QByteArray &Document::bytes() const { return m_bytes; }
QByteArray Document::serializeNoop() const { return m_bytes; }

QString Document::name() const
{
    if (!inRange(m_bytes, NameOffset, NameLength)) return {};
    QByteArray raw = m_bytes.mid(NameOffset, NameLength);
    const int nul = raw.indexOf('\0');
    if (nul >= 0) raw.truncate(nul);
    while (!raw.isEmpty() && raw.endsWith(' ')) raw.chop(1);
    return QString::fromLatin1(raw);
}

quint8 Document::u8(int offset) const
{
    return inRange(m_bytes, offset) ? static_cast<quint8>(m_bytes.at(offset)) : 0;
}

quint16 Document::u16(int offset) const { return readU16(m_bytes, offset); }
qint16 Document::i16(int offset) const { return readI16(m_bytes, offset); }

QVector<EqSection> Document::eqSections() const
{
    QVector<EqSection> out;
    if (!validSize()) return out;
    out.reserve(static_cast<qsizetype>(std::size(EqMap)));
    for (const auto &d : EqMap) {
        EqSection s;
        s.key = QString::fromLatin1(d.key);
        s.fileOffset = d.fileOffset;
        s.liveOffset = d.liveOffset;
        s.bandCount = d.bands;
        s.enabledFlag = readU16(m_bytes, d.fileOffset);
        s.bands.reserve(d.bands);
        for (int i = 0; i < d.bands; ++i) {
            const int p = d.fileOffset + 2 + i * 8;
            s.bands.push_back(EqBand{readU16(m_bytes, p), readU16(m_bytes, p + 2),
                                     readU16(m_bytes, p + 4), readI16(m_bytes, p + 6)});
        }
        const int footer = d.fileOffset + 2 + d.bands * 8;
        s.crossover.lpTypeRaw = readU16(m_bytes, footer);
        s.crossover.lpfHz = readU16(m_bytes, footer + 2);
        s.crossover.reserved4 = m_bytes.mid(footer + 4, 4);
        s.crossover.hpTypeRaw = readU16(m_bytes, footer + 8);
        s.crossover.hpfHz = readU16(m_bytes, footer + 10);
        out.push_back(std::move(s));
    }
    return out;
}

quint8 additiveChecksum(const QByteArray &bytes)
{
    quint8 sum = 0;
    for (const char c : bytes) sum = static_cast<quint8>(sum + static_cast<quint8>(c));
    return sum;
}

bool validateChecksum(const QByteArray &bytes)
{
    return bytes.size() == PresetFileLength && additiveChecksum(bytes) == 0;
}

QByteArray updateChecksum(QByteArray bytes)
{
    if (bytes.size() != PresetFileLength) return {};
    bytes[ChecksumOffset] = 0;
    const quint8 sum = additiveChecksum(bytes);
    bytes[ChecksumOffset] = static_cast<char>(static_cast<quint8>(0u - sum));
    return bytes;
}

PatchResult applyWhitelistedPatches(const QByteArray &source,
                                    const QVector<BytePatch> &patches,
                                    const QSet<int> &allowedOffsets,
                                    bool recomputeChecksum)
{
    PatchResult r;
    if (source.size() != PresetFileLength) {
        r.error = QStringLiteral("Expected 0x0478-byte .k500 file, got %1 bytes.").arg(source.size());
        return r;
    }
    QByteArray out = source;
    for (const auto &patch : patches) {
        if (!inRange(out, patch.offset, patch.bytes.size())) {
            r.error = QStringLiteral("Patch at 0x%1 is outside the preset file.").arg(patch.offset, 4, 16, QLatin1Char('0'));
            return r;
        }
        for (int i = 0; i < patch.bytes.size(); ++i) {
            const int offset = patch.offset + i;
            if (!allowedOffsets.contains(offset)) {
                r.error = QStringLiteral("Offset 0x%1 is not in the explicit patch whitelist.").arg(offset, 4, 16, QLatin1Char('0'));
                return r;
            }
            out[offset] = patch.bytes.at(i);
        }
    }
    if (recomputeChecksum && out != source) out = updateChecksum(std::move(out));
    for (int i = 0; i < source.size(); ++i)
        if (source.at(i) != out.at(i)) r.changedOffsets.push_back(i);

    QSet<int> effective = allowedOffsets;
    if (recomputeChecksum && out != source) effective.insert(ChecksumOffset);
    for (const int offset : r.changedOffsets) {
        if (!effective.contains(offset)) {
            r.error = QStringLiteral("Unexpected changed offset 0x%1 after serialization.").arg(offset, 4, 16, QLatin1Char('0'));
            r.bytes.clear();
            r.changedOffsets.clear();
            return r;
        }
    }
    r.ok = true;
    r.bytes = std::move(out);
    return r;
}

QByteArray buildDeviceSlotImage(const QByteArray &presetFile, QString *error)
{
    if (presetFile.size() != PresetFileLength) {
        if (error) *error = QStringLiteral("Device slot conversion requires a 0x0478-byte .k500 file.");
        return {};
    }
    QByteArray live(DeviceSlotImageLength, char(0));
    for (int i = 0; i < LiveScalarEnd; ++i) {
        const int delta = i < LiveScalarSplit ? LiveScalarDeltaLow : LiveScalarDeltaHigh;
        live[i] = presetFile.at(i + delta);
    }

    for (const auto &d : EqMap) {
        for (int i = 0; i < d.bands; ++i) {
            const int src = d.fileOffset + 2 + i * 8;
            const int dst = d.liveOffset + i * 5;
            const quint16 typeRaw = readU16(presetFile, src);
            const quint16 freq = readU16(presetFile, src + 2);
            const quint16 qRaw = readU16(presetFile, src + 4);
            const qint16 gainRaw = readI16(presetFile, src + 6);
            live[dst] = static_cast<char>(freq & 0xff);
            live[dst + 1] = static_cast<char>((freq >> 8) & 0xff);
            live[dst + 2] = static_cast<char>(std::clamp<int>(qRaw, 1, 0xff));
            live[dst + 3] = static_cast<char>(compactTypeNibble(typeRaw) | (gainRaw < 0 ? 0x80 : 0x00));
            const int magnitude = std::min<int>(std::abs(static_cast<int>(gainRaw)), 0xff);
            live[dst + 4] = static_cast<char>(magnitude);
        }
    }

    live.replace(0x027c, 4, presetFile.mid(0x044c, 4));
    live.replace(0x0280, 0x10, presetFile.mid(NameOffset, 0x10));
    return live;
}

} // namespace K500PresetCodec
