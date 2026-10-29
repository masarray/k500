# Historical record — superseded by GEN5 D2R reordering on 2026-10-29

This document records the earlier planned GEN3 catalog migration; current canonical numbering is in [GEN5 D2R Primary Bank](GEN5_D2R_PRIMARY_BANK_2026-10-29.md). Binary identities remain valid but catalog numbers below are historical.

# GEN3 Priority Preset Library — 2026-10-25

## Catalog (PC, not device slot)
- **01–10 — BT MODE:** exact ten user-supplied GEN3 files from `gen3_k500_levelmatch_10_presets(1).zip`, no EQ/FX/gain edits. Embedded source scalar `file[0x000E]=2`.
- **11–20 — UAUDIO:** companions generated offline at CMake configure from 01–10 in order. Each changes only `file[0x000E]=5`, hardware-visible name bytes `0x0454–0x0463`, and checksum `0x0475`. EQ, routing, gains, compressors, and FX remain identical.
- **21–30 — LEGACY:** the ten previous official 01–10 binaries, unchanged; their original embedded hardware names remain as-is.
- **31–40:** reserved for possible additional old presets; no invented content or empty fake entries.

## Primary mapping
| BT | UAUDIO | GEN3 source | Legacy | Previous accepted name |
|---|---|---|---|---|
| 01 | 11 | `01_ARTIST_GEN3_ARI.k500` | 21 | KONSER NYANYI |
| 02 | 12 | `02_PODCAST_REBORN.k500` | 22 | MC HOST RADIO |
| 03 | 13 | `03_DANGDUT_GEN3_ARI.k500` | 23 | KAR DANGDUT |
| 04 | 14 | `04_KARAOKE_ARTIST.k500` | 24 | POP ROCK BALLAD |
| 05 | 15 | `05_AKUSTIK_GEN3_ARI.k500` | 25 | POP KENANGAN V2 |
| 06 | 16 | `06_IMAM_QORI_GEN3.k500` | 26 | SHOLAWAT SYAHDU |
| 07 | 17 | `07_JAZZ_GEN3_ARI.k500` | 27 | JAZZ LOUNGE |
| 08 | 18 | `08_ROCK_GEN3_ARI.k500` | 28 | BLUES CLUB |
| 09 | 19 | `09_MC_CERAMAH_GEN3.k500` | 29 | ACOUSTIC NATURAL |
| 10 | 20 | `10_ADZAN_MEKAH_GEN3.k500` | 30 | REGGAE DUB |

## Safety and rollback
- Only **10 hardware slots** physically exist on the K500. The PC library can hold 30+ source presets; transfer selection still caps at 10, with no implicit upload, recall or destructive device action.
- The user-auditioned GEN3 bank is listening authority for this migration, *not* objective proof of DSP clipping safety or a firmware-level qualification.
- The historical previous official files are retained bit-for-bit at 21–30. Never edit their raw PEQ aliases, hidden crossover/footer data or names as part of catalog renumbering.
- Verify exact hashes, 1144-byte size, additive checksum, device-slot compatibility, and UAUDIO allowed-offset diff before packaging.
- Source directories: `resources/presets` contains checked-in 01–10 + 21–30; `tools/generate_uaudio_presets.py` creates 11–20 at build configuration. Native bundled resource and PC catalog combine all 30.
- Remote sync still tracks checked-in `.k500` files from the same public GitHub directory. The ten UAUDIO companions are bundled locally; they are not remote catalog assets.
- Historical listening acceptance documents continue to describe the **legacy** bank. New GEN3 release acceptance requires a separate hardware test and owner approval.
