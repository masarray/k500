#pragma once

// K500_NATIVE_UI_LIMITS_V1
//
// Authoritative host-side limits that have been verified against the native
// KTV "Professional Audio System" V3.10 UI on physical hardware.
//
// IMPORTANT:
// - Wire width is NOT a UI-domain specification. A 16-bit protocol field does
//   not mean the native control is allowed to span 0..65535.
// - Generic DSP ranges (for example 20..20000 Hz) must not be reused for an
//   effect control unless native endpoint evidence proves that range.
// - Add new limits only when an endpoint capture / native UI observation is
//   recorded in docs/K500_NATIVE_CONTROL_LIMITS.md.

namespace K500NativeLimits {

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

namespace Eq {
inline constexpr double GainMinDb = -24.0;
inline constexpr double GainMaxDb = 24.0;
} // namespace Eq

} // namespace K500NativeLimits
