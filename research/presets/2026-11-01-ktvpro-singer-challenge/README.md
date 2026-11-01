# KTVPRO Singer Challenge (2026-11-01)

**Audition candidates, not proven better on physical hardware.** Exact donor `resources/presets/03_DANGDUT_D2R.k500` (GEN5 UAUDIO; same DSP as untouched D2R GOLDEN #41), source SHA-256 `732ac46ab689d1dcf6adf37e7fe6c55cd2e1ab9429668189a96271e0de04d0d1`.

All EQ changes below refer to **real frequency Hz, not index order**. This is critical: K500 Mic band 1 = 2550Hz, band 6 = 1550Hz, band 7 = 3900Hz, band 8 = 6200Hz, band 9 = 9270Hz.

| Candidate | Mic A+B individual band gain delta | Music gain delta | Other |
|---|---|---|---|
| FOCUS A | 1550 +0.7, 2550 -0.2, 3900 -1.5, 6200 -0.8 dB | 2880 -1.2, 6500 -0.6 dB | No other DSP changes |
| WARM B | 388 +0.8, 1550 -0.8, 2550 -0.3, 3900 -0.5, 6200 -0.8 dB | 1300 -0.6, 2880 -0.8, 6500 -0.6 dB | No other DSP changes |
| SPACE C | 1550 +0.4, 2550 -0.1, 3900 -1.0, 6200 -0.7 dB | 2880 -0.8, 6500 -0.5 dB | Echo repeat 10→9, delay 207→240ms; Reverb decay 2880→3080ms, predelay 75→85ms |

## 48 kHz canonical RBJ simulation: Mic/Main and Music/Main deltas vs GEN5

| Region / modeled delta | Focus A | Warm B | Space C |
|---|---:|---:|---:|
| Dry vocal 1.5–2.8 kHz average | -0.02 dB | -0.49 dB | -0.03 dB |
| Dry vocal 3.2–6.8 kHz average | -1.10 dB | -0.63 dB | -0.80 dB |
| Music 2.5–4 kHz average | -0.93 dB | -0.70 dB | -0.63 dB |
| Vocal 300–8 kHz power-proxy | -0.09 dB | -0.01 dB | -0.09 dB |
| Music 300–8 kHz power-proxy | -0.30 dB | -0.35 dB | -0.22 dB |

These are model differences, not measured SPL/LUFS, DSP clipping guarantees, or proof of subjective preference. Relative vocal/articulation protection and backing masking must be judged with matched-level device audition.

## Binary safety and implementation status
- Every candidate passes 1144-byte size / sum%256=0 checksum, hardware visible name <=16 chars, strict PEQ native domain and 656-byte native-slot encoding.
- Mic A/B changes are paired. All EQ **type, frequency, Q**, crossover/footers, hidden Alt, compressor, output gain/routing and unknown bytes are preserved. Source stays UAUDIO raw=5. Only whitelisted gain words, C FX temporal scalars, name and checksum differ.
- Native GEN5 01–10, Gen3 11–30, legacy 31–40 and D2R GOLDEN 41 are untouched.
- No build / CI / release workflows added. Do not promote as official or merge until physical K500 blind/matched-level A/B demonstrates meaningful benefit and no clipping/feedback.

Full reproduction script, comparative plots, per-offset audit, SHA-256, and USB-audio baseline are provided in the accompanying audition ZIP.
