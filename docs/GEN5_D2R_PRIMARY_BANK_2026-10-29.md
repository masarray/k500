# D2R GEN5 as Primary K500 Bank — 2026-10-29

## Canonical catalog (PC-only; 10 actual hardware slots)

| PC IDs | Bank | Source / integrity |
| --- | --- | --- |
| 01–10 | GEN5 D2R **UAUDIO** | Every file sourceRaw=5; 02 MC HOST RADIO uses earlier 10 Ceramah DSP; 10 Ballad uses earlier 02 Ballad DSP. Other DSP bytes unchanged. |
| 11–20 | GEN3 Bluetooth | Previous 01–10 copied byte-for-byte; individual SHA-256 hashes locked in `tools/ci/contracts.py` |
| 21–30 | GEN3 UAUDIO | Derived from 11–20 by changing music source BT raw 2 to USB Audio raw 5, name + checksum only |
| 31–40 | Legacy SonKuPik | Previous 21–30 copied byte-for-byte; original SHA-256 hashes locked |
| **41** | **D2R Golden** | Exact owner-auditioned D2R artifact, SHA-256 `465c077d654ca0100156ccfafbf29f8bdb59af37a7dcc2633a8a56dc6552864a`; internal hardware name remains `D2_REV` |

### GEN5 01–10 current filenames / SHA-256
- 01_ARTIST_D2R.k500 (ARTIST G5, UAUDIO=5): `e99acae1fd80cf89257006b233a56c43801dc3e6655601c0fff574bbe58b9c4c`
- 02_MC_HOST_RADIO_D2R.k500 (MC HOST RADIO, UAUDIO=5): `7fd3f30ebc1e759e3b2e5c0f03c66764efa2f19b52dc085da2aa371f5f5e913a`
- 03_DANGDUT_D2R.k500 (DANGDUT G5, UAUDIO=5): `732ac46ab689d1dcf6adf37e7fe6c55cd2e1ab9429668189a96271e0de04d0d1`
- 04_ROCK_D2R.k500 (ROCK G5, UAUDIO=5): `1a52db23ff5d08f45783228cf2aa48cde21a8afd477308e0572319d256922fcf`
- 05_ACOUSTIC_D2R.k500 (ACOUSTIC G5, UAUDIO=5): `a644d2d51865dc82a59ef2dd2878c3b2363c3b8d298446c765198a01341063c0`
- 06_JAZZ_D2R.k500 (JAZZ G5, UAUDIO=5): `9280b098f626aa96a8e6f8840ab3fb1559132162f0d35249714a5c20bf58a217`
- 07_MELAYU_D2R.k500 (MELAYU G5, UAUDIO=5): `e4eb5f5c1d1f83517196ba52fccf4f2f4997742a9c5bebeb8a95b135b22dc92b`
- 08_QORI_D2R.k500 (QORI G5, UAUDIO=5): `9a2c34414d75010acbfdea19603797c7a5986e0d84647834dece7dfad2ef11c4`
- 09_ADZAN_D2R.k500 (ADZAN G5, UAUDIO=5): `3a8638dbe5777ea257bf3e9aa8a2a7f07fbdb872673e935c8b1257f9e8ed1de6`
- 10_BALLAD_D2R.k500 (BALLAD G5, UAUDIO=5): `a9e38086329e37a94a9a83a6773b0191604ff37cf27527c006920dfd5f86b78e`

## Sonic qualification
User personally selected D2R_33_REVERB_SOFT over all GEN4 candidates. The latest GEN5 genre/mosque variants are extrapolations from that reference, not all individually tested to be equal or better. Only 41 is a bit-perfect golden reference. Tested 03 DANGDUT (formerly GEN5 33) differs from Golden solely in embedded name and checksum, with identical DSP data. New GEN5 01–10 all operate in UAUDIO mode raw=5. Old GEN3 BT 11–20 and exact original D2R GOLDEN 41 remain raw=2.

## Pre-release engineering gates
- 41 must remain byte-identical; all 31 committed files size 1144 / checksum / name / SHA-256 and strict slot semantics.
- 21–30 generated UAUDIO must preserve 11–20 sonic bytes.
- Exactly 41 built-in catalog definitions, no duplicates, correct canonical IDs; new 01–10 first, golden last.
- Avoid stale cache duplicates after migrations. Physical Mass Upload stages <=10 slots.
- Green consolidated CI incl Windows native build+regression, manual GUI check and hardware A/B before claiming sonic final.
- Never overwrite immutable public v1.1.3 assets. Do not merge rejected GEN4 V2/2.1 PR #217.

## November 1 corrective revision: native radio workflow

- **02 MC HOST RADIO** replaces prior Ballad at catalog 02 using the previous #10 CERAMAH D2R DSP, source=UAUDIO (raw5), native embedded name `MC HOST RADIO`; target broadcast/hosting speech, not a vocal-singing Ballad curve.
- **10 BALLAD** retains the previous #02 BALLAD D2R DSP at catalog 10 with native UAUDIO source.
- 01,03–09 retain their full DSP tonal, FX, routing, and dynamics parameter bytes; only source 2→5 and checksum differ.
- 02 preserves the former Ceramah microphone/EQ/reverb/echo and voice-processing DSP; it additionally enables Music→Main at raw70 (from the earlier Ballad/Main balanced music routing), so radio announcers can hear UAUDIO backing tracks. Source, hardware name, Main music level and checksum are the only changed fields relative to the Ceramah donor. 10 differs from previous Ballad only by source+checksum.
- Preserve *exact* GOLDEN 41 SHA-256 `465c077d654ca0100156ccfafbf29f8bdb59af37a7dcc2633a8a56dc6552864a` and archival 11–40 hashes.
- The official library loader rejects **stale BT cache** for primary 01–10 and ignores prior cache filenames `02_BALLAD_D2R.k500` and `10_CERAMAH_D2R.k500` without deleting user content.
- v1.1.4-rc.1 is an immutable prerelease with the older arrangement; publish a **new** RC number (e.g. v1.1.4-rc.2) only after exact-main green CI.

### Radio-host usability audit (2026-11-01)

The original GEN5 Ceramah preset muted music in all output routes. A radio-host preset must not silently discard the UAUDIO backing track. Therefore **02 MC HOST RADIO** now restores Music→Main at native raw=70, with its speech-focused Mic EQ, reverb and zero echo unchanged. Other music routes (Surround, Center, Sub) stay 0 by design; users can change routing if required. Verified SHA-256: `7fd3f30ebc1e759e3b2e5c0f03c66764efa2f19b52dc085da2aa371f5f5e913a`.

## 2026-11-01: owner audition selected Velvet R1 for GEN5 02 MC HOST
- **Active #02:** exact owner-listened `02_MC_HOST_RADIO_VELVET_R1.k500` binary promoted under the current repository path `resources/presets/02_MC_HOST_RADIO_D2R.k500`; SHA-256 `af86797542e11d1079d1c2ee61000307469704db208daae10795b1e2dad7586e`, 1144 bytes, native name MC HOST RADIO and UAUDIO raw5.
- **Verdict:** R1 best ('lumayan'); OnAir R3 only second choice; Silk R2 worst; old D2R/Ceramah MC HOST unacceptable. These verdicts are specifically for radio-host voice. **D2R GOLDEN #41 for singing remains untouched.**
- **Important audit:** `BEST_START` and `VELVET_R1` have identical complete SHA-256 and bytes. Perceived difference cannot arise from differing preset parameters; investigate device slot state, readback, monitoring level and external processing before fine-tuning.
- **R1 sonic anchor:** Mic compressor ratio3, gate -70 dB, Music->Main raw48, Reverb level22, Main Reverb49, decay850 ms and predelay32 ms, Echo disabled. Native Mic EQ: 1550Hz -7.5 dB, 3900Hz -10.2 dB, 6300Hz -11dB, 9270Hz -16dB, high shelf 11000Hz +10dB. Full EQ response includes other shelf/bell filters.
- **Why R2 lost (hypothesis):** RBJ proxy suggests R2 attenuates about 1.96 dB more than R1 at 6300Hz and 1.86 dB more at 8000Hz, plus less 1550Hz articulation and more 125Hz body. Suppression of sibilance at the cost of natural voice timbre may explain the owner's verdict; not a verified isolated causal experiment.
- **Safety:** official cache must not override #02 with a prior same-named valid UAUDIO preset unless byte SHA matches R1. Never delete user-owned cached files. Historical Ceramah/R3 notes in this document are superseded as *current* authority.
