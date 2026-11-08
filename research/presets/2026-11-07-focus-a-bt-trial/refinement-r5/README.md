# FOCUS A BT research R5 — Artist 01 nuance between R4 and E9

## Real K500 feedback is authoritative

R4 is comfortable but not as vocal-forward as E9. E9 has desirable standout but a slightly over-forward midrange. Two original `.k500` native files have exactly the same Mic EQ, Music EQ, Reverb EQ, Echo EQ, output EQ, FX level, decay and routing; **only `echo.repeat` 10 vs 9 differs in audio**, plus the preset name/checksum. Therefore no claim that R4 and E9 differ in static EQ is justified.

## R5 A/B candidates — all slot 01, Bluetooth sourceRaw 2

| File | Echo repeats | Main Echo return | Echo Bell3 at 2708Hz | Echo HS5 at 2500Hz | Purpose |
| --- | ---: | ---: | ---: | ---: | --- |
| `01_ARTIST_R4_BT_REFERENCE.k500` | 10 | 95 | −12.4 dB | +11.7 dB | Existing less forward control |
| `01_ARTIST_R4_E9_REFERENCE.k500` | 9 | 95 | −12.4 dB | +11.7 dB | Owner-liked forward control |
| `01_ARTIST_R5_SWEET_BT.k500` | 9 | 94 | −12.8 dB | +11.4 dB | Primary: subtle wet-mid/echo balance |
| `01_ARTIST_R5_RETURN_BT.k500` | 9 | 92 | −12.4 dB | +11.7 dB | Independent global return-only test |
| `01_ARTIST_R5_MID_BT.k500` | 9 | 95 | −13.2 dB | +11.2 dB | Independent wet-mid spectral test |

Other Echo PEQ bands, MIC dry, Reverb, Music, Main, dynamics, Echo delay/repeat type and routing stay byte-identical to the preferred E9 parent. Integer native repeat count cannot be interpolated to 9.5.

## Offline relative echo-only 48 kHz RBJ magnitude model vs E9

| Candidate | 2–2.5 kHz | 2.5–3.5 kHz | 3.5–6 kHz |
| --- | ---: | ---: | ---: |
| Sweet | −0.499 dB | −0.580 dB | −0.455 dB |
| Return | −0.279 dB | −0.279 dB | −0.279 dB |
| Mid | −0.767 dB | −0.917 dB | −0.658 dB |

These are **Echo-branch estimates only**, not a total vocal or exact K500 acoustic measurement. Perception of repeat-count change is time-domain and is not captured in RBJ static curves. Echo send/return is approximated by linear scalar ratio; this is not established as firmware gain law.

## Native-file safety

- Derived from exact original E9 file SHA-256 `c847ddd316c699ec1c252e73f5b99ddbfa938667ed091cba859d320122cdbfd4`.
- New files each exactly 1144 bytes, checksum modulo 256 = zero, Bluetooth selector raw 2, name length <=16 ASCII.
- Strict primary-PEQ type/frequency/Q/gain representation checked for MicA/B, Music, Main, Surround, Center, Sub, Reverb and Echo.
- Byte-diff whitelist relative to E9: `0x002E` (Main echo return), `0x0428` (Echo band3 gain), `0x0438` (Echo band5 gain), `0x0454..0x0463` (name), `0x0475` (checksum); no Alt/unknown/crossover bytes or Mic dry changed.
- Root `../generate.py --check` reproduces bank primary from bit-identical original native `KTVPRO_FOCUS_A` donor. `generate_candidates.py` reproduces R5 variants from existing `../refinement-r4/01_ARTIST_R4_E9_BT.k500` parent.
- 9 other bank `.k500` files unchanged byte-for-byte versus R4, including locked Melayu R3 and Ballad ALT.

## Physical K500 trial

Back up K500 slot 01. Use same Bluetooth song, mic distance, monitor level, speaker/room and sing normal phrases plus sustained e/i vowel. Compare E9 -> R5 Sweet -> Return or Mid only if needed, replacing **the same physical slot 01**. Listen for standout, chest/body, 2–3.5k glare, echo tail smoothness, fatigue and feedback. Match perceived level before final selection; do not assume model deltas are audible.

If over-forward mids persist with both FX muted, dry Mic or room/feedback requires a separate targeted investigation. Do not overwrite official preset library or merge until K500 user accepts R5.

**Status: research-only, hardware listening still pending.**
