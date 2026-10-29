# D2R GEN5 as Primary K500 Bank — 2026-10-29

## Canonical catalog (PC-only; 10 actual hardware slots)

| PC IDs | Bank | Source / integrity |
| --- | --- | --- |
| 01–10 | GEN5 D2R BT | Exact ten binaries previously shipped in `K500_D2R_GEN5_30-40_GENRE_MASJID_AUDITION_20261029.zip` as original 31–40; filenames renumbered only, bytes unchanged |
| 11–20 | GEN3 Bluetooth | Previous 01–10 copied byte-for-byte; individual SHA-256 hashes locked in `tools/ci/contracts.py` |
| 21–30 | GEN3 UAUDIO | Derived from 11–20 by changing music source BT raw 2 to USB Audio raw 5, name + checksum only |
| 31–40 | Legacy SonKuPik | Previous 21–30 copied byte-for-byte; original SHA-256 hashes locked |
| **41** | **D2R Golden** | Exact owner-auditioned D2R artifact, SHA-256 `465c077d654ca0100156ccfafbf29f8bdb59af37a7dcc2633a8a56dc6552864a`; internal hardware name remains `D2_REV` |

### GEN5 01–10 names / original SHA-256
- 01_ARTIST_D2R.k500 (ARTIST G5): `9d6d8a09fb0eb2d10ea12092176baf877132aee4dbeaedc30d66b95dc6a70f55`
- 02_BALLAD_D2R.k500 (BALLAD G5): `cb4c46bc6b70c97e8f4348b2d8e6117d223904d5a61994749a46876d287dfc3b`
- 03_DANGDUT_D2R.k500 (DANGDUT G5): `3241df08a7938523133751555f3fc3daf77cd37c638e79adc529f997d47b37da`
- 04_ROCK_D2R.k500 (ROCK G5): `f4b903e98582e3f5fbfd2c4739d82766ac10c5432165e00eecd49d420482302a`
- 05_ACOUSTIC_D2R.k500 (ACOUSTIC G5): `d2b749412f59963fc4133a1485d949ad86a1c0970119b90f31ebdc533c4f3563`
- 06_JAZZ_D2R.k500 (JAZZ G5): `c5a6afcc6dd9bcabb4d2f1044dd688c2b3c76bdf00a3df50b561fd59a3d31798`
- 07_MELAYU_D2R.k500 (MELAYU G5): `4b22a5656dd5521fba0fc96c06eb8e8a9118931e739997020c9299051a308a11`
- 08_QORI_D2R.k500 (QORI G5): `66d953e2f580c8d4b42473b62085ba43f698287b7e474ee5be8fc2f7060e537e`
- 09_ADZAN_D2R.k500 (ADZAN G5): `cc31fb555ac0248903519363dda2f2d6a95f907a5dd69adfdfb83caf01890224`
- 10_CERAMAH_D2R.k500 (CERAMAH G5): `644ffcee84c34015957ec65534ae679c8c404c14a5fe2f84e4f83d9f562e701f`

## Sonic qualification
User personally selected D2R_33_REVERB_SOFT over all GEN4 candidates. The latest GEN5 genre/mosque variants are extrapolations from that reference, not all individually tested to be equal or better. Only 41 is a bit-perfect golden reference. Tested 03 DANGDUT (formerly GEN5 33) differs from Golden solely in embedded name and checksum, with identical DSP data. K500 sourceRaw=2 means the new 01–10 operate in BT mode. If UAUDIO is desired for GEN5, new companions must be designed and explicitly cataloged rather than silently switching sources.

## Pre-release engineering gates
- 41 must remain byte-identical; all 31 committed files size 1144 / checksum / name / SHA-256 and strict slot semantics.
- 21–30 generated UAUDIO must preserve 11–20 sonic bytes.
- Exactly 41 built-in catalog definitions, no duplicates, correct canonical IDs; new 01–10 first, golden last.
- Avoid stale cache duplicates after migrations. Physical Mass Upload stages <=10 slots.
- Green consolidated CI incl Windows native build+regression, manual GUI check and hardware A/B before claiming sonic final.
- Never overwrite immutable public v1.1.3 assets. Do not merge rejected GEN4 V2/2.1 PR #217.
