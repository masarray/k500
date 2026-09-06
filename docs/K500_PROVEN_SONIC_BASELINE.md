# K500 PROVEN SONIC BASELINE

> **Purpose:** permanent sonic handoff for a new ChatGPT/Codex/AI thread.
>
> **Read this before starting any K500 preset tuning.** This document captures the currently proven listening target, the Hi-Fi music reference, the successful tuning sequence, the transplant strategy across modes, and the areas that must remain locked unless new hardware evidence says otherwise.
>
> **Last consolidated:** 2026-09-06
>
> **Authority:** real K500 hardware listening wins over simulation. Simulation is used to understand causality, relative energy, and safe transplant compensation.

---

# 1. Product-level sonic goal

The K500 preset library is not intended to provide only genre EQ or maximum volume. The product goal is:

> **Every mode should give the listener an obvious, premium music-enhancement benefit while preserving the vocal identity of that mode.**

The target is **maximum enjoyment / maximum enhancement**, not maximum loudness.

A successful K500 music preset should feel:

```text
deep
+ round
+ punchy
+ clean
+ textured
+ detailed
+ silky
+ wide
+ solid
+ non-fatiguing
```

The user should hear that the processor improves playback quality across the whole audible spectrum:

- very-bottom energy is present and satisfying;
- kick and bass have physical impact;
- midrange has texture, body, separation and musical presence;
- original song vocals, guitars, piano, synths and snare are easier to distinguish;
- upper detail is clear without 3-4 kHz aggression;
- treble has gloss, sparkle and air without becoming cheap or sharp;
- stereo scale is larger, while center image remains coherent;
- the result feels more Hi-Fi and polished without needing a large loudness increase.

Do **not** interpret "enhance" as "boost every band". Enhancement comes from spectral contrast, routing, depth, width, and controlled detail.

---

# 2. Current gold sonic reference

The current **hardware-approved music reference** is:

```text
resources/presets/03_DANGDUT_SUPREME.k500
```

Current internal/hardware name:

```text
DGT HIFI CORE V3
```

This file was developed from Mode 03 through three Hi-Fi music-core revisions.

## 2.1 Hardware listening result

User hardware feedback after V3:

```text
"oke ini enak nikmat"
```

The important progression was:

### Core V1

- bass became enjoyable;
- treble became reasonably good;
- mid still felt ordinary.

### Core V2

Mid-specific refinement was added while bass was locked.

User feedback:

```text
mid nikmat
```

### Core V3

A small very-bottom extension was added without changing the already successful mid.

User feedback:

```text
oke ini enak nikmat
```

Therefore:

> **03 Hi-Fi Core V3 is the current GOLD MUSIC REFERENCE. Do not casually retune its bass/mid architecture.**

If a new experiment sounds worse, branch back from this reference.

---

# 3. Hi-Fi Music Core V3 — exact proven tonal core

The Music PEQ section is at `0x01B0`.

The current proven Music EQ is:

| Band | Type | Frequency | Q | Gain |
|---:|---|---:|---:|---:|
| 1 | P | 11704 Hz | 2.0 | **+1.8 dB** |
| 2 | P | 158 Hz | 2.3 | **+2.3 dB** |
| 3 | P | 66 Hz | 0.4 | **+7.6 dB** |
| 4 | P | 1315 Hz | 2.4 | **-0.6 dB** |
| 5 | P | 2916 Hz | 2.1 | **+1.3 dB** |
| 6 | P | 6495 Hz | 1.9 | **+1.5 dB** |
| 7 | HS | 766 Hz | 0.4 | **+6.5 dB** |

Raw gain values in tenths of dB:

```text
[18, 23, 76, -6, 13, 15, 65]
```

## Why this balance works

### 66 Hz +7.6 dB

Provides broad low-end excitement and weight, but was intentionally reduced from +8.0 dB when the dedicated Sub path was strengthened.

Do not keep increasing this band to get more "bottom". Use the Sub path for extra very-bottom depth.

### 158 Hz +2.3 dB

Provides roundness and physical punch/body. This is important for kick, bass-body and musical satisfaction.

### 1315 Hz -0.6 dB

This band was one of the key V2 changes. Earlier, at -1.2 dB, the user described the mid as ordinary. Moving to -0.6 dB made the mid more textured and enjoyable without making it shouty.

### 2916 Hz +1.3 dB

Adds definition and articulation, but remains restrained enough to protect the 2.5-4.5 kHz harshness guardrail.

### 6495 Hz +1.5 dB

Adds musical micro-detail and polish.

### 11704 Hz +1.8 dB

Adds air and top-end luxury without relying only on a broad brightness shelf.

### HS 766 Hz +6.5 dB

Provides broad openness/excitement. Do not blindly raise it further. Future refinement should prefer targeted detail bands before more broad shelf energy.

---

# 4. Proven Sub foundation

The Sub EQ section is at `0x0368`.

The current Hi-Fi Core V3 low-frequency reference is:

| Band | Frequency | Gain |
|---:|---:|---:|
| 1 | 53 Hz | **+2.9 dB** |
| 2 | 65 Hz | **+4.3 dB** |
| 3 | 80 Hz | **+1.0 dB** |
| 4 | preserve donor | **0.0 dB** |
| 5 | preserve donor | **0.0 dB** |

Raw gain values:

```text
[29, 43, 10, 0, 0]
```

The key learning from V3 is:

> When bass and punch are already enjoyable, add very-bottom satisfaction by making a **small 53-65 Hz Sub change**, not by raising the entire Sub route or adding more 80-150 Hz energy.

The successful V3 change from V2 was only:

```text
53 Hz  +2.4 -> +2.9 dB
65 Hz  +4.0 -> +4.3 dB
80 Hz  stayed +1.0 dB
Sub route stayed locked
```

Comparative proxy showed the largest added energy in approximately 45-75 Hz, while 95-120 Hz changed much less.

This produced more bottom-end satisfaction without destroying the already-good mid or making upper bass boomy.

---

# 5. Bass design rule: broad bass and dedicated Sub have different jobs

Use this architecture:

```text
Music PEQ / Main path = broad musical bass + punch + body
Sub path              = very-bottom foundation / physical depth
```

Do not solve every bass request by raising 66 Hz or the Sub output.

Preferred decision order:

1. If music lacks **deep foundation**, inspect 45-75 Hz Sub energy.
2. If music lacks **kick/body impact**, inspect 90-180 Hz / 158 Hz region.
3. If music sounds **boomy**, do not add more low end; inspect 45-100 Hz overlap and room/sub behavior.
4. If bass is already "enak", **LOCK IT** and fix the actual weak area instead.

---

# 6. Mid design rule: the "enjoyable mid" lesson

The most important recent hardware lesson is that a preset can have excellent bass and decent treble but still feel ordinary if the midrange lacks texture.

The successful V2 refinement was intentionally small:

```text
1315 Hz  -1.2 -> -0.6 dB
2916 Hz  +1.0 -> +1.3 dB
6495 Hz  +1.4 -> +1.5 dB
11704 Hz +1.6 -> +1.8 dB
```

Bass, Sub, routing, vocal, compressor and FX were locked.

Approximate comparative intent:

```text
700-1200 Hz     +small texture
1200-2000 Hz    +moderate presence / body information
2000-3500 Hz    +controlled definition
3500-5000 Hz    +very small change only
5000-12000 Hz   +micro-detail / polish
```

This is the preferred way to make the mid feel more premium:

> **increase information density and texture, not broad mid loudness.**

Do not make 3-4 kHz the universal clarity knob.

---

# 7. Treble design rule: luxury detail, not cheap brightness

For premium K500 playback:

```text
2.5-4.5 kHz = guardrail / definition
5-7 kHz     = harmonic detail / polish
7-10 kHz    = sparkle
10-14 kHz   = air / openness
```

If the user asks for more detail:

1. protect 3-4 kHz from excessive boost;
2. prefer small targeted changes around the existing 6.5 kHz and 11.7 kHz bands;
3. preserve good mid texture;
4. hardware-check cymbal harshness and listening fatigue.

A premium result is **silky and revealing**, not simply brighter.

---

# 8. Music quality should be universal across modes

The library strategy is now:

```text
Mode identity = vocal architecture + FX/spatial vocal behavior
Music quality = shared premium Hi-Fi target
```

Examples:

```text
Mode 01 = Hi-Fi Music + Concert vocal
Mode 02 = Hi-Fi Music + Broadcast/Host vocal
Mode 03 = Hi-Fi Music + Dangdut vocal
Mode 04 = Hi-Fi Music + Rock vocal
Mode 05 = Hi-Fi Music + Pop Kenangan vocal
Mode 06 = Hi-Fi Music + Qori/Sholawat vocal
```

A user should not lose music quality merely because they switch vocal mode.

---

# 9. Do not blindly copy route numbers across modes

Output gains differ between presets. Therefore identical route values can produce different effective energy.

For comparative route matching, use:

```text
Effective route amplitude ~= route * 10^(output_dB / 20)
```

To compare two modes:

```text
relative_dB = 20 * log10(E_candidate / E_reference)
```

This is a useful **routing proxy**, not a complete acoustic model.

The goal is to match the music-energy architecture while preserving each mode's vocal behavior.

---

# 10. Current Core V3 effective music-routing reference

Mode 03 current reference:

| Path | Output gain | Music route |
|---|---:|---:|
| Main | +12 dB | **100** |
| Surround | +9 dB | **78** |
| Center | +6.5 dB | **48** |
| Sub | +12 dB | **96** |

This is the current target effective music-energy architecture.

The raw route number does **not** need to be identical if the output gain differs.

---

# 11. Current preset 01-06 Hi-Fi transplant map

The repository currently contains the following music architecture.

## Mode 01 — `01_ALL_GENRE.k500`

Current target:

```text
Main      +12 dB / Music 100
Surround   +9 dB / Music 78
Center    +6.5 dB / Music 48
Sub       +12 dB / Music 96
```

Effective music routing is approximately identical to Core V3.

Vocal/Concert architecture must remain independent from music tuning.

## Mode 02 — `02_BROADCAST.k500`

Current target:

```text
Main      +12 dB / Music 100
Surround   +9 dB / Music 78
Center      +8 dB / Music 40
Sub       +12 dB / Music 96
```

The Center route is lower because Center output is +8 dB; its effective music energy is approximately matched to the Core V3 reference.

### Critical Broadcast compensation lesson

The old Broadcast Surround output was +6 dB. To give music premium scale without making the host ambience 3 dB louder, the successful transplant strategy was:

```text
Surround output +6 -> +9 dB
Music route       70 -> 78

Mic route         42 -> 30
Reverb route      68 -> 48
Echo route         8 -> 6
```

The non-music route compensation follows approximately:

```text
new_route ~= old_route * 10^(-3/20)
```

This keeps host ambience approximately stable while increasing music scale.

**General lesson:** do not make Broadcast music small just to protect the host. Preserve host dominance through source-specific routing compensation.

## Mode 03 — `03_DANGDUT_SUPREME.k500`

Gold reference:

```text
Main      +12 dB / Music 100
Surround   +9 dB / Music 78
Center    +6.5 dB / Music 48
Sub       +12 dB / Music 96
```

This is the current hardware-approved sonic reference.

## Mode 04 — `04_ROCK.k500`

Current compensated target:

```text
Main      +12 dB / Music 100
Surround   +8 dB / Music 88
Center    +6.5 dB / Music 48
Sub       +12 dB / Music 96
```

The higher raw Surround Music route compensates for the lower +8 dB Surround output.

Relative routing proxy vs Core V3 is approximately:

```text
Main      0.00 dB
Surround +0.05 dB
Center    0.00 dB
Sub       0.00 dB
```

## Mode 05 — `05_POP_KENANGAN.k500`

Current target:

```text
Main      +12 dB / Music 100
Surround   +9 dB / Music 78
Center    +6.5 dB / Music 48
Sub       +12 dB / Music 96
```

Routing proxy is essentially identical to Core V3.

## Mode 06 — `06_QORI_SHOLAWAT.k500`

Current compensated target:

```text
Main      +12 dB / Music 100
Surround +8.5 dB / Music 83
Center    +7.5 dB / Music 43
Sub       +12 dB / Music 96
```

Relative routing proxy vs Core V3 is approximately:

```text
Main      0.00 dB
Surround +0.04 dB
Center   +0.04 dB
Sub       0.00 dB
```

This is why route values differ while the intended effective music scale remains nearly the same.

---

# 12. Confidence / validation status

Do not confuse modeled transplant with hardware-proven sonic approval.

## Hardware-proven GOLD

```text
03_DANGDUT_SUPREME.k500 / DGT HIFI CORE V3
```

The final bass, mid and very-bottom progression was directly approved through listening.

## Transplanted / needs per-mode hardware A/B

Current Hi-Fi music transplant exists in:

```text
01_ALL_GENRE.k500
02_BROADCAST.k500
04_ROCK.k500
05_POP_KENANGAN.k500
06_QORI_SHOLAWAT.k500
```

Their music PEQ/Sub target and effective routing were engineered to approach the Core V3 reference while preserving each mode's vocal architecture.

They still require per-mode hardware verification because:

- output PEQ differs by mode;
- crossover values can differ by donor;
- room and speaker interaction is real;
- vocal + music masking can differ by mode;
- modeled effective route equality does not guarantee identical acoustics.

If hardware feedback says one transplant is worse, **do not modify the gold Core V3 first**. Fix the mode-specific compensation.

---

# 13. Locking policy after positive hardware feedback

Whenever the user says an area is good, explicitly LOCK it.

Example progression from Core V1-V3:

```text
V1: bass good -> LOCK bass
V2: mid good  -> LOCK mid
V3: bottom good / overall enjoyable -> GOLD candidate
```

Next revisions should change only the unresolved problem.

Do not "improve everything" after hardware reports that a region is already enjoyable.

---

# 14. Preferred tuning sequence for a new mode

Use this workflow instead of starting from zero.

## Step 1 — start from the current repository donor

Never reconstruct the preset from memory.

## Step 2 — preserve vocal identity

Unless the user explicitly asks to change singing/host behavior, LOCK:

- Mic A/B EQ;
- Mic dynamics;
- vocal direct routing;
- Reverb/Echo timing;
- Reverb/Echo EQ;
- compressor settings;
- output EQ/crossover architecture;
- Alt blocks;
- unknown/reserved bytes.

## Step 3 — apply the Hi-Fi tonal core

Use the proven Core V3 Music EQ and Sub EQ as the starting reference.

## Step 4 — calculate output-aware music routing

Match effective Main/Surround/Center/Sub music energy to Core V3 using output gain compensation.

Do not blindly copy route values.

## Step 5 — if an output gain must change, compensate non-music routes

If output gain is changed to give music the correct scale, adjust Mic/Reverb/Echo routes so vocal ambience remains approximately unchanged.

Mode 02 is the reference example.

## Step 6 — validate binary safety

Mandatory:

```text
size = 1144 bytes
checksum sum(bytes) % 256 = 0
name <= 16 visible characters
checksum written last
unexpected byte diff = none
```

## Step 7 — hardware A/B

Use the same song and approximately matched playback level.

Evaluate:

```text
VERY BOTTOM
- depth / floor / physical foundation

BASS / PUNCH
- roundness
- kick impact
- bass-note definition
- boom

MID
- vocal texture in the source music
- guitar/piano/synth separation
- snare body
- richness vs shout

DETAIL / TREBLE
- definition
- cymbal polish
- sparkle
- air
- harshness / fatigue

SPATIAL
- front solidity
- stereo scale
- side energy
- center coherence

GLOBAL
- more enjoyable without simply being louder
- can listen for 10-20 minutes without fatigue
```

## Step 8 — make one small causal revision

Examples:

```text
bass already good, mid ordinary
-> do not touch bass; refine 1.3 / 2.9 / 6.5 / 11.7 kHz

mid already good, bottom needs a little more
-> do not touch Music EQ; add a small 53/65 Hz Sub refinement

music sounds small in one mode
-> inspect output-aware routing before changing EQ
```

---

# 15. Things that repeatedly produce worse results

Avoid these unless hardware evidence specifically demands them.

## 15.1 More volume mistaken for enhancement

Do not raise all routes or broad EQ simply to create a louder first impression.

## 15.2 Broad bass stacking

Do not simultaneously keep increasing:

```text
66 Hz Music EQ
+ Sub EQ
+ Sub Music route
+ Sub output
```

Choose the correct layer for the problem.

## 15.3 3-4 kHz as a generic detail knob

This can make music and vocal sharp, tiring and cheap.

## 15.4 Copying Mode 03 raw routes blindly

If output gain differs, the same route number does not produce the same effective energy.

## 15.5 Fixing host/vocal masking by making music low quality

Keep music premium; solve priority through source-specific Center/Surround routing.

## 15.6 Changing compressor before fixing tonal/routing problems

The successful Hi-Fi Core progression did not require changing the Main compressor. Tonal balance and routing solved the perceived problems first.

## 15.7 Changing vocal architecture during a music-only request

A music-quality request is not permission to redesign Mic, Reverb or Echo.

---

# 16. Current golden heuristic

When a future user asks:

```text
"buat musik lebih mewah / polished / detail / nendang / hi-fi / enak"
```

Do **not** start from a generic EQ recipe.

Start from this hypothesis:

```text
1. Compare against 03 Hi-Fi Core V3.
2. Preserve any already-good vocal architecture.
3. Use Core V3 Music EQ/Sub EQ as the tonal reference.
4. Match effective output-aware music routing.
5. Protect 3-4 kHz.
6. Use 5-7 kHz and 10-14 kHz for premium detail/air.
7. Use dedicated Sub 53-65 Hz for extra very-bottom satisfaction.
8. Use ~158 Hz for roundness/punch rather than solving everything with deep sub.
9. Make one small revision at a time after hardware feedback.
10. Treat positive user listening feedback as a LOCK instruction.
```

---

# 17. Thread-start checklist for AI

Before answering a new K500 preset-tuning request, confirm internally:

```text
[ ] I read AGENTS.md
[ ] I read this proven sonic baseline
[ ] I read the engineering playbook
[ ] I read the bit-perfect guide
[ ] I know which repository preset is the current donor
[ ] I know 03 Hi-Fi Core V3 is the current gold music reference
[ ] I know hardware listening outranks simulation
[ ] I know what the user already said was enjoyable and must remain locked
[ ] I will patch only proven offsets
[ ] I will diff-audit every output
```

If the thread has no prior conversation history, this document is the starting sonic state. Do not restart the research from a generic karaoke preset.
