# K500 Native Control Limits

This document is the repository source-of-truth for **native UI endpoint limits** that have been physically verified. It exists to prevent generic DSP ranges or protocol field widths from being mistaken for the limits exposed by the original K500/KTV application.

## Evidence policy

A value is marked **VERIFIED** only when its minimum/maximum endpoint is visible in the native KTV application or is proven by an equivalent physical-device capture. Ordinary preset values, protocol examples, or the storage width of a field are **not** sufficient evidence of a UI range.

When new evidence is collected, update this document and `src/k500/K500NativeLimits.h` together.

## Reverb — native KTV V3.10

Physical native-app endpoint screenshots captured during P4 hardware validation on 2026-09-18 establish the following limits:

| Control | Native minimum | Native maximum | Status |
| --- | ---: | ---: | --- |
| Reverb Level | 0 | 100 | VERIFIED |
| Reverb Decay | 500 ms | 5000 ms | VERIFIED |
| Reverb Predelay | 0 ms | 100 ms | VERIFIED |
| Reverb Highpass | 20 Hz | 1000 Hz | VERIFIED |
| Reverb Lowpass | 4000 Hz | 16000 Hz | VERIFIED |
| Reverb Direct | 0 | 100 | VERIFIED |
| Reverb PEQ gain | -24 dB | +24 dB | VERIFIED |

The native Reverb panel shows numeric Highpass/Lowpass controls but no selectable HP/LP filter-type dropdown. SonKuPik must therefore not expose a writable Reverb filter-type selector unless a separate native capture proves such a control and its wire mapping.

### What this corrects

Before this evidence was recorded, SonKuPik used several generic/assumed limits:

- Reverb Decay: `100..5000 ms` — **wrong minimum**.
- Reverb Predelay: `0..300 ms` — **wrong maximum**.
- Reverb HPF/LPF: generic `20..20000 Hz` — **wrong native domains**.
- Protocol/controller paths accepted much wider 16-bit values — **wire capacity incorrectly treated as UI range**.

These values must now be clamped consistently in UI, controller state, and protocol encoding.

## PEQ fields not yet endpoint-qualified

The screenshots above prove Reverb PEQ **gain** endpoints only. They do not establish authoritative minimum/maximum values for:

- PEQ frequency,
- PEQ Q,
- every PEQ filter type.

Existing protocol mappings for those fields may remain in use where already captured, but their generic software bounds must not be described as native endpoint limits until separate evidence exists.

## Echo and other sections

The repository contains captured wire mappings and many real parameter values for Echo, Music, Mic, Main, Surround, Center, Sub, and System. Those are **not automatically min/max evidence**.

Until native endpoint captures are recorded for a control:

1. do not claim a generic software clamp is the native limit;
2. do not widen a UI merely because the wire field can store a wider number;
3. do not add a control that the native UI does not expose unless its behavior is independently captured and deliberately documented.

This distinction is intentional: **protocol representation, observed preset value, and native UI domain are three different facts.**
