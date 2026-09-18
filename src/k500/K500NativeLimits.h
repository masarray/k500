#pragma once

// K500_NATIVE_UI_LIMITS_V1
//
// Authoritative host-side control domains with explicit evidence provenance.
// Reverb endpoints below are verified against the physical native KTV
// "Professional Audio System" V3.10 UI. Older domains recovered from Git
// history are included only where their original donor/capture evidence is
// identifiable in docs/K500_NATIVE_CONTROL_LIMITS.md.
//
// IMPORTANT:
// - Wire width is NOT a UI-domain specification. A 16-bit protocol field does
//   not mean the native control is allowed to span 0..65535.
// - Generic DSP ranges (for example 20..20000 Hz) must not be reused for an
//   effect control unless native endpoint evidence proves that range.
// - Add new limits only when an endpoint capture / native UI observation is
//   recorded in docs/K500_NATIVE_CONTROL_LIMITS.md.

namespace K500NativeLimits {

// DONOR-WEB evidence: commit 8524101578b18e96ffb5a8381b83036e37260718
// intentionally changed all three top/master faders from 0..100 to 0..84.
namespace TopVolume {
inline constexpr int Min = 0;
inline constexpr int Max = 84;
} // namespace TopVolume

// DONOR-WEB evidence: commit a61b4ba3c3580d785f14123391de6d441ae71898
// intentionally changed all five physical music gain domains to -12..+12 dB.
namespace MusicInputGain {
inline constexpr double MinDb = -12.0;
inline constexpr double MaxDb = 12.0;
} // namespace MusicInputGain

// CAPTURED-NATIVE evidence: commit 6ca67ff22c7dd706f8c1bc53f59c81189ade7487
// records the FBE/FBX level progression 3 -> 2 -> 1 -> 0.
namespace MicFbx {
inline constexpr int Min = 0;
inline constexpr int Max = 3;
} // namespace MicFbx

namespace Reverb {
inline constexpr int LevelMin = 0;
inline constexpr int LevelMax = 100;

inline constexpr int DirectMin = 0;
inline constexpr int DirectMax = 100;

inline constexpr int DecayMinMs = 500;
inline constexpr int DecayMaxMs = 5000;

inline constexpr int PredelayMinMs = 0;
inline constexpr int PredelayMaxMs = 100;

inline constexpr int HpfMinHz = 20;
inline constexpr int HpfMaxHz = 1000;

inline constexpr int LpfMinHz = 4000;
inline constexpr int LpfMaxHz = 16000;
} // namespace Reverb

namespace ReverbEq {
inline constexpr double GainMinDb = -24.0;
inline constexpr double GainMaxDb = 24.0;
} // namespace ReverbEq

} // namespace K500NativeLimits
