#pragma once

#include <algorithm>
#include <cmath>

// FX_PEQ_NATIVE_SCREENSHOT_PARITY_V1
// Graph-only perceptual calibration against original manufacturer's Reverb
// and Echo UI (provided comparison screenshots, 2026-10-13). The device's
// actual DSP transfer function has NOT been measured or identified. Do not
// use this approximation to alter I/O, saved preset, or DSP/hardware values.
// Mode 0 = normal editable HP/LP type, 1 = native Reverb, 2 = native Echo.
namespace FxPeqVisualResponse {
constexpr double pi = 3.1415926535897932384626433832795;
constexpr double sampleRateHz = 48000.0;

inline double warp(double hz) noexcept
{
    return std::tan(pi * std::clamp(hz, 1.0, sampleRateHz / 2.0 - 1.0) / sampleRateHz);
}

inline double cutoffDb(int mode, double hpfHz, double lpfHz, double frequencyHz) noexcept
{
    if (mode != 1 && mode != 2)
        return 0.0;
    const double f = warp(frequencyHz);
    const double hpRatio = warp(hpfHz) / f;
    const double lpRatio = f / warp(lpfHz);
    // Native screenshot parity: 12 dB/oct visual HPF on both FX pages;
    // Reverb LPF requires the markedly faster digital roll-off observed at
    // ~13.2-20 kHz (24 dB/oct), Echo LPF matches a 12 dB/oct digital roll-off.
    // These are visual reference curves, NOT claims of captured DSP topology.
    const double highPass = -10.0 * std::log10(1.0 + std::pow(hpRatio, 4.0));
    const double lowPass = -10.0 * std::log10(1.0 + std::pow(lpRatio, mode == 1 ? 8.0 : 4.0));
    return highPass + lowPass;
}
} // namespace FxPeqVisualResponse
