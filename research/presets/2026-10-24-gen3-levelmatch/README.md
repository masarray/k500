# Gen3 user-feedback level-match candidates — 2026-10-24

Ten **user-supplied, user-preferred** original Gen3 K500 presets, normalized toward the **current official** `resources/presets/01_KONSER_NYANYI.k500` as a single common reference. This bank is deliberately isolated from the official distributed library until **physical K500 audition**.

## What is locked
- **Every EQ byte is unchanged**, including Mic A/B, Music, Main, MainAlt, Surround/Alt, Center/Alt, Sub/Alt, Reverb and Echo; all types, frequencies, Q, gains, crossover settings, mirror/footer bytes stay native-donor-identical. No voicing or curve redesign.
- FX, dynamics, Mic direct returns, mic input A/B, names, unknown/reserved bytes remain unchanged.
- Top Music/Mic/Effect normalized to **25/25/25** from current reference.
- Five music input trims normalized to **-3/-3/-3/-4/-4 dB**, same as current reference.
- Independent per-output gain and music route corrections solve *music vs vocal* level gap without modifying any EQ curve. Stereo L/R output scalar pairs remain equal.
- The built-in `resources/presets` bank, program code, release workflow and hardware protocol are **untouched**.

## Relative output levels
These are device output gain control readings (dB), **not measured SPL**. Each Main Music and composite Vocal result is within about ±0.25 dB of the reference **in the comparative RBJ/energy-proxy simulation**. This does not guarantee exact hardware equivalence, clipping headroom or equal perceived loudness.

| # | Internal name | Main dB | Surround dB | Center dB | Sub dB | Music route Main/Surr/Center/Sub | Sim Main Music/Vocal residual dB |
|---|---|---:|---:|---:|---:|---|---|
| 01 | ARTIST GEN3 ARI | -0.5 | +5.5 | +4.5 | +8.5 | 56/44/27/88 | +0.00 / +0.22 |
| 02 | PODCAST REBORN | +2.5 | +11.0 | +10.0 | +8.0 | 30/23/14/88 | -0.02 / +0.12 |
| 03 | DANGDUT GEN3 ARI | -1.5 | +4.5 | +3.5 | +10.0 | 73/56/35/88 | +0.03 / -0.18 |
| 04 | KARAOKE ARTIST | -2.0 | +6.5 | +5.0 | +7.5 | 50/37/24/88 | +0.01 / +0.11 |
| 05 | AKUSTIK GEN3 ARI | -1.5 | +4.5 | +3.5 | +9.0 | 65/49/30/88 | -0.05 / -0.22 |
| 06 | IMAM QORI GEN 3 | -1.5 | +5.0 | +3.5 | +9.5 | 67/49/33/88 | -0.01 / -0.03 |
| 07 | JAZZ GEN3 ARI | -1.0 | +5.0 | +4.0 | +10.0 | 67/51/32/88 | +0.05 / -0.19 |
| 08 | ROCK GEN3 ARI | -1.0 | +5.0 | +4.0 | +9.5 | 66/49/31/88 | +0.06 / +0.12 |
| 09 | MC CERAMAH GEN 3 | -1.5 | +5.0 | +3.5 | +9.5 | 67/49/33/88 | -0.01 / -0.03 |
| 10 | ADZAN MEKAH GEN3 | -1.5 | +5.0 | +3.5 | +9.5 | 67/49/33/88 | -0.01 / -0.03 |

## Verification before use
- Validate binary length **1144** and additive checksum `sum(bytes)%256 == 0`.
- Validate 16-character preset name, native scalar guard and every PEQ type/frequency/Q/gain via `python tools/k500_preset_lab.py validate <file>`.
- Diff each candidate against its user-supplied donor: only Top 0x08..0x0A, input gains 0x1E..0x22, output 0x24/0x26/0x38/0x3A/0x4C/0x60, music routes 0x2A/0x3E/0x52 and additive checksum 0x475 are allowed to change.
- Compare at matched source signal/master setting in a real K500: dry vocal, backing music, FX tails and Sub, with safe initial listening volume. Large legacy EQ boosts may overload intermediate DSP even when final output proxy is matched; simulation **does not prove** unclipped output.

This research candidate requires device-side listening approval before any official-library promotion.
