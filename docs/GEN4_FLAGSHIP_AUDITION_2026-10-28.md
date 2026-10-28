# GEN4 V2 — Audible K500 Singing Tuning (2026-10-29)

Status: **engineering candidates for hardware audition**, not validated flagship sound.
This revision supersedes the tiny-delta GEN4 V1 candidates in PR #217. The exact owner-preferred GEN3 01–10 and accepted legacy 21–30 stay byte-identical. PR #216 remains the prerequisite.

## Design intention

The GEN3 listening feedback is the controlling subjective reference: stronger perceived body, less horn-like upper-mid harshness and more integrated karaoke FX than former repo defaults. The earlier GEN4 V1 model changed the Mic→Main response by just ~0.4–1.0 dB and inherited dry Podcast/speech presets. GEN4 V2 deliberately applies **1.0–3.5 dB per-band changes** across Mic A/B, Music and Main, plus warranted FX controls. All equalizer frequencies, Q, type aliases, the extreme but owner-preferred shelf values, crossovers, alt EQ and unknown firmware bytes are held fixed.

The mix priorities are a controlled vocal push around 1–2.5 kHz, a modest reduction in the combined 2.5–4.5 kHz harsh zone, less upper-music competition, and nonuniform (genre-specific) reverb/echo treatment. None of those approximate-frequency hypotheses proves subjective improvement without a real device A/B session.

## Library / identity

| PC | GEN4 V2 audition | locked GEN3 donor | key difference |
|---|---|---|---|
| 31 | Artist | 01 Artist | Lead separation, smoother edge, shorter reverb |
| 32 | Ballad | 02 Podcast | **Rebuilt** very-dry speech FX as ballad; Main music routing 30→46; Main L/R output trim −2 dB |
| 33 | Dangdut | 03 Dangdut | Kick/body shaping, warm vocal presence |
| 34 | Karaoke | 04 Karaoke | Greater perceived vocal intelligibility vs music |
| 35 | Acoustic | 05 Akustik | Less low-energy accumulation, lighter wet signal |
| 36 | Qori | 06 Imam Qori | More diction, less echo repeat, expanded reverb |
| 37 | Jazz | 07 Jazz | Vocal intimacy, controlled echo |
| 38 | Rock | 08 Rock | Strongest presence difference without indiscriminate high-shelf boost |
| 39 | Duet | 09 Ceramah | **Rebuilt** speech donor as two-singer karaoke ambience |
| 40 | Grand Hall | 10 Adzan | **Rebuilt** long reverb + excessive repeat into performance hall |

Original GEN3 functions remain 01–10 BT and 11–20 UAUDIO. Legacy 21–30 remains unchanged. Hardware has **10 slots only**, regardless of 40 PC catalog numbers.

## Calculation audit (comparative RBJ 48 kHz proxy, not firmware)

The table below measures the average magnitude difference in the filter chain **Mic A → Main** or **Music → Main** between GEN4 V2 and the exact GEN3 donor.

| ID | Vocal 1–2.5 kHz Δ dB | Vocal 2.5–4.5 kHz Δ dB | Vocal 5–8 kHz Δ dB | Music 1.5–4.5 kHz Δ dB |
|---|---:|---:|---:|---:|
| 31 | +1.4 | −0.9 | −1.2 | −1.5 |
| 32 | +1.7 | −0.5 | −1.2 | −0.9 |
| 33 | +1.2 | −0.9 | −1.2 | −1.5 |
| 34 | +2.1 | −0.7 | −1.4 | −1.4 |
| 35 | +1.2 | −0.6 | −1.0 | −0.6 |
| 36 | +1.5 | −0.9 | −1.4 | −1.5 |
| 37 | +1.2 | −0.6 | −1.1 | −0.6 |
| 38 | +2.2 | −1.0 | −1.7 | −1.8 |
| 39 | +1.2 | −0.7 | −1.1 | −1.6 |
| 40 | +1.2 | −0.9 | −1.2 | −1.4 |

This model does not measure nonlinear DSP, microphone, crossover, acoustics, actual routing gains, hardware headroom, wet path summation, or stereo response. Full baseline EQ boosts exceed +20 dB at individual shelves, so **physical clipping/feedback and levels must be checked**. Generator deliberately does not raise these shelves or the output master except one negative trim on Ballad.

## Binary safety and verified invariants

- 10 native .k500 images, each 1144 bytes, valid checksum, sourceRaw=2.
- Donor SHA-256 checked against owner-supplied GEN3 ZIP hash corpus.
- EQ **gain-only** edits (max 3.5 dB per band) within native ±24 dB parameter range; all Mic A and Mic B gain edits mirrored exactly.
- Only named FX source offsets can change: reverb scalar 0x74, echo scalar 0x7B, echo repeat 0x7C, reverb decay 0xC8, pre-delay 0xCA, echo delay 0xCC, selected Ballad Music routes (0x2A/0x3E/0x52), Ballad Main L/R master (0x24/0x26), name at 0x454 and checksum 0x475.
- All other unknown fields, alternate EQ channels, EQ filter topology and output routing are left intact.
- `tools/generate_gen4_presets.py` is a reproducible one-pass CMake configure generator. `tools/ci/test_gen4_audition.py` locks **all ten exact SHA-256 outputs**, tests native 1144→656 projection, exact diff allowlist, stereo mic EQ parity and donor tamper rejection.
- Do **not** add another GitHub workflow; use consolidated K500 CI.

## Real listening acceptance and rollback

Before any Mass Upload, back up physical hardware slots; apply one candidate to a test slot, read back, and verify its Music Source BT. A/B against exact counterpart GEN3 01↔31, 02↔32 etc on the same microphone, music, seating and amplifier routing. Match perceived level for both presets (especially 32) before judging clarity. Record held note, fast lyrics, consonants/sibilants, harsh upper-mids, bass, music mask, feedback onset, wet/dry tail and clipping across Main/Surround/Center/Sub.

If one GEN4 is not clearly superior, **keep corresponding GEN3** as the known listening reference. Nothing about valid files, CI or modeled EQ alone qualifies a flagship release.
