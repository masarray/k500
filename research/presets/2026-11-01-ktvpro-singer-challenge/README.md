# KTVPRO Singer Challenge — 2026-11-01

Three new **audition-only**, native-compatible K500 preset candidates built from the exact current GEN5 03_DANGDUT_D2R.k500 DSP donor. Reference SHA-256: `732ac46ab689d1dcf6adf37e7fe6c55cd2e1ab9429668189a96271e0de04d0d1`. **These are not proven better until matched-level A/B singing on a physical K500.**

| Candidate | Relative purpose | EQ changes |
|---|---|---|
| FOCUS A | Keep 1.5k articulation, control 3.9–6.2k harshness, reduce backing masking | Mic both: 1550 +0.7, 2550 -0.2, 3900 -1.5, 6200 -0.8 dB. Music: 2880 -1.2, 6500 -0.6 dB |
| WARM B | Stronger body and more forgiving upper-mid | Mic both: 388 +0.8, 1550 -0.8, 2550 -0.3, 3900 -0.5, 6200 -0.8 dB. Music: 1300 -0.6, 2880 -0.8, 6500 -0.6 dB |
| SPACE C | Sharper separation between dry and wet vocal, softer upper-mid | Mic both: 1550 +0.4, 2550 -0.1, 3900 -1.0, 6200 -0.7 dB. Music: 2880 -0.8, 6500 -0.5 dB. Echo repeat 10→9, delay 207→240 ms; reverb decay 2880→3080 ms, predelay 75→85 ms |

## Quantitative simulation relative to GEN5
48kHz RBJ cascaded Mic+Main and Music+Main, NOT a real K500 measurement:

| Model | Focus A | Warm B | Space C |
|---|---:|---:|---:|
| Vocal 1.5–2.8kHz delta (mean) | +0.17 | -0.37 | +0.08 dB |
| Vocal 3.2–6.8kHz delta (mean) | -0.96 | -0.61 | -0.67 dB |
| Backing 2.5–4kHz delta (mean) | -0.93 | -0.70 | -0.63 dB |
| Dry vocal 300–8k broadband model | -0.04 | +0.01 | -0.06 dB |

Changes across full filter stacks differ from individual band gains due to interaction. Comparisons must be loudness-matched on the physical hardware. No claimed clipping/headroom proof.

## Safety and source preservation
- The original GEN5 primary library, #41 D2R GOLDEN, archived GEN3 and hardware protocols remain untouched.
- Only whitelisted Mic A+B and Music EQ gain words, limited C FX timing scalars, the native hardware-visible name, and additive checksum are modified.
- Every candidate is 1144 bytes, valid checksum, strict native-compatible PEQ domain; source remains UAUDIO raw=5; filter types/frequencies/Q, crossovers/Alt, dynamics, routing, and output gains are unchanged.
- Exact independent file SHA-256 and complete changed-byte audits are supplied in local audition package.
- Physical device testing: backup existing device presets, audition baseline vs A/B/C at equal perceived level, monitor clipping/feedback, keep whichever sounds best or retain unchanged GEN5.
