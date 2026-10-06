#pragma once

#include <array>

namespace K500FieldContract {

constexpr int InvalidOffset = -1;

enum class EvidenceLevel : unsigned char {
    Structural,
    Captured,
    ProvenRoundTrip,
};

enum class FileMutationPolicy : unsigned char {
    PreserveOnly,
    Writable,
};

enum class FieldId : unsigned char {
    MicHpType,
    MicLpType,
    MicFbe,
    AdjMannerVrOff,
};

namespace ScalarGeometry {
constexpr int FileLowBegin = 0x0008;
constexpr int FileLowEnd = 0x0096;
constexpr int FileHighBegin = 0x0098;
constexpr int FileHighEnd = 0x00EF;
constexpr int ActiveSplit = 0x008F;
constexpr int ActiveEndExclusive = 0x00E7;
constexpr int LowDelta = 0x08;
constexpr int HighDelta = 0x09;

constexpr bool isFileScalarOffset(int fileOffset) noexcept
{
    return (fileOffset >= FileLowBegin && fileOffset <= FileLowEnd)
        || (fileOffset >= FileHighBegin && fileOffset <= FileHighEnd);
}

constexpr bool isActiveScalarOffset(int activeOffset) noexcept
{
    return activeOffset >= 0 && activeOffset < ActiveEndExclusive;
}

constexpr int activeOffsetForFileScalar(int fileOffset) noexcept
{
    if (fileOffset >= FileLowBegin && fileOffset <= FileLowEnd)
        return fileOffset - LowDelta;
    if (fileOffset >= FileHighBegin && fileOffset <= FileHighEnd)
        return fileOffset - HighDelta;
    return InvalidOffset;
}

constexpr int fileOffsetForActiveScalar(int activeOffset) noexcept
{
    if (!isActiveScalarOffset(activeOffset)) return InvalidOffset;
    return activeOffset < ActiveSplit ? activeOffset + LowDelta
                                      : activeOffset + HighDelta;
}

static_assert(activeOffsetForFileScalar(FileLowBegin) == 0x0000);
static_assert(activeOffsetForFileScalar(FileLowEnd) == 0x008E);
static_assert(activeOffsetForFileScalar(0x0097) == InvalidOffset);
static_assert(activeOffsetForFileScalar(FileHighBegin) == 0x008F);
static_assert(activeOffsetForFileScalar(FileHighEnd) == 0x00E6);
static_assert(fileOffsetForActiveScalar(0x008E) == 0x0096);
static_assert(fileOffsetForActiveScalar(0x008F) == 0x0098);
} // namespace ScalarGeometry

namespace FileOffset {
constexpr int TopMusicVol = 0x0008;
constexpr int TopMicVol = 0x0009;
constexpr int TopEffectVol = 0x000A;
constexpr int MusicInitVol = 0x000B;
constexpr int MusicMaxVol = 0x000C;
constexpr int MusicSource = 0x000E;
constexpr int MusicKey = 0x0011;
constexpr int MicInitVol = 0x0012;
constexpr int MicMaxVol = 0x0013;
constexpr int MicAVol = 0x0014;
constexpr int MicBVol = 0x0015;
constexpr int MicNoiseGate = 0x0016;
constexpr int MicCompThreshold = 0x0017;
constexpr int MicCompRatio = 0x0018;
constexpr int MicAttack = 0x0019;
constexpr int MicRelease = 0x001A;
constexpr int MicHpType = 0x001B;
constexpr int MicLpType = 0x001C;
constexpr int EffectInitLevel = 0x001D;
constexpr int MusicInput1Gain = 0x001E;
constexpr int MusicInput2Gain = 0x001F;
constexpr int MusicBluetoothGain = 0x0020;
constexpr int MusicUDiskGain = 0x0021;
constexpr int MusicDigitalGain = 0x0022;
constexpr int MicFbe = 0x0023;
constexpr int MicEqLink = 0x0092;
constexpr int AdjMannerVrOff = 0x0094;
constexpr int UDiskRecordVol = 0x0095;
constexpr int UsbRecordVol = 0x0096;
} // namespace FileOffset

struct ScalarFieldSpec {
    FieldId id;
    int fileOffset;
    int activeOffset;
    int rawMin;
    int rawMax;
    EvidenceLevel evidence;
    FileMutationPolicy fileMutation;
};

namespace Field {
constexpr ScalarFieldSpec MicHpType{
    FieldId::MicHpType, FileOffset::MicHpType,
    ScalarGeometry::activeOffsetForFileScalar(FileOffset::MicHpType),
    0, 7, EvidenceLevel::Captured, FileMutationPolicy::PreserveOnly};
constexpr ScalarFieldSpec MicLpType{
    FieldId::MicLpType, FileOffset::MicLpType,
    ScalarGeometry::activeOffsetForFileScalar(FileOffset::MicLpType),
    0, 7, EvidenceLevel::Captured, FileMutationPolicy::PreserveOnly};
constexpr ScalarFieldSpec MicFbe{
    FieldId::MicFbe, FileOffset::MicFbe,
    ScalarGeometry::activeOffsetForFileScalar(FileOffset::MicFbe),
    0, 4, EvidenceLevel::ProvenRoundTrip, FileMutationPolicy::Writable};
constexpr ScalarFieldSpec AdjMannerVrOff{
    FieldId::AdjMannerVrOff, FileOffset::AdjMannerVrOff,
    ScalarGeometry::activeOffsetForFileScalar(FileOffset::AdjMannerVrOff),
    0, 1, EvidenceLevel::ProvenRoundTrip, FileMutationPolicy::Writable};
} // namespace Field

constexpr std::array<ScalarFieldSpec, 4> EvidenceBackedScalars{
    Field::MicHpType, Field::MicLpType, Field::MicFbe, Field::AdjMannerVrOff};

constexpr bool rawValueValid(const ScalarFieldSpec &field, int raw) noexcept
{
    return raw >= field.rawMin && raw <= field.rawMax;
}

constexpr bool geometryMatches(const ScalarFieldSpec &field) noexcept
{
    return ScalarGeometry::activeOffsetForFileScalar(field.fileOffset) == field.activeOffset
        && ScalarGeometry::fileOffsetForActiveScalar(field.activeOffset) == field.fileOffset;
}

static_assert(Field::MicHpType.activeOffset == 0x0013);
static_assert(Field::MicLpType.activeOffset == 0x0014);
static_assert(Field::MicFbe.activeOffset == 0x001B);
static_assert(Field::AdjMannerVrOff.activeOffset == 0x008C);
static_assert(geometryMatches(Field::MicHpType));
static_assert(geometryMatches(Field::MicLpType));
static_assert(geometryMatches(Field::MicFbe));
static_assert(geometryMatches(Field::AdjMannerVrOff));

} // namespace K500FieldContract
