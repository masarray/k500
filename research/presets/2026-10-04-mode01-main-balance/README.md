# Mode 01 — Main Balance hardware-audition progression

Date: 2026-10-04

## Current candidate

**V2 is the active GOLD candidate pending real K500 listening acceptance.**

`01_KONSER_NYANYI_MAIN_BALANCE_V2.k500`

The official distribution file is intentionally **not replaced yet**. Promotion only
happens after hardware listening confirms V2.

## Problem statement

The supplied Mode 01 Main-output capture and listening feedback showed:

- sub/bass and kick originally lacked authority;
- cymbal/detail energy attracted too much attention ("kemrences");
- the existing KONSER NYANYI vocal/FX identity must be preserved.

V1 corrected the broad low-vs-high balance. The follow-up capture showed that:

- deep foundation is now sufficient and must be locked;
- treble is already civilized and must not be cut further;
- stereo imaging is healthy and must be locked;
- the remaining refinement is a small **kick/body vs midrange** rebalance.

## Donor and progression

Official donor:

`resources/presets/01_KONSER_NYANYI.k500`

Official donor Git blob:

`dcbdd768c455922af147d1ac1e2bd025e4ba4fc6`

V1 Git blob:

`93bc6910a4cec0b01c48bb2d5b4456ebe5f7eba7`

V2 Git blob:

`3325fd8312fbf093193bd68c5a0b8ca5b3b2d487`

Internal hardware name remains:

`KONSER NYANYI`

## V1 — broad correction

V1 changed only Music PEQ gain values:

| Music band | Official | V1 |
| --- | ---: | ---: |
| 11.704 kHz / Q2.0 | +1.8 dB | +1.5 dB |
| 158 Hz / Q2.3 | +2.3 dB | +2.6 dB |
| 66 Hz / Q0.4 | +7.6 dB | +8.2 dB |
| 6.495 kHz / Q1.9 | +1.5 dB | +1.0 dB |
| HS 766 Hz / Q0.4 | +6.5 dB | +5.8 dB |

The real Main-output follow-up confirmed the intended direction: stronger low-end
authority, less distracting top-end energy, healthy stereo field, and no reason to
continue cutting treble.

## V2 — final micro-balance

V2 keeps every V1 correction and changes only two semantic parameters:

| Music band | V1 | V2 | Delta | Purpose |
| --- | ---: | ---: | ---: | --- |
| 158 Hz / Q2.3 | +2.6 dB | **+3.0 dB** | +0.4 dB | more kick/body authority |
| 1315 Hz / Q2.4 | -0.6 dB | **-0.9 dB** | -0.3 dB | slightly relax backing midrange / vocal pocket |

Everything else is locked.

Modeled V1 -> V2 Music-path delta:

- peak at 158 Hz: about **+0.40 dB**
- average 120–180 Hz: about **+0.31 dB**
- peak at 1315 Hz: about **-0.30 dB**
- average 800 Hz–2 kHz: about **-0.15 dB**
- effectively unchanged below ~90 Hz and above ~4 kHz

Relative to the original official Mode 01, V2 remains approximately:

- +0.56 dB average at 40–60 Hz
- +0.62 dB at 60–90 Hz
- +0.64 dB at 90–120 Hz
- +0.88 dB at 120–180 Hz
- -0.61 dB at 800 Hz–2 kHz
- -0.68 dB at 2–4 kHz
- -1.01 dB at 4–8 kHz
- -0.98 dB at 8–12 kHz

These are comparative model values, not claims of exact acoustic K500 transfer.

## V1 -> V2 byte-diff audit

Only **three bytes** differ, including checksum:

- `0x01C0` Music band 2 gain: 26 -> 30
- `0x01D0` Music band 4 gain: -6 -> -9
- `0x0475` checksum: 137 -> 136

Container remains exactly 1144 bytes and checksum modulo 256 remains zero.

## Locked regions

Do not alter these in the next listening round:

- 66 Hz low foundation;
- 6.495 kHz detail;
- HS 766 Hz broad openness;
- 11.704 kHz air;
- all Mic A/B settings;
- Main / Center / Surround / Sub EQ, routing and output levels;
- Reverb / Echo;
- compressors;
- delays;
- crossovers;
- every Alt / reserved / unknown field.

This is deliberate. V2 tests one remaining causal hypothesis only.

## Hardware acceptance

Use the same song, audio-interface gain, K500 Main output, speaker level and room
position used for the V1 follow-up capture.

Accept V2 only if:

- kick has slightly more physical "DUM/thump" without upper-bass boom;
- bass remains deep but does not become heavier in the very bottom;
- 300 Hz–2 kHz no longer feels more authoritative than necessary;
- cymbal/hi-hat remains as smooth as V1 — not darker;
- backing music leaves a more effortless pocket for the singer;
- no loss of detail, openness or width;
- long listening remains non-fatiguing.

If these pass, treat the exact V2 bytes as the Mode 01 GOLD candidate for promotion.
Do not rebuild the binary after acceptance.
