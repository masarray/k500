# Public FOCUS BT catalog migration (owner-approved, 2026-11-08)

Owner accepted the last Music R6 research set as official PC IDs 01–10; the 10 blobs come from commit 6a0ab4a5e8219b4402f1ca0af567424ff99dc32f, unchanged (no new DSP tuning).

| ID | Bank | Source / protection |
| --- | --- | --- |
| 01–10 | New FOCUS R6 research bank | Bluetooth source raw2 |
| 11–20 | Former GEN5 D2R UAUDIO 01–10 incl Velvet R1 | Original file bytes, source raw5 |
| 21–30 | Former GEN3 BT 11–20 | Original file bytes |
| 31–40 | Former GEN3 UAUDIO 21–30 | Regenerated from shifted Gen3 BT, only source/name/checksum |
| **41** | **Owner-selected D2R GOLDEN** | Exact original bytes and ID, protected |
| 42–51 | Former legacy BT 31–40 | Original file bytes |

The GOLDEN 41 exception is intentional. Keeping GOLDEN protected in its immutable catalog slot means old legacy 31–40 map 42–51, not 41–50. This preserves all 41 previous presets, while presenting 10 newly approved FOCUS presets first. PC catalog IDs never represent physical K500 slots (exactly 10).

## Full binary mapping (Git blob SHA)

| New ID | Current file | Former ID | SHA-1 Git blob |
| ---: | --- | ---: | --- |
| 1 | `01_FOCUS_ARTIST_BT.k500` | Research | fc7275b0a28589239fb0412aa56a91b20f7d2737 |
| 2 | `02_FOCUS_MC_BT.k500` | Research | a454910c42e81155ff94482187ca18b6145f6c14 |
| 3 | `03_FOCUS_DANGDUT_BT.k500` | Research | c3113e54868d8e072ab8758623bc1ce9281aea9d |
| 4 | `04_FOCUS_ROCK_BT.k500` | Research | 7cfc51322a06d2be5475b061e0380400ab1547a6 |
| 5 | `05_FOCUS_ACOUSTIC_BT.k500` | Research | 332e1b462a6d7b23d5a0a1d69370167561d7d637 |
| 6 | `06_FOCUS_JAZZ_BT.k500` | Research | 512bb12b13d5ba19b8ed825ef899a6c5f42d37f3 |
| 7 | `07_FOCUS_MELAYU_BT.k500` | Research | 04ae105932e042eca37d683cf6d99d0863f7ef08 |
| 8 | `08_FOCUS_QORI_BT.k500` | Research | 2f14b935bf150be8ec9aa7b7e31a171500ce9b9a |
| 9 | `09_FOCUS_ADZAN_BT.k500` | Research | 324170d8fe5f2018b4b7d0797dfc2c217fb14697 |
| 10 | `10_FOCUS_BALLAD_BT.k500` | Research | 1a0e815ee6aa08372783e743bcb08b06fd7567aa |
| 11 | `11_ARTIST_D2R.k500` | 1 | 59bc01375818ea74dd480af5fb9691aa9b4c1cf4 |
| 12 | `12_MC_HOST_RADIO_D2R.k500` | 2 | dc8c71c09b2d2c6b146fe09df50daa46b2cd5cb4 |
| 13 | `13_DANGDUT_D2R.k500` | 3 | 520c77e9b8e26d5c737c09d84ad4c4f1770f9a1a |
| 14 | `14_ROCK_D2R.k500` | 4 | 5240c89d73dbf57e9ca255ddbcdc320a30e305e1 |
| 15 | `15_ACOUSTIC_D2R.k500` | 5 | dbc5ccc955738cdc314262c0be4a361c6cd397b3 |
| 16 | `16_JAZZ_D2R.k500` | 6 | 774be86415742f9ba430a216fbdc2b0b77147ed7 |
| 17 | `17_MELAYU_D2R.k500` | 7 | 45f15a244355b5c671c309b319896cabbf007e85 |
| 18 | `18_QORI_D2R.k500` | 8 | 03ef351055b25981af771639d0e36a7e9fd011cf |
| 19 | `19_ADZAN_D2R.k500` | 9 | 9851c4ca399f52b4cfd07a39227b54f7b76d9d0e |
| 20 | `20_BALLAD_D2R.k500` | 10 | 74350116c00bc952cc0ff4bfc38ebb94a1f5a8e1 |
| 21 | `21_ARTIST_GEN3_ARI.k500` | 11 | 252f2bec378ab3b0ebf015cce58b80e4ce44d50f |
| 22 | `22_PODCAST_REBORN.k500` | 12 | 46cce915dbc4502410dd8834788db05a173cfae2 |
| 23 | `23_DANGDUT_GEN3_ARI.k500` | 13 | 93939bcbc7a5710460294be01f982d19578ea7f6 |
| 24 | `24_KARAOKE_ARTIST.k500` | 14 | 5abeabbd9eb90e3b5c56578147d249d267dcb7b5 |
| 25 | `25_AKUSTIK_GEN3_ARI.k500` | 15 | 509776527a495ddd87b385156d0649e642457c77 |
| 26 | `26_IMAM_QORI_GEN3.k500` | 16 | 19a129e1cd5b1f80d14c3a26db5db1a3360c8ad9 |
| 27 | `27_JAZZ_GEN3_ARI.k500` | 17 | 77242cff66b5c742f2392327c5e068d1ea69b19d |
| 28 | `28_ROCK_GEN3_ARI.k500` | 18 | ad38e5a607ce99a300032bd2d29044af5b9c2741 |
| 29 | `29_MC_CERAMAH_GEN3.k500` | 19 | 3b8916038476b0ed625fefc20ba75081160a8420 |
| 30 | `30_ADZAN_MEKAH_GEN3.k500` | 20 | 3b96b07c4783cd85b5e9ce3a792d3193690846f2 |
| 31 | `31_ARTIST_GEN3_ARI_UAUDIO.k500` | 21 | Build-time generation |
| 32 | `32_PODCAST_REBORN_UAUDIO.k500` | 22 | Build-time generation |
| 33 | `33_DANGDUT_GEN3_ARI_UAUDIO.k500` | 23 | Build-time generation |
| 34 | `34_KARAOKE_ARTIST_UAUDIO.k500` | 24 | Build-time generation |
| 35 | `35_AKUSTIK_GEN3_ARI_UAUDIO.k500` | 25 | Build-time generation |
| 36 | `36_IMAM_QORI_GEN3_UAUDIO.k500` | 26 | Build-time generation |
| 37 | `37_JAZZ_GEN3_ARI_UAUDIO.k500` | 27 | Build-time generation |
| 38 | `38_ROCK_GEN3_ARI_UAUDIO.k500` | 28 | Build-time generation |
| 39 | `39_MC_CERAMAH_GEN3_UAUDIO.k500` | 29 | Build-time generation |
| 40 | `40_ADZAN_MEKAH_GEN3_UAUDIO.k500` | 30 | Build-time generation |
| 41 | `41_GOLDEN_D2R.k500` | 41 | 0396e1c36919b9365abcc7b8f0809729edddf60b |
| 42 | `42_KONSER_NYANYI.k500` | 31 | 05bb6bab680cbf8abf38dc3f158ca152d259e024 |
| 43 | `43_MC_HOST_RADIO.k500` | 32 | 0daa24a1d93af51c1c4f737bda39eacc07280be7 |
| 44 | `44_KAR_DANGDUT.k500` | 33 | a80d8d53c411efafce08e454297d5df76fbb96ff |
| 45 | `45_POP_ROCK_BALLAD.k500` | 34 | f608d3dc7552dc95db3d0cfa5c5a0bb5582d7cb0 |
| 46 | `46_POP_KENANGAN_V2.k500` | 35 | f98b20468dc8468dbdac601d5b5371788a158972 |
| 47 | `47_SHOLAWAT_SYAHDU.k500` | 36 | e271fb67ff016fcfd5aab6d68c00dfe31ce8d907 |
| 48 | `48_JAZZ_LOUNGE.k500` | 37 | f592a22bd7bed9eca8c828514bff912b354cd04a |
| 49 | `49_BLUES_CLUB.k500` | 38 | ddc6d7919eeef8f96d9f046b8ec974eb7d2adb1f |
| 50 | `50_ACOUSTIC_NATURAL.k500` | 39 | c44b8de3694bc4beab439b1dd33603728c233fe2 |
| 51 | `51_REGGAE_DUB.k500` | 40 | f9699263e6e3dc2920942982a31fe49bdb7f6a8f |

## Validation and release controls

- All 10 FOCUS bank Git blobs and their SHA-256 fingerprints exactly match the owner-accepted R6 research manifest.
- 30 prior committed presets are moved by Git blob reference; no renaming of embedded hardware identities, EQ, effects, source or checksums. GOLDEN 41 remains untouched.
- Original old Gen3 UAUDIO companion sequence is regenerated at configure-time from relocated 21–30. Strict size/checksum, type/Q/gain/frequency validation and 656-byte slot projection remain fail-closed.
- Native bundled catalog and Official Sync cache must not allow stale source or same-named owner-regressed versions to override new public presets. Prior official cache aliases are hidden, not removed.
- Automatic CI scope remains one consolidated Windows flow. No new CI workflow, no v1.1.5 release overwrite.
- Source and website preset links can be public when merged; new Windows application release still needs completed native build, installer check, and new immutable version/tag. Do not claim old v1.1.5 binary already contains new 51-entry catalog.
