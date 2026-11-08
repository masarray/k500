# R5 — Artist 01 sweet-spot update, 2026-11-08

**Owner hardware A/B:** R4 sounds pleasant but E9 has more vocal standout; E9 feels slightly excessive in vocal mids. Binary audit found **the sole audio difference** is Echo repeat 10 versus 9, with EQ identical.

Research bank 01 now targets E9 repeat 9 with a small Main Echo return change (95 -> 94) plus minor Echo-only EQ adjustments (bell3 −12.4 -> −12.8 dB, shelf5 +11.7 -> +11.4 dB). All Mic dry / Music / Main / Reverb EQ and compressor are untouched.

Variants and detailed A/B instructions: `refinement-r5/README.md`. Slots 02–10 are **identical**, including accepted Melayu R3 and owner-favorite Ballad ALT. Native .k500 bytes, checksum, PEQ and Bluetooth audit passed. The RBJ 48 kHz Echo path comparison is **only a relative model**; no firmware impulse, room acoustics or physical test proven.

PR remains Draft; official 41 presets, Golden #41, Windows stable build and CI are untouched.

---
# R4 update (2026-11-07) — Artist 01 effects ringing at 2–2.5kHz

**Owner feedback:** Melayu R3 sounds good and is frozen; Artist 01 now has mild wet-effect ringing around 2–2.5 kHz. Ballad ALT remains the owner's preferred Ballad.

**Trial bank update:** Slot 01 becomes `F1 ARTIST R4`, using the identical wet Reverb PEQ band2/band5 geometry that made Melayu R3 good. Mic A/B, Echo EQ, Music, Main, routing, compressor and time settings on Artist remain unchanged. All other nine bank files are byte-for-byte unchanged. `refinement-r4/` contains Artist original rollback, primary R4, Soft and Echo9 alternatives.

Comparative 48 kHz RBJ magnitude model for Artist R4 vs Artist original: wet Reverb **−3.835 dB** over 2.0–2.5 kHz; Main vocal energy proxy **−1.108 dB** over 2.0–2.5 kHz; dry Mic signal **0.000 dB change**. This is a model, **not** measured K500 acoustic response or feedback cure.

**Do not promote into official `resources/presets` or change stable installers** before real K500 listening acceptance. Further details: `refinement-r4/README.md`.

---
# 2026-11-07 R3 — K500 listening-feedback override

**Melayu R2:** user hears vocal clearly and finds it increasingly enjoyable, but reports effect ringing near 2 kHz. R3 retains all dry Mic A/B, Music, Main, Echo, dynamics, routing and timing, and changes only two Reverb EQ bands to reduce wet resonance in the 1.7–2.3 kHz range.

**Ballad:** user explicitly prefers R2 **ALT** over regular R2. The exact existing Ballad ALT binary is now the default 10th trial file, and SILK R3 is only an optional A/B experiment.

Four R3 audition files, hypothesis limitations, reproducibility hashes, and listening instructions are documented in `refinement-r3/`.

Source master/official 41-entry library/Golden #41/owner-selected radio #02/installer/workflows are unchanged. Model comparisons are not physical K500 listening proof.

---
# R2 IMPORTANT: owner feedback for slots 07 and 10

R1 Ballad was not enjoyable and R1 Melayu was somewhat lacking on real K500. Their trial files here are NOW R2 (not accepted yet), with near-original preferred FOCUS A vocal EQ/echo. Other eight trial files are byte-for-byte unchanged. See refinement-r2/ for both ALT variants. Historical R1 descriptions below are superseded for 07 and 10.

---

# K500 FOCUS A BT — Trial Preset Bank 01–10

**Research only — not yet physically auditioned on a K500.** The exact owner-supplied KTVPRO FOCUS A preset is kept in source/ unchanged. The original source mode is UAUDIO (raw 5); all ten research trials use Bluetooth (raw 2).

## Ten native slot candidates

| Slot | Native name | Purpose |
|---:|---|---|
| 01 | F1 ARTIST BT | Anchor FOCUS A vocal for singing |
| 02 | F2 MC RADIO BT | Drier speech, echo OFF |
| 03 | F3 DANGDUT BT | Rhythmic, controlled repeats |
| 04 | F4 ROCK BT | Tighter tail, clearer articulation |
| 05 | F5 ACOUSTIC BT | Close and natural |
| 06 | F6 JAZZ BT | Relaxed dynamics, warm club tone |
| 07 | F7 MELAYU BT | Long gentle vocal tails |
| 08 | F8 QORI BT | Vocal anchor, spacious, lower backing level |
| 09 | F9 ADZAN BT | Long reverb, quiet backing, near-zero echo |
| 10 | F10 BALLAD BT | Lush ballad, softer echo feedback |

## Exact-source and safety rules

- Every trial file is 1,144 bytes and must have additive checksum modulo 256 == 0, native name <=16 chars, source raw 2, and compatible PEQ fields.
- Each file derives from the owner's exact 1,144-byte donor SHA-256 e48e45d5cdbd66aa6b80609d1ad48987fff48eddb5673e2fc893e53e463ddfdb, checked bit-for-bit.
- Slot 01 preserves all donor DSP bytes; only source mode, visible native name and checksum are modified.
- Other genre variants use small, intentionally different proven scalar and EQ-gain writes; all EQ type/frequency/Q, crossover/mirror/Alt/reserved/unknown bytes remain untouched.
- patches.json carries the complete edits and each expected SHA-256. generate.py --check confirms native files regenerate byte-identically, validating checksum and protected regions.
- No changes to official resources/presets, official 01–10 UAUDIO, owner-selected Velvet R1 #02, protected GOLDEN #41, app runtime, CI, installer or previous releases.

## Offline simulation completed

- Comparative 48 kHz RBJ frequency-response software model, output wet-branch energy proxy and narrowband headroom risk review.
- Ten WAV samples of original synthetic/harmonic test signals are included in the user-delivered trial ZIP alongside plots, per-band metrics and audit JSON.
- These are NOT real voice recordings or physical K500 firmware DSP simulation; model peaks are not actual clipping measurements. The donor contains very strong shelf EQ, so audition from a low physical volume.
- All files require actual device upload/recall/readback and singing tests before official promotion.

## Physical audition workflow

1. Back up the hardware 10-slot bank. In K500 application use LOCAL preset import / explicit physical slot staging; selecting a PC entry does not necessarily write or recall hardware.
2. Load 01–10 in physical K500 slots deliberately, then read back and confirm Bluetooth mode/name.
3. Compare 01/03/10 for singing with same song, BT source output, microphone distance, gain and listening volume. Keep room and speakers constant.
4. Evaluate 04/05/06/07 for tighter or natural genres; 02 for speech, 08 for Quran recitation, 09 for adzan. Check harsh sibilance, pitch cue, echo smear, feedback, bass masking and fatigue.
5. Return listening feedback before committing to official bundled presets.

Do not treat a software-proxy improvement as hardware proof. The hardware remains the authority.
