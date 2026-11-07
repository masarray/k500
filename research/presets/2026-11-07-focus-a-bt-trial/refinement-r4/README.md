# K500 FOCUS BT R4 — Artist 01 selective effects refinement

## Owner hardware observations

- **Melayu R3 is OK**: freeze its current exact bits (SHA-256 `315b750917e028d99472cf46ad50240a792904594ebe4e9e0c0ca237d3904030`).
- **Artist 01**: effect ringing mildly around 2–2.5 kHz.
- **Ballad ALT** is preferred over Ballad R2: freeze exact preferred bank slot 10.

## Why this tuning

Current Artist 01 and owner-accepted Melayu R3 have exactly identical Mic A/B EQ, Music EQ, Main EQ and Echo EQ. The accepted Melayu R3 was already edited to reduce 2 kHz-area wet Reverb energy; Artist had the pre-correction Reverb bell and high shelf. R4 migrates **only the two Reverb bands**, avoiding any changes to dry vocal, music, Echo, output or FX timing.

| Variant (slot 01) | Reverb PEQ band 2 | Reverb high shelf band 5 | Echo repeat | Status |
| --- | --- | --- | ---: | --- |
| `01_ARTIST_R3_ROLLBACK.k500` | 2700 Hz / Q2.6 / −11.8 dB | +13.5 dB | 10 | Original, available for A/B |
| `01_ARTIST_R4_BT.k500` | 2500 Hz / Q2.0 / −12.0 dB | +12.7 dB | 10 | Primary trial; directly matches Melayu R3 wet EQ |
| `01_ARTIST_SOFT_R4_BT.k500` | 2550 Hz / Q2.1 / −12.0 dB | +12.6 dB | 10 | Alternative, gentler modeled 1.7–2.3 kHz reduction |
| `01_ARTIST_R4_E9_BT.k500` | 2500 Hz / Q2.0 / −12.0 dB | +12.7 dB | 9 | Optional: isolate tail repeat-count impact |

## Offline model (Artist original -> R4)

- 48 kHz RBJ static EQ/crossover response, 4096 frequency samples, parallel dry/wet *energy-sum proxy*.
- Reverb-to-Main path 2.0–2.5 kHz: **−3.835 dB**.
- Full Main vocal composite energy proxy 2.0–2.5 kHz: **−1.108 dB**.
- Dry Mic-to-Main: **0.000 dB**; all original Main Music and Echo EQ unchanged.
- Echo9 static response equals primary R4 exactly; the temporal repeats are **not modeled** by static plots.
- Wet reverb correction is also −3.265 dB in 1.7–2.3kHz, and near-zero (+0.05 dB) in 2.5–3.5kHz, avoiding a new presence ridge in the proxy.

## Binary/safety audit

- All candidates 1144 bytes; checksum sum%256==0; source raw2 Bluetooth; native name <=16 ASCII; strict PEQ type/frequency/Q/gain range checks and deterministic exact 656-byte device-slot projection with shadow verification.
- Changes only to proven Reverb band2 frequency/Q/gain and band5 gain plus name/checksum; optional Echo9 changes native repeat raw from 10 to 9. Other EQ, crossovers, Alt blocks and reserved/unknown data are untouched.
- `generate_candidates.py` reproduces each from the exact byte-matched, SHA-256 verified `../source/KTVPRO_FOCUS_A.k500` donor. The main primary bank remains reproducible via `../generate.py --check`.
- Nine other bank candidates, including Melayu R3 and owner-preferred Ballad ALT, are byte-identical to the previously shared R3 bank.

## Physical testing

Back up current K500 bank. Change **slot 01 only**, start at low volume (the original source has unusually high EQ boosts), use identical BT song, microphone distance, speaker positions, level and room. Sing sustained vowels `i/e` and phrases, then stop singing to listen for the effect tail. Compare original versus R4; if remaining ringing follows repeated echo taps, try E9 in slot 01. If R4 feels too dry, try SOFT in slot 01. Do not add alternative presets as extra slots.

If the ringing remains with both Reverb and Echo **OFF**, the acoustic Mic/speaker/room/feedback path is a stronger hypothesis; a preset wet EQ alone cannot guarantee a fix.

**Research only. No physical acceptance claimed for R4; no edits to the official 41-preset library, stable installer or CI workflows.**
