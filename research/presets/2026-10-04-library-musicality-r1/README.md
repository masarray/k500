# K500 Official Library — Musicality Evolution R1

Date: 2026-10-04

## Status

This folder is the R1 research bank. **Slot 01 and Slot 03 have completed the final real-K500 listening gate and their exact accepted bytes are promoted. Slot 05 R1 was superseded by the separately documented, hardware-accepted R2. Slots 02, 04, and 06–10 remain hardware-audition candidates and are not official.**

The empirical anchor is the real-K500 accepted direction from
`KONSER NYANYI Main Balance V2`: the user reported that V2 is more enjoyable
after the Main-output capture moved to a much healthier Low / Mid / High balance.

The lesson is transferred as an **engineering approach**, not as one identical EQ
curve.

## Common musicality approach

Every mode is evolved toward:

`foundation -> punch -> body -> clean mid -> smooth detail -> retained air`

with these guardrails:

1. Do not solve missing punch by endlessly increasing deep sub.
2. Prefer a small 158 Hz kick/body correction before more 66 Hz.
3. Relax backing-mid density only enough to create a singer pocket.
4. Reduce broad/cymbal dominance only where the donor is already bright.
5. Preserve air; do not make the library dull.
6. Keep each preset's vocal/FX/spatial identity intact.

## Why only Music PEQ in R1

Mode 01 proved that the reported balance problem was solved effectively by a
small Music-path correction. Changing shared Main EQ would also alter Mic dry,
Reverb and Echo returns, creating unnecessary regression risk.

Therefore this round changes **Music PEQ gain fields only**.

Unchanged in every candidate:

- Music frequencies, Q and raw PEQ type aliases;
- Mic A/B and vocal dynamics;
- Reverb and Echo EQ/timing/levels;
- Main/Center/Surround/Sub EQ and routing;
- output compressors and delays;
- crossovers;
- every Alt/reserved/unknown byte;
- all names and non-audio/system fields.

No Sub EQ is changed in this round.

## Context-specific strategy

| Slot | Mode | Evolution intent |
| ---: | --- | --- |
| 01 | KONSER NYANYI | Accepted V2 reference: strongest balanced concert authority, smooth top |
| 02 | MC HOST RADIO | Same balance benefit but slightly restrained so speech utility stays dominant |
| 03 | KAR DANGDUT | Strongest 158 Hz punch of the set; preserve more sparkle for percussion/groove |
| 04 | POP ROCK BALLAD | Kick/guitar impact plus a small vocal pocket; keep articulation |
| 05 | POP KENANGAN V2 | Warmer nostalgia; more body, softer upper energy, no heavy sub rewrite |
| 06 | SHOLAWAT SYAHDU | Gentle foundation and very smooth top so long devotional sessions stay soft |
| 07 | JAZZ LOUNGE | Do not transplant concert voicing; only add small acoustic-bass/kick body |
| 08 | BLUES CLUB | Add groove/weight while preserving warm vintage restraint |
| 09 | ACOUSTIC NATURAL | Minimal intervention: tiny 158 Hz support, essentially preserve natural top |
| 10 | REGGAE DUB | Deep bottom already strong; lock 66 Hz and add upper-bass groove instead |

## Simulation summary: candidate minus current official donor

Values are comparative model averages from the repository RBJ-style simulator,
not exact acoustic measurements.

| Slot | 45-90 Hz | 90-180 Hz | 120-250 Hz | 1.5-2.5 kHz | 2.5-4.5 kHz | 5-7 kHz | 10-14 kHz |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 01 | +0.61 | +0.78 | +0.69 | -0.65 | -0.72 | -1.10 | -0.98 |
| 02 | +0.40 | +0.53 | +0.47 | -0.57 | -0.61 | -0.93 | -0.87 |
| 03 | +0.61 | +0.84 | +0.76 | -0.46 | -0.50 | -0.74 | -0.68 |
| 04 | +0.41 | +0.64 | +0.58 | -0.54 | -0.61 | -0.92 | -0.79 |
| 05 | +0.40 | +0.52 | +0.45 | -0.79 | -0.88 | -1.22 | -1.15 |
| 06 | +0.29 | +0.34 | +0.27 | -0.77 | -0.88 | -1.22 | -1.15 |
| 07 | +0.22 | +0.41 | +0.40 | -0.11 | -0.09 | -0.09 | -0.02 |
| 08 | +0.21 | +0.36 | +0.34 | -0.11 | -0.10 | -0.10 | -0.10 |
| 09 | +0.02 | +0.21 | +0.23 | -0.08 | -0.09 | -0.10 | -0.10 |
| 10 | +0.03 | +0.32 | +0.34 | -0.22 | -0.19 | -0.20 | -0.20 |

This intentionally creates two families:

- Slots 01-06 share the legacy Hi-Fi music core and receive the stronger,
  empirically justified rebalance.
- Slots 07-10 already have deliberately restrained genre-specific music cores,
  so they receive only micro-evolution rather than normalization.

## Remaining hardware acceptance order

Slots 01 and 03 are accepted. Slot 05 R1 is superseded by accepted R2.

To avoid subjective drift, audition the remaining R1 candidates in this order:

1. 09 ACOUSTIC NATURAL — verifies that minimal modes remain natural.
2. 10 REGGAE DUB — verifies no excess deep-bass accumulation.
3. 02 MC HOST RADIO — verifies utility identity remains dry/controlled.
4. 04 POP ROCK BALLAD.
5. 06 SHOLAWAT SYAHDU.
6. 07 JAZZ LOUNGE.
7. 08 BLUES CLUB.

Use the same source tracks, interface gain, K500 output, speaker level and room
position whenever possible.

A candidate passes only if the intended improvement is audible without causing:

- boom at 120-250 Hz;
- loss of singer intelligibility;
- dull cymbals/air;
- fatigue;
- loss of the preset's genre identity.

Promote **exact accepted bytes** only; do not rebuild after hardware acceptance.
