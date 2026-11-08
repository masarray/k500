# K500 Music R6 — shared Artist 01 and MC 02 music path

**Hardware observations:** 01 Music already stands out but is slightly excessive and loses some perceived detail; 02 Music lacks standout. Binary audit shows **the previous seven-band Music EQ was already almost identical**: only 160 Hz band2 differed −4.3 vs −4.4 dB. The material configuration mismatch is Main Music raw **73 in 01 vs 55 in 02**. Both already had Bluetooth gain −3 dB, Center Music 35, Surround Music 56 and identical Main EQ. A `55→73` native level change is **18 raw steps**, not an asserted 18 dB increase.

## Shared Music EQ settings — R6

| Band | Type / frequency | 01 prior | 02 prior | Shared R6 both |
|---|---|---:|---:|---:|
|1|P 13 kHz|−4.7 dB|−4.7 dB|−4.3 dB|
|2|P 160 Hz|−4.3 dB|−4.4 dB|−4.3 dB|
|3|P 70 Hz|+15.7 dB|+15.7 dB|+15.0 dB|
|4|P 1.3 kHz|−5.7 dB|−5.7 dB|−5.7 dB|
|5|P 2.88 kHz|−11.0 dB|−11.0 dB|−10.3 dB|
|6|P 6.5 kHz|−8.4 dB|−8.4 dB|−7.4 dB|
|7|High shelf 488 Hz|+15.7 dB|+15.7 dB|+14.6 dB|

**Main Music raw = 73 on 01 and 02.** Music EQ region `0x01B0..0x01F7` is bit-for-bit identical between both, including enable/type/frequency/Q/crossover and reserved bytes. Main EQ is also identical. **Mic A/B, Reverb, Echo, Music crossovers and all other output routing remain exactly as their previous preset**, so MC 02 stays a dry speech/MC profile while Artist 01 keeps its approved R4 vocal identity.

## Relative software model (48 kHz RBJ, Music→Main EQ only)

R6 vs Artist R4: bass 60–120 Hz −0.763 dB; low mids 150–500 Hz −0.679 dB; presence 1.5–3 kHz −0.660 dB; detail 3–5 kHz −0.502 dB; detail 5–9 kHz −0.375 dB; air 9–14 kHz −0.743 dB. Relative 5–9 kHz detail is better maintained than broadband shelf overemphasis. Model excludes exact K500 firmware EQ and any effects of Main Music raw volume scale, compression/headroom, room and microphone masking.

## Byte safety, rollback, and test workflow

- R6 native BT `.k500` files each 1144 bytes, modulo-256 additive checksum 0, Bluetooth selector raw2, valid PEQ domains, only Music gain bytes and MC Main Music raw changed (plus names/checksum).
- Rollback original 01 Artist R4 and original 02 MC are included unchanged in this folder. Every file verified against exact SHA-256; `validate_shared_music.py` confirms change allowlist, byte identity of full 72-byte Music section and matching Main Music raw73.
- `../patches.json` and `../generate.py --check` reproduce current 10-primary-bank outputs from committed source FOCUS A. Presets **03–10 unchanged** including already-liked Melayu R3 and Ballad ALT.
- Test same Bluetooth music at the same global volume and device output, compare 01 R4→R6 (overdone/detail) and 02 old→R6 (standout). Back up physical slot 01 and 02. Start at low level, as source Music EQ and Main EQ have large boosts; watch distortion and feedback.
- If 01 and 02 still differ in music despite same EQ/main Music scalar, compare recall state, active input, firmware DSP and global volume. Vocal/FX differences may change perceived music masking even with identical Music path.

**This is the candidate final shared Music EQ, not physically accepted production settings.** Do not merge official 41-preset cache, protected Golden #41 or installer release until K500 hardware audition confirms.
