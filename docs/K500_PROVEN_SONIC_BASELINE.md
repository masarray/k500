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
resources/presets/03_DANGDUT_SUPREME.k500
internal/hardware name: DGT HIFI CORE V3
```

The successful real-hardware progression was:

```text
Core V1: bass became enjoyable; mid still ordinary
Core V2: small mid refinement -> mid became enjoyable
Core V3: small very-bottom extension -> overall result became enjoyable
```

Therefore Mode 03 Hi-Fi Core V3 remains the **GOLD MUSIC REFERENCE**. Do not casually rebuild its successful bass/mid architecture from a generic karaoke curve.

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

## 7. Critical Mode 01 authority update — v1.0

Historical research documents treated Mode 01 as a modeled Hi-Fi transplant derived toward Mode 03. **That assumption is now superseded.**

The authoritative v1.0 Mode 01 is the exact native preset that passed physical K500 testing:

```text
resources/presets/01_ALL_GENRE.k500
internal name: CONCERT HIFI V4
SHA-256: 9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

The reconstructed predecessor was structurally/checksum valid but was not byte-identical to the native donor and could leave music silent after Mass Upload. Therefore:

> **For Mode 01, exact native donor identity overrides older modeled transplant targets.**

Do not reapply historical Mode 01 route/EQ targets onto the native file merely to make it numerically resemble Mode 03.

Any future Mode 01 sonic experiment must branch from the exact native donor and preserve a clear rollback path to that hash.

## 8. Other modes

Modes 02–10 should be treated from their current repository donor bytes, not reconstructed from tables in historical notes. During the pre-v1 native audit, Modes 02–10 matched the supplied native K500 donor files byte-for-byte; preserve that provenance unless a deliberate new preset revision is being engineered.

Mode 03 remains the gold **music listening reference**, but it is not permission to normalize every other preset into Mode 03.

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
