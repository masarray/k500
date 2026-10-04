# K500 Proven Sonic Baseline

> **Purpose:** durable handoff for engineers and AI agents working on SonKuPik K500 presets.
>
> **Authority:** real K500 listening and exact donor bytes win over simulation or historical modeled assumptions.
>
> **Baseline:** SonKuPik K500 v1.0 stable line.

## 1. Product-level sonic goal

The official preset library is designed to preserve each mode's vocal identity while giving music a consistent premium playback benefit.

Target character:

```text
deep + round + punchy + clean + textured + detailed + silky + wide + non-fatiguing
```

Enhancement does **not** mean maximum loudness or boosting every EQ band. It comes from spectral balance, routing, depth, width, controlled transient/detail energy, and appropriate use of Main/Center/Surround/Sub.

## 2. Current gold music reference — Mode 03

The current hardware-approved **music tuning reference** remains:

```text
resources/presets/03_KAR_DANGDUT.k500
internal/hardware name: KAR DANGDUT
```

The successful real-hardware progression was:

```text
Core V1: bass became enjoyable; mid still ordinary
Core V2: small mid refinement -> mid became enjoyable
Core V3: small very-bottom extension -> overall result became enjoyable
2026-10-04 R1: real-K500 comparison accepted a slightly stronger 158 Hz punch / smoother upper balance while preserving the proven Sub foundation
```

The underlying Mode 03 Hi-Fi Core V3 Music/Sub architecture remains the **GOLD MUSIC REFERENCE** inside the official `KAR DANGDUT` file. Do not casually rebuild its successful bass/mid architecture from a generic karaoke curve.

## 3. Proven Mode 03 tonal core

Music EQ reference:

| Band | Type | Frequency | Q | Gain |
|---:|---|---:|---:|---:|
| 1 | P | 11704 Hz | 2.0 | +1.6 dB |
| 2 | P | 158 Hz | 2.3 | +3.1 dB |
| 3 | P | 66 Hz | 0.4 | +8.2 dB |
| 4 | P | 1315 Hz | 2.4 | -0.8 dB |
| 5 | P | 2916 Hz | 2.1 | +1.3 dB |
| 6 | P | 6495 Hz | 1.9 | +1.2 dB |
| 7 | HS | 766 Hz | 0.4 | +6.0 dB |

Raw gain values in tenths of dB:

```text
[16, 31, 82, -8, 13, 12, 60]
```

### Why the balance matters

- **66 Hz:** broad low-end excitement/weight. Do not keep raising it to solve every request for deeper bass.
- **158 Hz:** kick/bass-body and physical punch.
- **1315 Hz:** the accepted R1 value is -0.8 dB, preserving vocal/music separation without hollowing the backing.
- **2916 Hz:** controlled articulation; protect the 2.5–4.5 kHz harshness guardrail.
- **6495 Hz:** micro-detail and polish; R1 deliberately backs this down to +1.2 dB to avoid cymbal dominance.
- **11704 Hz:** air and top-end luxury; R1 keeps it present at +1.6 dB rather than chasing brightness.
- **HS 766 Hz:** broad openness; accepted R1 uses +6.0 dB and should not be increased casually.

## 4. Proven Sub foundation

Mode 03 Sub reference:

| Band | Frequency | Gain |
|---:|---:|---:|
| 1 | 53 Hz | +2.9 dB |
| 2 | 65 Hz | +4.3 dB |
| 3 | 80 Hz | +1.0 dB |
| 4 | preserve donor | 0.0 dB |
| 5 | preserve donor | 0.0 dB |

The successful lesson is:

> When bass/punch is already good, add very-bottom satisfaction with a **small 53–65 Hz Sub change**, not by raising the entire bass structure.

Use Music/Main for broad musical bass/body and the dedicated Sub path for very-bottom foundation.

## 5. Mid and treble design lessons

For enjoyable midrange, increase information density and texture rather than broad mid loudness. The current accepted Mode 03 R1 balance uses small, role-specific moves around the proven core rather than a wholesale rewrite:

```text
158 Hz   +2.3 -> +3.1 dB
66 Hz    +7.6 -> +8.2 dB
1315 Hz  -0.6 -> -0.8 dB
6495 Hz  +1.5 -> +1.2 dB
11704 Hz +1.8 -> +1.6 dB
HS 766   +6.5 -> +6.0 dB
```

Useful treble roles:

```text
2.5–4.5 kHz = articulation guardrail
5–7 kHz     = harmonic detail / polish
7–10 kHz    = sparkle
10–14 kHz   = air / openness
```

Premium treble should be revealing and silky, not merely brighter.

## 6. Mode identity vs shared music quality

Working architecture:

```text
Mode identity = vocal architecture + FX/spatial vocal behavior + output/Sub context
Music quality = shared premium balanced-enhanced family, with small context-specific Music-gain voicing
```

Real-K500 listening has converged on a tight accepted Music envelope in Modes 01, 03 and 05. Use that envelope as the family anchor for the whole library; do not leave another mode on an older, thinner Music balance merely to preserve historical donor differences. Context should be expressed with small Music deltas and with the existing Mic/FX/output/Sub architecture, not by transplanting a whole preset.

Output gain differs by preset, so identical route values do not imply identical effective energy. A useful comparative proxy is:

```text
Effective route amplitude ~= route * 10^(output_dB / 20)
```

This is a routing proxy, not an exact acoustic/DSP model.

## 7. Current Mode 01 authority

The exact native `CONCERT HIFI V4` donor remains the rollback/provenance reference that fixed the pre-v1 Mass Upload failure class. The current official distribution preset is donor-derived and hardware-listening-evolved:

```text
resources/presets/01_KONSER_NYANYI.k500
internal name: KONSER NYANYI
SHA-256: 761d0ecf1f470ce433fcf760d7ee1317e994dbefbb16fc71e8498aea9d99d6c4
```

Future experiments normally branch from the current official bytes; rollback investigations may explicitly return to the archived native hash recorded in the v1.0 history.

## 8. Current official preset library

The 2026-10-04 official bank now uses one balanced-enhanced Music family. Modes 01, 03 and 05 are the real-K500 listening anchors and remain locked; Modes 02, 04 and 06–10 use surgical Music-gain-only variants close to that accepted envelope. All non-Music donor identity is preserved.

Nominal family anchor (Music PEQ gain, bands 1..7):

```text
[ +1.6, +3.0, +8.1, -0.9, +1.3, +1.1, +5.8 ] dB
```

| Slot | Repository file | Internal name | SHA-256 | Sonic role |
|---:|---|---|---|---|
| 01 | `01_KONSER_NYANYI.k500` | `KONSER NYANYI` | `761d0ecf1f470ce433fcf760d7ee1317e994dbefbb16fc71e8498aea9d99d6c4` | locked concert/universal anchor |
| 02 | `02_MC_HOST_RADIO.k500` | `MC HOST RADIO` | `05ef7c06323d19dd091bb04c47263cbb37e24c24e367e01e02e39d9fde0aa064` | exact nominal enhanced Music core; dry broadcast Mic/FX retained |
| 03 | `03_KAR_DANGDUT.k500` | `KAR DANGDUT` | `ec683042962c635d8d87d262512a694797bd62842c07775e53eb497f34d329bc` | locked punch/groove anchor |
| 04 | `04_POP_ROCK_BALLAD.k500` | `POP ROCK BALLAD` | `8e533466dcd29cbf9cf325dc8128246f1445b35b5d8d4b96a3ea9e47072a3ae1` | slightly stronger punch/opening |
| 05 | `05_POP_KENANGAN_V2.k500` | `POP KENANGAN V2` | `e0d6e985f068576a37ea776c1ed730b63bafac44d3afc80ab31f8541ff6398b5` | locked warm/romantic anchor |
| 06 | `06_SHOLAWAT_SYAHDU.k500` | `SHOLAWAT SYAHDU` | `d55d882e742cab6b0b5f1cbb8a6f2a92238ba78eaa972488bca70167d7b4bbc5` | gentler punch/top for devotional material |
| 07 | `07_JAZZ_LOUNGE.k500` | `JAZZ LOUNGE` | `126a952847c221095189c0a13c1ed8851b0aa41b15a5c592d98b005ccfabc80d` | intimate/classy enhanced variant |
| 08 | `08_BLUES_CLUB.k500` | `BLUES CLUB` | `dbba6942598f15f61db1ab9069a052e0726846a51e76c706e66c16b88ce8c5d8` | warmer/darker enhanced variant |
| 09 | `09_ACOUSTIC_NATURAL.k500` | `ACOUSTIC NATURAL` | `d3834f43518b0de29af86275659bbe2299845a3e7ad950fb4124d87fbb56e4bc` | natural enhanced variant |
| 10 | `10_REGGAE_DUB.k500` | `REGGAE DUB` | `ec70d5ec0fe249ff8d19da12d456e1eab48988c37a619ab5ab51841b9644a2f6` | deeper/groove-led enhanced variant |

The library is heterogeneous in **context**, not in baseline Music quality. Never clone a full preset architecture across modes; share the mature Music balance, then preserve or micro-voice the mode-specific Mic/FX/output/Sub architecture.

## 9. Locking policy

Positive hardware feedback creates a temporary engineering lock.

Example:

```text
bass good -> LOCK bass
mid good  -> LOCK mid
bottom good / overall enjoyable -> GOLD candidate
```

The next revision should target the unresolved problem only. Avoid broad multi-parameter rewrites after a region has already been accepted.

## 10. Preferred tuning workflow

1. Start from the current repository donor bytes.
2. Record SHA-256 and internal name.
3. Identify the exact listening problem and all currently locked regions.
4. Compare against Mode 03 only where a shared music-quality reference is useful.
5. Form one narrow causal hypothesis.
6. Simulate cumulative Music/Mic/FX/output paths as a comparative tool.
7. Patch only proven fields/offsets.
8. Recompute checksum last.
9. Perform changed-byte audit.
10. Generate comparison plots/metrics.
11. Test on a real K500.
12. Keep the change only if hardware listening improves the intended problem without breaking locked areas.

## 11. Signal-flow mental model

```text
Music Input -> Music PEQ/XO -------------------------> output routing -> output PEQ/XO

Mic A/B -> gain/dynamics -> Mic PEQ/XO -> dry -------+
                                                    |
                                                    +-> Reverb PEQ/XO -> wet ---+
                                                    +-> Echo PEQ/XO ----> wet ---+-> output routing -> output PEQ/XO
```

Dry Mic, Reverb, and Echo are **parallel** contributions. Do not model Reverb and Echo as serial inserts.

Conceptual output roles:

- **Main:** front image, tonal body, music punch, primary vocal.
- **Center:** lead/vocal anchor.
- **Surround:** width, air, ambience, decorrelation; not a second Main.
- **Sub:** deep music foundation; keep vocal/FX out unless explicitly proven useful.

## 12. Simulation boundaries

Simulation can help answer:

- what changed;
- where relative energy moved;
- whether cumulative EQ became excessive;
- whether route compensation is directionally sensible;
- whether one revision is likely brighter/warmer/deeper than another.

Simulation cannot prove:

- exact proprietary K500 DSP coefficients for unknown filter implementations;
- room/speaker interaction;
- subjective karaoke confidence/support;
- native compatibility of unknown bytes;
- that a checksum-valid reconstructed preset behaves like an exact native donor.

Hardware listening and donor identity remain authoritative.

## 13. Standard tooling

```bash
python tools/k500_preset_lab.py validate preset.k500
python tools/k500_preset_lab.py inspect preset.k500 --json audit.json
python tools/k500_preset_lab.py plot preset.k500 --out-dir analysis/
python tools/k500_preset_lab.py compare donor.k500 candidate.k500 --out-dir comparison/
python tools/k500_preset_lab.py patch donor.k500 patch.json candidate.k500
```

For plotting/comparison dependencies:

```bash
python -m pip install -r tools/requirements-preset-lab.txt
```

## 14. Handoff rule

A preset-research handoff should include exact donor/output files and hashes, listening goal, locked regions, semantic and byte-level changes, checksum/size/name validation, plots/metrics, real-hardware feedback, and what remains unproven.

If a future note conflicts with this file, prefer the latest exact repository preset bytes plus evidence-backed hardware feedback. For Mode 01, the v1 native donor hash above is the current rollback authority.
