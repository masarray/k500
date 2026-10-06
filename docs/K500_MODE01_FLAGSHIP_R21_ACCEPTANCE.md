# K500 Mode 01 Flagship R2.1 Acceptance

Date: 2026-10-06

## Decision

Promote the R2.1 sonic architecture as the official Mode 01 flagship and lock the
accepted singer-comfort regions. The public/internal preset identity remains
`KONSER NYANYI`.

The decision is based on iterative real-K500 listening/capture plus the repository's
comparative path model. No further broad tuning is justified: the remaining
differences versus Mode 05 are intentionally biased toward lower i-ring occupancy
and slightly greater spatial polish rather than maximum brightness.

## Exact provenance

| Item | SHA-256 |
|---|---|
| Previous official Mode 01 | `761d0ecf1f470ce433fcf760d7ee1317e994dbefbb16fc71e8498aea9d99d6c4` |
| R2.1 hardware-audition candidate (`KONSER NYANYI R2`) | `b5a75750f46aeb51775e42eea1c9eedceae0137857e2913db06e21c466ea8622` |
| Promoted official `KONSER NYANYI` | `66ba788daadf56212e4de6673a702404cfc915dc934e98fe3b479ee972f978a1` |

The promoted file is exactly 1144 bytes, passes the additive checksum, and embeds
the hardware-safe name `KONSER NYANYI`. The R2.1 audition candidate and promoted
file differ only in the temporary name bytes and resulting checksum; sonic
parameters are identical.

Relative to the previous official file, 68 bytes change. Unknown/reserved bytes and
Alt blocks remain donor-owned.

## Sonic architecture locked

### Preserved from the accepted Mode 01 foundation

- Music PEQ and Music routing
- Main PEQ
- Sub PEQ / crossover / routing
- Mic dynamics
- output foundation and unknown/reserved data

### Singer-support evolution

- Mic 130 Hz: `-3.0 -> -2.0 dB`
- Mic LS 470 Hz: `+5.2 -> +5.5 dB`
- Mic 444 Hz: `-1.2 -> -1.1 dB`
- Mic 2.55 kHz: `-1.8 -> -1.7 dB`
- Mic 9.27 kHz: `-6.0 -> -6.1 dB`
- Surround Mic: `70 -> 74`
- Center Reverb: `74 -> 84`
- Center Echo: `34 -> 36`
- Surround Echo: `50 -> 56`

The goal is more singer body/support without converting the flagship into a
forward or sharp vocal preset.

### Final FX lock

- Main Reverb: `97 -> 95`
- Main Echo: `38 -> 34`
- Reverb: `2450 ms` decay, `77 ms` predelay, `250-13200 Hz`
- Echo: `310 ms`, repeat `2`, `600-4100 Hz`
- Echo 1.166 / 2.708 / 6.003 kHz cuts: `-9.3 / -9.8 / -11.2 dB`

This keeps the direct cue authoritative while the wet field supplies scale around
the singer instead of competing in the front image.

## Comparative model versus accepted Mode 05

Using the repository's RBJ/crossover comparative model:

| Path / region | Mode 01 R2.1 minus Mode 05 |
|---|---:|
| Dry Main vocal body 120-250 Hz | +0.145 dB |
| Dry Main vocal low-mid 250-500 Hz | -0.152 dB |
| Dry Main vocal presence 1.5-2.5 kHz | -0.038 dB |
| Dry Main vocal i-ring 2.5-4.5 kHz | -0.215 dB |
| Dry Main vocal detail 5-7 kHz | +0.057 dB |
| Dry Main vocal sparkle 7-10 kHz | +0.135 dB |
| Dry Main vocal air 10-14 kHz | +0.105 dB |
| Main composite i-ring 2.5-4.5 kHz | -0.300 dB |
| Center vocal body 120-250 Hz | +0.221 dB |
| Surround vocal i-ring 2.5-4.5 kHz | -0.274 dB |
| Surround vocal air 10-14 kHz | +0.215 dB |

Interpretation: Mode 01 no longer needs more broad EQ or more wet level to beat
Mode 05 as the flagship singer experience. It already combines equal/greater body
and air with lower i-ring occupancy.

## Hardware-capture gate

The supplied R2.1 WAV is 127.32 s / 44.1 kHz stereo. The capture has ample
headroom (approximately -10.68 dBTP by 4x oversampled proxy) and no clipping.
Interface L/R gain mismatch is intentionally excluded from the sonic decision.

Matched same-program comparison confirmed that the R2.1 micro-calibration restored
wet-field detail/air relative to R2 while preserving the R2 singer-support
architecture. Capture-level spectral differences are treated as supporting
evidence only because singer performance and interface gain are not calibration
references.

## Lock

After promotion, treat the following as LOCKED unless a new controlled hardware
test identifies a specific failure:

- Music / Main / Sub tonal foundation
- Mic body and harshness guard
- Mic dynamics
- Echo delay/repeat/bandwidth
- Center anchor
- Surround support architecture
- Reverb/FX integration

Future tuning must state one narrow failure mode and preserve the other locked
regions. Do not restart broad preset experimentation from a generic curve.
