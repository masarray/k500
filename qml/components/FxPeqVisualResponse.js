.pragma library
// FX_PEQ_NATIVE_SCREENSHOT_PARITY_V1 — optical chart approximation only.
// Match src/FxPeqVisualResponse.h, never transmit a filter type or DSP value.
function warp(hz) {
    var f = Math.max(1, Math.min(23999, Number(hz)))
    return Math.tan(Math.PI * f / 48000)
}
function cutoffDb(mode, hp, lp, frequency) {
    if (mode !== 1 && mode !== 2) return 0
    var f = warp(frequency)
    var hpRatio = warp(hp) / f
    var lpRatio = f / warp(lp)
    return -10 * Math.log(1 + Math.pow(hpRatio, 4)) / Math.LN10
           -10 * Math.log(1 + Math.pow(lpRatio, mode === 1 ? 8 : 4)) / Math.LN10
}
