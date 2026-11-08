# FOCUS A BT R3 — Melo 2k ringing audit & Ballad preference

Owner listened on K500: Melayu R2 is getting enjoyable and has present vocals, but effect resonance/ringing is heard near 2 kHz. Owner prefers Ballad ALT to Ballad R2. These observations outrank simulation.

## Changes

| Candidate | Purpose | DSP change relative to preferred parent |
| --- | --- | --- |
| 07 Melayu R3 | Primary | Reverb PEQ band2 2700 Hz/Q2.6/-11.8 -> 2500 Hz/Q2.0/-12.0; Reverb high shelf band5 +13.5 -> +12.7 dB |
| 07 Melayu Soft R3 | Lighter comparison | Reverb band2 2550 Hz/Q2.1/-12.0; Reverb shelf +12.6 dB |
| 10 Ballad ALT Favorite | Primary | **No change**. Exact 1144-byte owner-preferred Ballad ALT. |
| 10 Ballad Silk R3 | Optional comparison | Reverb band2 2600 Hz/Q2.4/-11.8; Reverb shelf +12.9 dB. |

All 4 files in this folder are independent native 1144-byte Bluetooth sourceRaw=2 K500 files. Physical upload bank still has exactly ten slots; put an alternative into the **same** slot, not an extra slot.

## Comparative 48kHz static DSP model

- Melayu R3 vs Melayu R2 at 1.7–2.3kHz: Reverb response -3.269 dB, Main vocal energy proxy -1.146 dB, Mic dry 0 dB.
- Melayu Soft R3: Reverb -2.655 dB, composite -0.973 dB, Mic dry 0 dB.
- Ballad Favorite: exact same bytes as user preferred ALT; 0 dB change by definition.
- Ballad Silk R3: Reverb -1.366 dB, composite -0.517 dB, Mic dry 0 dB.

RBJ/crossover magnitudes and energy-summed wet/dry paths are **not exact K500 firmware DSP**, not a physical recording and not proof that an acoustic ringing/feedback is solved. User-delivered audit ZIP has model plots and detailed per-band CSV. Reverb/echo/Mic/room interact acoustically in hardware.

## Reproducibility and safety

- Each native file 1144 bytes with checksum sum%256=0, native name <=16 ASCII, Bluetooth input raw2; PEQ frequency/Q/gain/type ranges checked and 656-byte slot projection verified independently.
- No changes to MicA/B, Music, Main, Echo, Center, Surround, Sub, crossovers/footers, Alt or unknown regions, output compressor, FX delay/repeat, BT source gain.
- Only Reverb EQ and intentional name+checksum bytes change in R3 variants; Ballad ALT Favorite is byte-identical to user-liked ALT.
- 8 other research bank presets remain byte-identical; official 41-preset PC bank, protected Golden and stable installer stay untouched.
- Main bank's `../patches.json` and `../generate.py --check` reproduce current primary 07/10. Alternative candidates are separately documented with exact expected hashes in `MANIFEST_R3.json`.

## Physical A/B instructions

Back up existing K500 slots. With same BT music, microphone, listening position and low starting volume, compare Melayu R2 vs R3 on 2k ringing with sung sustained vowels and trailing reverb. Confirm vowel body/pitch support is not lost. If R3 becomes too dry, try Melayu Soft R3.

Load exactly the Ballad ALT Favorite file into slot10 first; it should sound the same as the earlier preferred ALT. Try Ballad Silk only as an explicit optional change. If 2k ringing remains even with FX muted, investigate acoustic feedback/room or Mic path rather than continuing to cut reverb.

**Status: research-only, physical K500 re-audition required before promotion.**
