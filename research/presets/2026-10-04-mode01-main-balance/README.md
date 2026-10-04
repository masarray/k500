# Mode 01 — Main Balance V1 hardware-audition candidate

Date: 2026-10-04

## Purpose

Target the reported Mode 01 Main-output failure mode:

- sub/bass and kick do not feel authoritative enough;
- upper detail/cymbal energy reads as too "kemrences";
- preserve the already-developed KONSER NYANYI vocal/FX identity.

This is intentionally a **Music-path-only** revision. Main/Center/Surround/Sub EQ,
all Mic A/B settings, Reverb/Echo, output routing, compressors, delays, crossovers,
and startup/system fields are bit-preserved from the current official Mode 01.

## Donor

`resources/presets/01_KONSER_NYANYI.k500`

Source Git blob:

`dcbdd768c455922af147d1ac1e2bd025e4ba4fc6`

Candidate Git blob:

`93bc6910a4cec0b01c48bb2d5b4456ebe5f7eba7`

Internal hardware name remains:

`KONSER NYANYI`

## Surgical changes

| Music band | Role | Official | Candidate | Delta |
| ---: | --- | ---: | ---: | ---: |
| 1 @ 11.704 kHz | air | +1.8 dB | +1.5 dB | -0.3 dB |
| 2 @ 158 Hz | kick/body | +2.3 dB | +2.6 dB | +0.3 dB |
| 3 @ 66 Hz | low foundation | +7.6 dB | +8.2 dB | +0.6 dB |
| 6 @ 6.495 kHz | cymbal/detail | +1.5 dB | +1.0 dB | -0.5 dB |
| 7 HS @ 766 Hz | broad openness | +6.5 dB | +5.8 dB | -0.7 dB |

All frequencies, Q values and raw PEQ type aliases are preserved.

## Byte-diff audit

Only six bytes differ, including checksum:

- `0x01B8` Music band 1 gain: 18 -> 15
- `0x01C0` Music band 2 gain: 23 -> 26
- `0x01C8` Music band 3 gain: 76 -> 82
- `0x01E0` Music band 6 gain: 15 -> 10
- `0x01E8` Music band 7 gain: 65 -> 58
- `0x0475` checksum: 131 -> 137

Container size remains 1144 bytes and modulo-256 checksum remains zero.

## Capture-informed modeled delta

Applying the Music-EQ response delta to the supplied Mode 01 Main-output WAV predicts
approximately:

- 40–60 Hz: **+0.57 dB**
- 60–90 Hz: **+0.60 dB**
- 90–120 Hz: **+0.56 dB**
- 120–180 Hz: **+0.58 dB**
- 1–2 kHz: **-0.50 dB**
- 2–4 kHz: **-0.65 dB**
- 4–8 kHz: **-0.96 dB**
- 8–12 kHz: **-0.96 dB**

This creates roughly 1.5 dB more low-end authority relative to the cymbal/detail
region without a broad bass rewrite.

## Why this shape

The current Mode 01 already carries the proven Mode 03 Hi-Fi Core V3 Music/Main/Sub
architecture. The reported test, however, is **Main-output listening**, so the
dedicated Sub foundation is not the right place to solve the complaint.

This candidate therefore:

1. preserves the proven core topology;
2. adds only a small 66/158 Hz Music-path lift;
3. removes excess broad Music-path brightness rather than cutting the shared Main EQ;
4. leaves Mic and vocal FX untouched, so singer identity is not collateral damage;
5. keeps 10–14 kHz air present instead of applying a destructive global treble cut.

## Hardware acceptance

Use the same song, same interface gain, same K500 Main output and same speaker level.

Accept only if all are true:

- kick gains physical impact without becoming bloated;
- bass line is easier to follow at low/medium listening level;
- cymbal/hi-hat remains detailed but no longer dominates attention;
- male and female vocal backing space feels easier to sing into;
- no new 120–250 Hz boom appears;
- no loss of desirable air/openness;
- 20–30 minute listening is less fatiguing.

If accepted on real K500, promote the exact candidate bytes into official Mode 01
and update the official SHA/golden references. Do not rebuild after acceptance.
