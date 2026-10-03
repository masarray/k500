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
```

The underlying Mode 03 Hi-Fi Core V3 Music/Sub architecture remains the **GOLD MUSIC REFERENCE** inside the official `KAR DANGDUT` file. Do not casually rebuild its successful bass/mid architecture from a generic karaoke curve.

## 3. Proven Mode 03 tonal core

Music EQ reference:

| Band | Type | Frequency | Q | Gain |
|---:|---|---:|---:|---:|
| 1 | P | 11704 Hz | 2.0 | +1.8 dB |
| 2 | P | 158 Hz | 2.3 | +2.3 dB |
| 3 | P | 66 Hz | 0.4 | +7.6 dB |
| 4 | P | 1315 Hz | 2.4 | -0.6 dB |
| 5 | P | 2916 Hz | 2.1 | +1.3 dB |
| 6 | P | 6495 Hz | 1.9 | +1.5 dB |
| 7 | HS | 766 Hz | 0.4 | +6.5 dB |

Raw gain values in tenths of dB:

```text
[18, 23, 76, -6, 13, 15, 65]
```

### Why the balance matters

- **66 Hz:** broad low-end excitement/weight. Do not keep raising it to solve every request for deeper bass.
- **158 Hz:** kick/bass-body and physical punch.
- **1315 Hz:** the V2 move from -1.2 to -0.6 dB was important to make the mid feel less ordinary.
- **2916 Hz:** controlled articulation; protect the 2.5–4.5 kHz harshness guardrail.
- **6495 Hz:** micro-detail and polish.
- **11704 Hz:** air and top-end luxury.
- **HS 766 Hz:** broad openness; do not blindly increase it further.

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

For enjoyable midrange, increase information density and texture rather than broad mid loudness. The successful V2 refinement was small:

```text
1315 Hz  -1.2 -> -0.6 dB
2916 Hz  +1.0 -> +1.3 dB
6495 Hz  +1.4 -> +1.5 dB
11704 Hz +1.6 -> +1.8 dB
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
Mode identity = vocal architecture + FX/spatial vocal behavior
Music quality = shared premium Hi-Fi target where the donor allows it
```

Do not destroy an accepted vocal identity just to make raw music-routing numbers look identical between modes.

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
SHA-256: 4d1f2dd4f5431de1df1819931ecf65be4242e2bc9ae4e9dbabdbee3504004cf1
```

Future experiments normally branch from the current official bytes; rollback investigations may explicitly return to the archived native hash recorded in the v1.0 history.

## 8. Final official preset library

| Slot | Repository file | Internal name | SHA-256 / status | Sonic role |
|---:|---|---|---|---|
| 01 | `01_KONSER_NYANYI.k500` | `KONSER NYANYI` | `4d1f2dd4f5431de1df1819931ecf65be4242e2bc9ae4e9dbabdbee3504004cf1` | concert / Air-Focus universal karaoke |
| 02 | `02_MC_HOST_RADIO.k500` | `MC HOST RADIO` | rename-only from the same donor audio | dry/controlled broadcast utility |
| 03 | `03_KAR_DANGDUT.k500` | `KAR DANGDUT` | `3c446e94ddf0db32490d69a182fdc675f0a289c7d9e6cc54512dc91cb4673e85` | Dangdut vocal FX over preserved Hi-Fi Core V3 music/sub |
| 04 | `04_POP_ROCK_BALLAD.k500` | `POP ROCK BALLAD` | `d18c9ddf4d8ba9d5d5fa6f027b97cc31784138a510c1e35778b8995e380172f5` | finalized pop-rock / slow-rock |
| 05 | `05_POP_KENANGAN_V2.k500` | `POP KENANGAN V2` | `80b793878642b97b401674c01def771ee2390bd7032f1b66a0d12d84ff41c3b8` | warm romantic 70s/80s slow-pop |
| 06 | `06_SHOLAWAT_SYAHDU.k500` | `SHOLAWAT SYAHDU` | `e7854512443699f6b3202db488a9d2d137162b4d4e756b4f7c34950a541f909c` | soft/fresh sholawat |
| 07 | `07_JAZZ_LOUNGE.k500` | `JAZZ LOUNGE` | `934caa877de5e8cb7ef2dfe8d12a99a4b804989807357ce62805e419b0a6bae7` | intimate/classy lounge |
| 08 | `08_BLUES_CLUB.k500` | `BLUES CLUB` | `741bfa917a8d491070d18d223e4be7a5010d7c4edb171f2e0e65f95cbbe22146` | warm vintage/slap character |
| 09 | `09_ACOUSTIC_NATURAL.k500` | `ACOUSTIC NATURAL` | `8a5ce6f8b310d24d47d755f99c5acf9b9d992f9a85a2e13ccb7156c0f1d8b81c` | natural/organic acoustic vocal |
| 10 | `10_REGGAE_DUB.k500` | `REGGAE DUB` | `1984d309db61c7258329765306e3e573e64730b04a5cd4472568c3f332742c90` | bass/groove + rhythmic echo |

The library is intentionally heterogeneous. Air Focus is a quality reference, not a parameter template. Genre-specific Music/Mic/Sub voicing remains donor-based and should not be normalized without new hardware evidence.

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
