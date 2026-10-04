# POP KENANGAN R2 — tempo-aware singability evolution

Date: 2026-10-04

## Why R2 exists

Real-hardware feedback on the R1 library candidate was clear:

- Mode 05 retained warmth but was **less enjoyable and less fresh to sing through** than Mode 01.
- For slow Pop Kenangan, Mode 05 should be the better choice, not merely the warmer one.
- The user specifically suspected that the slow tempo deserves a longer reverb decay.

R2 therefore does **not** chase a brighter analyzer curve. It redesigns the vocal
experience around the actual tempo and around the proven K500 output roles:

`clear/direct center + lush tempo-aware tail + halo in surround`.

## Tempo analysis of the supplied song

The supplied `05 POP KENANGAN.wav` is 168.48 s long.

Onset/beat analysis produces a double-time solution near 145 BPM. Interpreting the
musical pulse at half-time gives approximately **72.8-73 BPM**, consistent across
autocorrelation/beat-grid checks.

At ~72.8 BPM:

| Musical value | Approx. time |
| --- | ---: |
| quarter note | 824 ms |
| eighth note | 412 ms |
| dotted sixteenth | 309 ms |
| 1/64 | 51.5 ms |
| dotted 1/64 | 77.3 ms |
| 3 quarter-note beats | 2473 ms |
| whole bar (4/4) | 3297 ms |

### Important finding: the existing echo is already right

The R1 echo delay is **310 ms**.

At the measured tempo, dotted-sixteenth sync is ~**309 ms**.

So the correct engineering action is to **LOCK the 310 ms echo delay**, not change
it just because the preset needs improvement.

## Reverb design

R1:

- decay 2200 ms ~= 2.67 beats;
- predelay 60 ms;
- HPF 220 Hz.

R2:

- decay **2470 ms** ~= 3 beats;
- predelay **75 ms**, close to dotted-1/64 at this tempo;
- HPF **240 Hz**.

The longer decay gives slow phrases a more emotionally rewarding tail. The longer
predelay deliberately keeps the consonant/dry note in front before that tail blooms.
Raising the wet HPF slightly prevents the longer tail from accumulating chest/mud.

## Spatial reroute: clearer singer, larger halo

The biggest structural issue in R1 was that Mode 05 was wetter in the Center and
carried more direct Mic in Surround than the enjoyable Mode 01 architecture.

R2 changes the *distribution* of wetness rather than simply adding more wet level:

| Route | R1 | R2 |
| --- | ---: | ---: |
| Main Reverb | 95 | 96 |
| Center Mic | 94 | 97 |
| Center Reverb | 87 | 82 |
| Center Echo | 43 | 38 |
| Surround Mic | 80 | 74 |
| Surround Reverb | 96 | 100 |
| Surround Echo | 58 | 55 |
| Surround delay L/R | 10/14 ms | 12/18 ms |

Intent:

- **Center becomes the singer anchor.**
- **Surround becomes ambience/halo, not a second dry singer.**
- Main remains lush.
- The longer reverb is heard around the singer without pulling the voice backward.

This keeps Mode 05 more romantic/wet than Mode 01, while adopting the same successful
mental model.

## Vocal freshness micro-EQ

R2 does not turn the Mic bright. It removes only the excess warm/box density that made
R1 feel stale.

Mic A and B remain linked and receive the same changes:

- 130 Hz: -2.2 -> **-2.5 dB**
- LS 470 Hz: +6.3 -> **+5.8 dB**
- 444 Hz: -0.5 -> **-0.8 dB**
- 1550 Hz: -1.0 -> **-0.7 dB**
- 3900 Hz: -2.2 -> **-2.4 dB** (harshness guard remains)
- 6300 Hz: -0.7 -> **-0.5 dB**
- 9270 Hz: -6.4 -> **-6.2 dB**

The modeled dry-Mic delta is about:

- **-0.76 dB** body 120-250 Hz;
- **-0.70 dB** mud 250-500 Hz;
- **+0.07 dB** cue 1-2.5 kHz;
- **+0.17 dB** detail 5-7 kHz;
- **+0.11 dB** air 10-14 kHz.

So this is a cleanup/freshness move, not a treble boost.

## Music refinement

The backing music remains intentionally warmer than Mode 01.

R2 only restores a little luxury texture:

- 11.704 kHz: +1.4 -> **+1.6 dB**
- 158 Hz: +2.8 -> **+2.9 dB**
- 2.916 kHz: +1.2 -> **+1.3 dB**
- 6.495 kHz: +1.0 -> **+1.2 dB**

66 Hz remains **+8.0 dB** and 1315 Hz remains **-1.0 dB**.

The modeled Music/Main change is only about +0.18 dB in detail/air. The purpose is
to prevent "warm" from becoming "closed."

## Wet tonal polish

Reverb high-frequency EQ is opened only slightly:

- 5.55 kHz: -10.4 -> **-9.9 dB**
- 10.5 kHz: -14.0 -> **-13.6 dB**
- HS 1555 Hz: +8.3 -> **+8.2 dB**

Echo EQ and the tempo-perfect 310 ms echo delay are preserved.

## Simulation result

R2 vs R1 modeled Main vocal composite:

- body 120-250 Hz: **-0.94 dB**
- mud 250-500 Hz: **-0.78 dB**
- cue 1-2.5 kHz: **+0.10 dB**
- i/ring 2.5-4.5 kHz: **~0 dB**
- detail 5-7 kHz: **+0.29 dB**
- air 10-14 kHz: **+0.18 dB**

This is exactly the desired direction:

`less stale/thick -> clearer singer -> longer emotional tail -> retained warmth`.

## Hardware acceptance

Use the same slow Pop Kenangan song and same gain chain.

R2 passes only if:

1. singing feels easier/fresher than R1 **and** Mode 01;
2. the first syllable/consonant remains clear despite the longer decay;
3. the tail feels romantic and rewarding, not washy;
4. Center vocal remains stable and present;
5. Surround is perceived as a halo, not a second singer;
6. echo feels rhythmically natural — it should, because 310 ms is already tempo-aligned;
7. music stays warm without sounding closed;
8. no 200-500 Hz mud appears after 20-30 minutes.

If accepted, promote the **exact R2 bytes**; do not rebuild after hardware acceptance.
