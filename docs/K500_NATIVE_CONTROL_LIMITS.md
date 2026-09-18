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

## Recovered prior range evidence

The repository history did contain several earlier range decisions, but they were scattered across UI commits instead of being kept in one authoritative contract. P3.1 recovers them here so future work does not depend on remembering an old thread.

| Control family | Range | Evidence class | Repository evidence |
| --- | --- | --- | --- |
| Top/Master Music, Mic, FX | 0..84 | DONOR-WEB | `8524101578b18e96ffb5a8381b83036e37260718` — "Use web K500 master volume range" |
| Music Input1/Input2/BT/UDisk/Digital gain | -12..+12 dB | DONOR-WEB | `a61b4ba3c3580d785f14123391de6d441ae71898` — "Use web music input gain range" |
| Mic FBX/FBE level | 0..3 | CAPTURED-NATIVE | `6ca67ff22c7dd706f8c1bc53f59c81189ade7487` — native 3→2→1→0 capture |

These recovered domains are now also declared in `src/k500/K500NativeLimits.h`. Their provenance is intentionally kept distinct from the 2026-09-18 physical Reverb endpoint screenshots.

A key audit finding was that the old UI commits did **not** update every layer. Before P3.1:

- Master faders displayed 0..84, while `StudioEngine` still accepted 0..100.
- Music input faders displayed -12..+12 dB, while `StudioEngine` still clamped -60..+10 dB.
- Protocol/controller layers had their own independent clamps.

That split-source design is exactly what this registry is intended to eliminate.

## PEQ fields not yet endpoint-qualified

The screenshots above prove Reverb PEQ **gain** endpoints only. They do not establish authoritative minimum/maximum values for:

- PEQ frequency,
- PEQ Q,
- every PEQ filter type.

Existing protocol mappings for those fields may remain in use where already captured, but their generic software bounds must not be described as native endpoint limits until separate evidence exists.

## Open endpoint audit matrix

The following controls have live mappings or current UI ranges, but **their native minimum/maximum endpoints are not yet established by endpoint evidence in this repository**. The numbers shown under “current software range” describe the code that exists today; they are not promoted to native truth.

| Area | Control | Current software range | What is actually proven | Status |
| --- | --- | --- | --- | --- |
| Mic | Mic A / Mic B level | 0..100 | live scalar mapping exists | ENDPOINT CAPTURE NEEDED |
| Mic | Compressor threshold | -50..0 dB | storage/command mapping exists | ENDPOINT CAPTURE NEEDED |
| Mic | Compressor ratio | 1..100 | storage/command mapping exists | ENDPOINT CAPTURE NEEDED |
| Mic | Attack | 1..100 ms | storage/command mapping exists | ENDPOINT CAPTURE NEEDED |
| Mic | Release | 20..5000 ms in rack UX | storage/command mapping exists | ENDPOINT CAPTURE NEEDED |
| Mic/Music/Outputs | Generic HPF / LPF | 20..20000 Hz | CMD 0x11 selectors/mappings exist for supported sections | ENDPOINT CAPTURE NEEDED PER SECTION |
| Echo | Level / Direct | 0..100 in UX | native CMD 0x0D delta captures prove byte mapping and values around 90..100 | ENDPOINT CAPTURE NEEDED |
| Echo | Repeat | 0..10 in UX | capture proves at least 2→10 | MIN ENDPOINT NEEDED |
| Echo | Left Delay | 0..1000 ms in UX | capture proves at least 300→310 ms | ENDPOINT CAPTURE NEEDED |
| Echo | HPF | 20..20000 Hz in UX | capture proves at least 550→560 Hz | ENDPOINT CAPTURE NEEDED |
| Echo | LPF | 20..20000 Hz in UX | capture proves at least 4200→4210 Hz | ENDPOINT CAPTURE NEEDED |
| Outputs | L/R/mono level | -37.5..+24 dB in UX | raw↔dB mapping is implemented | ENDPOINT CAPTURE NEEDED |
| Outputs | Mic/Music/Reverb/Echo mix | 0..100 in UX | native block byte mapping exists | ENDPOINT CAPTURE NEEDED |
| Outputs | Delay | 0..50 ms in UX for exposed fields | some native delay mapping exists; some fields remain read-only | ENDPOINT CAPTURE NEEDED |
| Dynamics | Output compressor controls | shared rack ranges | native block mapping exists | ENDPOINT CAPTURE NEEDED |
| PEQ | Frequency | 20..20000 Hz in engine | EQ command encoding exists | ENDPOINT CAPTURE NEEDED |
| PEQ | Q | 0.1..30 in engine | EQ command encoding exists | ENDPOINT CAPTURE NEEDED |
| System | Recording / trigger / other limits | various UX ranges | mixed readback-only/local behavior | ENDPOINT CAPTURE NEEDED |

Until endpoint evidence is added, these ranges must not be described in docs, tests, or UI comments as “native min/max”. If a range is changed, the change must state whether it is a temporary UX guard, a donor-web value, or a captured-native endpoint.

## Echo and other sections

The repository contains captured wire mappings and many real parameter values for Echo, Music, Mic, Main, Surround, Center, Sub, and System. Those are **not automatically min/max evidence**.

Until native endpoint captures are recorded for a control:

1. do not claim a generic software clamp is the native limit;
2. do not widen a UI merely because the wire field can store a wider number;
3. do not add a control that the native UI does not expose unless its behavior is independently captured and deliberately documented.

This distinction is intentional: **protocol representation, observed preset value, and native UI domain are three different facts.**
