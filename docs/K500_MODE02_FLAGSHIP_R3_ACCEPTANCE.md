# K500 Mode 02 Flagship R3 Acceptance

Date: 2026-10-07

## Decision

Promote the hardware-auditioned R3 architecture as the official Mode 02
`MC HOST RADIO` flagship. The final preset is intentionally close, centered,
authoritative, fresh and airy, with a subliminal studio-booth field rather than
karaoke-style audible FX.

No additional R3.1 tuning is justified by the accepted listening result. Future
changes require one specific failure mode and must preserve all unrelated locked
regions.

## Exact provenance

| Item | SHA-256 |
|---|---|
| Previous official Mode 02 | `8b18cdbd1f4e1ae409c882d84955bfa34ca770e459615524591732182f14339c` |
| Accepted R3 audition candidate (`MC HOST RADIO R3`) | `39eddffca6aeffebbd5544b42ad4499c205ae0a8fd021b10ada374331a3dde67` |
| Promoted official `MC HOST RADIO` | `adf3c868cfa471b4b0975bb58ffb4e8c05c071a7ed0896fc0a89b72933a88fa7` |

The promoted file is exactly 1144 bytes and its additive checksum is valid.
Compared with the accepted R3 audition candidate, promotion changes only four
bytes: the trailing temporary ` R3` name bytes and the checksum. Sonic parameter
bytes are byte-identical.

Relative to the previous official Mode 02, the promoted file changes 35 bytes
including checksum. Unknown/reserved bytes remain donor-owned.

## Hardware-audition progression

### R1 — Broadcast Voice Foundation

The first gate changed only Mic A/B gain fields so the dry presenter voice could
be judged without spatial or dynamics confounds.

Final R1/R3 Mic changes relative to the previous official file:

| Mic band | Previous | Final |
|---|---:|---:|
| 130 Hz | -2.0 dB | -1.0 dB |
| LS 470 Hz | +4.5 dB | +4.1 dB |
| 444 Hz | -1.2 dB | -1.8 dB |
| 1.55 kHz | -0.4 dB | -0.2 dB |
| 2.55 kHz | -1.4 dB | -1.6 dB |
| 3.9 kHz | -2.4 dB | -2.7 dB |
| 6.3 kHz | -0.5 dB | 0.0 dB |
| 9.27 kHz | -6.0 dB | -6.0 dB (LOCK) |
| HS 11 kHz | +8.0 dB | +8.9 dB |

The intended result is more chest authority, less boxiness, essentially unchanged
speech presence, lower 2.5–4.5 kHz fatigue, and modest extra detail/air.

### R2 — Studio Booth Spatial Architecture

R2 kept the accepted dry voice intact and moved spatial support away from the
front image:

- Surround direct Mic: `30 -> 24`
- Main Reverb: `44 -> 34`
- Center Reverb: `20 -> 12`
- Main Echo: `5 -> 3`
- Center Echo: `2 -> 1`
- Reverb: `720 -> 560 ms`, predelay `30 -> 22 ms`, HPF `350 -> 400 Hz`,
  LPF `12.5 -> 11.2 kHz`
- Echo timing stayed `95 ms / repeat 1`

This produced a cleaner, more centered booth presentation and removed the old
short-karaoke-room impression.

### R3 — Fresh-Air Broadcast Polish

R3 responded to the remaining closed/dry character without boosting the critical
2.5–4.5 kHz presence region:

- Mic 6.3 kHz: `-0.3 -> 0.0 dB`
- Mic HS 11 kHz: `+8.3 -> +8.9 dB`
- Main Reverb: `34 -> 36`
- Surround Reverb: `46 -> 50`
- Reverb 5.55 kHz: `-13.0 -> -12.3 dB`
- Reverb 10.5 kHz: `-15.0 -> -13.5 dB`
- Reverb LPF: `11.2 -> 13.0 kHz`
- Reverb decay/predelay: `560/22 -> 600/25 ms`

The high-frequency opening is concentrated in detail/air and the lateral wet
field. The 3.9 kHz comfort guard and 9.27 kHz sibilance notch stay locked.

## Final architecture

```text
Dry presenter:
  Main Mic      100
  Center Mic    100
  Surround Mic   24
  Mic HPF        95 Hz
  Mic dynamics   preserved from accepted donor

Studio booth:
  Main Reverb     36
  Center Reverb   12
  Surround Reverb 50
  Reverb          600 ms / 25 ms / 400-13000 Hz

Early reflection:
  Main Echo        3
  Center Echo      1
  Surround Echo    6
  Echo             95 ms / repeat 1 / 700-3200 Hz
```

Music PEQ/crossover/routing, Main EQ, Center EQ, Surround EQ, Sub
EQ/crossover/routing, output dynamics and unknown/reserved fields remain outside
this vocal evolution unless explicitly listed above.

## LOCK

Treat these as hardware-accepted and locked:

- balanced enhanced Music core and Sub foundation
- Mic 95 Hz HPF and dynamics
- broadcast chest/body vs low-mid balance
- 2.5–4.5 kHz comfort guard
- 9.27 kHz sibilance notch
- fresh 6.3/11 kHz detail-air balance
- Main/Center presenter authority
- controlled Surround direct voice
- 95 ms / repeat-1 dark early reflection
- 600 ms fresh/airy studio-booth field

Do not turn Mode 02 into an Air-Focus karaoke preset. Its identity is professional
broadcast intimacy with freshness and long-form listening comfort.

## Promotion discipline

The R3 hardware bytes are promoted exactly; the only post-audition mutation is
normalizing the temporary hardware name back to `MC HOST RADIO` and recomputing
the additive checksum. No application/protocol/UI code is part of this preset
promotion.
