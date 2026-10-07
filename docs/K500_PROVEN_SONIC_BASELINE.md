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
SHA-256: 66ba788daadf56212e4de6673a702404cfc915dc934e98fe3b479ee972f978a1
```

2026-10-06 flagship acceptance evolved Mode 01 through two narrow hardware-audition passes without touching the accepted Music/Main/Sub foundation:

```text
R1 -> R2: more singer body/support, cleaner Center/Surround support, darker/less intrusive Echo, longer dry/wet separation
R2 -> R2.1: only micro-calibrated spatial openness (Main Reverb/Echo +1 step, 2450 ms decay, 77 ms predelay, 13.2 kHz Reverb LPF)
```

The promoted distribution bytes preserve the exact R2.1 sonic parameters. The temporary audition name `KONSER NYANYI R2` was normalized back to the official `KONSER NYANYI` identity before promotion; this name/checksum normalization is non-sonic. Relative to the previous official Mode 01, the final file changes 68 bytes and keeps all unknown/reserved bytes donor-owned.

Future experiments normally branch from the current official bytes; rollback investigations may explicitly return to the archived native hash recorded in the v1.0 history.

### Mode 02 flagship broadcast authority

Mode 02 completed a three-stage real-K500 hardware audition on 2026-10-07:

```text
R1: dry-voice foundation -> more 120–250 Hz authority, less 250–500 Hz boxiness, stronger 2.5–4.5 kHz comfort guard
R2: studio-booth spatial architecture -> less direct Surround voice and less audible front FX while retaining the 95 ms single early reflection
R3: fresh-air polish -> small dry-vocal detail/air lift plus a more open high-frequency wet field without raising the 9.27 kHz sibilance region
```

The accepted distribution authority is:

```text
resources/presets/02_MC_HOST_RADIO.k500
internal name: MC HOST RADIO
SHA-256: adf3c868cfa471b4b0975bb58ffb4e8c05c071a7ed0896fc0a89b72933a88fa7
```

The temporary audition file `MC HOST RADIO R3` had SHA-256 `39eddffca6aeffebbd5544b42ad4499c205ae0a8fd021b10ada374331a3dde67`. Promotion changes only the temporary name bytes plus additive checksum; all sonic parameter bytes are identical to the accepted R3 hardware candidate.

Locked architecture: balanced enhanced Music core, Sub/Main tonal foundation, Mic dynamics and 95 Hz HPF, broadcast body/boxiness balance, 3.9/9.27 kHz comfort guards, 95 ms repeat-1 Echo, Main/Center authority, controlled direct Surround voice, and the fresh/airy 600 ms studio-booth field. Detailed evidence is in `docs/K500_MODE02_FLAGSHIP_R3_ACCEPTANCE.md`.

## 8. Balanced Enhanced Music Core candidate bank

The hardware-accepted preset anchors are now **01, 02, 03, and 05**. The shared
Music core itself was derived from the explicitly preferred 01/03/05 Music
results; Mode 02 has since accepted that core in its independently auditioned
broadcast context. The 01/03/05 seven-band gains occupy a very tight,
already-proven envelope:

| Music band | 01 | 03 | 05 | Balanced core |
| --- | ---: | ---: | ---: | ---: |
| 11.704 kHz air | +1.5 | +1.6 | +1.6 | **+1.6 dB** |
| 158 Hz punch/body | +3.0 | +3.1 | +2.9 | **+3.0 dB** |
| 66 Hz foundation | +8.2 | +8.2 | +8.0 | **+8.2 dB** |
| 1.315 kHz density/pocket | -0.9 | -0.8 | -1.0 | **-0.9 dB** |
| 2.916 kHz articulation | +1.3 | +1.3 | +1.3 | **+1.3 dB** |
| 6.495 kHz detail/polish | +1.0 | +1.2 | +1.2 | **+1.2 dB** |
| HS 766 Hz openness | +5.8 | +6.0 | +5.7 | **+5.8 dB** |

The proposed shared core is therefore:

```text
[+1.6, +3.0, +8.2, -0.9, +1.3, +1.2, +5.8] dB
```

Every value is inside the exact range already accepted on real K500 hardware;
this is an interpolation of accepted results, not a new extrapolated voicing.

On branch `preset/balanced-enhanced-bank-20261004`, Slots 04 and 06–10 remain candidate work. Slot 02 has since completed its independent broadcast-flagship hardware audition; historically that branch prepared Slots 02, 04 and 06–10 to
apply that shared Music core **only to the seven Music PEQ gain fields**.
Frequencies, Q, raw filter aliases, crossovers, Mic A/B, dynamics, Reverb,
Echo, routing, output EQ/gain, compressor, delays and unknown/reserved bytes
remain byte-identical to each slot's current official donor.

That intentionally preserves context:

- **02 MC HOST RADIO:** the enhanced backing-music core is now retained inside the accepted fresh/airy broadcast flagship; dry-vocal and booth-spatial evolution is documented separately.
- **04 POP ROCK BALLAD:** enhanced common Music foundation; existing rock/ballad vocal and spatial routing remains the context.
- **06 SHOLAWAT SYAHDU:** enhanced Music quality while the softer devotional Mic/FX architecture remains untouched.
- **07 JAZZ LOUNGE:** the common Music quality is adopted, while its lower Surround/Sub routing and intimate vocal architecture preserve lounge scale.
- **08 BLUES CLUB:** Music gains become current-generation while the warm vintage output/FX context remains donor-owned.
- **09 ACOUSTIC NATURAL:** common Music quality is added without rewriting its more restrained routing/output or natural vocal identity.
- **10 REGGAE DUB:** the Music path is modernized while its existing strong Sub route and rhythmic Echo/Dub architecture remain the genre authority.

Bank status:

| Slot | Repository file | Status / SHA-256 |
|---:|---|---|
| 01 | `01_KONSER_NYANYI.k500` | accepted, unchanged — `66ba788daadf56212e4de6673a702404cfc915dc934e98fe3b479ee972f978a1` |
| 02 | `02_MC_HOST_RADIO.k500` | **accepted flagship R3** — `adf3c868cfa471b4b0975bb58ffb4e8c05c071a7ed0896fc0a89b72933a88fa7` |
| 03 | `03_KAR_DANGDUT.k500` | accepted, unchanged — `ec683042962c635d8d87d262512a694797bd62842c07775e53eb497f34d329bc` |
| 04 | `04_POP_ROCK_BALLAD.k500` | **candidate** — `bf8dbbdb0f8f79cca5a299f9d1c824bfa2f4c21facae9b03564ba1cbf2998e2c` |
| 05 | `05_POP_KENANGAN_V2.k500` | accepted R2, unchanged — `e0d6e985f068576a37ea776c1ed730b63bafac44d3afc80ab31f8541ff6398b5` |
| 06 | `06_SHOLAWAT_SYAHDU.k500` | **candidate** — `9a4377cba86880ab18d0caf2933fe054749782355771eb221f396510bd7eb5cb` |
| 07 | `07_JAZZ_LOUNGE.k500` | **candidate** — `a61a84c62865b66c51fb2baef857611cfeb34be0e3e62adb6d88bce7df25dbfb` |
| 08 | `08_BLUES_CLUB.k500` | **candidate** — `ca15409e5c5dddfcf8678c5ec225495d76f3331421410fc71944208c1982c060` |
| 09 | `09_ACOUSTIC_NATURAL.k500` | **candidate** — `785c814bfc2bb3d6df7d03ca036dbd39bf36ce36a4acae13a095c5aeeae5c37b` |
| 10 | `10_REGGAE_DUB.k500` | **candidate** — `e0eb3825aec6ce17fff804f7c6c2b0575e0f4fd3d9c844cb3052a965c2c98e1f` |

Slot 02 has now passed its controlled real-K500 audition and is promoted as the exact accepted flagship bytes above. Slots 04 and 06–10 remain candidates; do not call those hardware-accepted or merge them to the public release line until their listening gates pass. Once accepted, promote exact bytes; do not rebuild them.

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
