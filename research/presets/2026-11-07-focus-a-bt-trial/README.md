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
