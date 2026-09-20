# K500 Native Value Ranges

This file is the repository contract for manufacturer-KTV value domains. Do not
invent wider editor ranges for convenience. When new hardware/native evidence is
captured, update this file first, then keep QML, StudioEngine, Controller and
Protocol clamps aligned.

## Evidence levels

- **Native UI observed** — directly visible at the manufacturer application's
  minimum/maximum state on physical K500 hardware.
- **Captured/legacy contract** — retained from earlier K500 reverse-engineering
  patches/captures and must not be widened without new evidence.
- **Unverified** — do not tighten or expand based on guesswork.

## Reverb — native UI observed

The 2026-09-18 physical-device acceptance screenshots show the manufacturer
Reverb page at both endpoints.

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Reverb Level | 0 | 100 | scalar/% | Native UI observed |
| Reverb Decay | 500 | 5000 | ms | Native UI observed |
| Reverb Predelay | 0 | 100 | ms | Native UI observed |
| Reverb Highpass | 20 | 1000 | Hz | Native UI observed |
| Reverb Lowpass | 4000 | 16000 | Hz | Native UI observed |
| Reverb Direct | 0 | 100 | scalar/% | Native UI observed |
| Reverb PEQ Gain | -24 | +24 | dB | Native UI observed |

The FX filter bounds also match the older filterRangeForEqKey() contract:
HPF 20..1000 Hz, LPF 4000..16000 Hz.

## Echo — retained captured/legacy contract

The earlier K500 editor/reverse-engineering work used the same FX-filter domain
for Echo and the Qt native bridge already carries captured CMD 0x0D mappings.

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Echo Level | 0 | 100 | scalar/% | Captured/legacy contract |
| Echo Repeat | 0 | 10 | scalar | Captured/legacy contract |
| Echo Left Delay | 0 | 1000 | ms | Captured/legacy contract |
| Echo Highpass | 20 | 1000 | Hz | Captured/legacy FX filter contract |
| Echo Lowpass | 4000 | 16000 | Hz | Captured/legacy FX filter contract |
| Echo Direct | 0 | 100 | scalar/% | Captured/legacy contract |

Fields not listed here (for example hidden/right-channel timing metadata) remain
device-owned/unverified unless a dedicated capture proves their user-facing range.

## Music Tone — native capture observed

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Music Noise Gate | OFF, then -90 | -50 | dB | Native packet capture |
| Music Bass | -12.0 | +12.0 | dB | Native packet capture |

Music Noise Gate uses raw `0` for OFF and raw `1..41` for `-90..-50 dB`.
Music Bass uses CMD `0x0C`, selector `0x02`, encoded in 0.1 dB units:
`raw = round((dB + 12) * 10)`.

Connect/readback offsets for these two Music Tone controls are still evidence-gated;
the current checkpoint implements only their byte-verified live WRITE mappings.

## Mic FBX / anti-feedback — native capture observed

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| FBX Level | 0 | 4 | integer level | Paired physical USB captures |

READ truth is direct `activeMemory[0x001B]`. WRITE uses Top Mic `CMD 0x05`
with the same raw integer level `0..4`; the following command byte is fixed
`0x00` in all five captured levels.

## Common PEQ

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| PEQ Gain | -24 | +24 | dB | Native UI observed / captured |
| PEQ Frequency | 20 | 20000 | Hz | Existing native EQ contract |

## Implementation rule

The same range must exist at every writable layer:

1. QML interaction bounds.
2. EqBandModel / StudioEngine model clamp.
3. K500Controller semantic-state clamp.
4. K500Protocol transport serialization clamp.
5. Hardware-free CI guard/self-test.

A UI-only clamp is insufficient because programmatic calls could still generate
out-of-domain native writes.

## Reconciliation / live-edit rule

Ordinary live edits must not force the application through a full 939-byte
authoritative resync. Full active-memory hydration remains the authority barrier
for connect/reconnect/Recall/recovery and explicit qualification. Transport
acceptance is still not hardware confirmation, but verification must not make
normal knob/fader editing look like a disconnect/reconnect cycle.

## Change control

If a new native-app screenshot/capture contradicts this table:

1. preserve the evidence;
2. update this document with date/source;
3. update all writable layers together;
4. add/adjust deterministic guards;
5. rerun physical-device acceptance before merging.
