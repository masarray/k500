# R2 trial — corrected MELAYU and BALLAD

Owner K500 feedback: R1 Ballad not enjoyable; R1 Melayu somewhat lacking.
Hypothesis (NOT proof): large echo reductions plus longer reverb hurt singer support.
R2 preserves FOCUS A original Music/Mic/Main EQ, changes only few proven effect scalar fields.

| Candidate | Slot | Reverb decay | Predelay | Echo level | Repeat | Main reverb |
|---|---:|---:|---:|---:|---:|---:|
| MELAYU R2 | 7 | 3100ms | 72ms | 88 | 9 | 97 |
| MELAYU ALT | 7 | 3150ms | 75ms | 87 | 8 | 97 |
| BALLAD R2 | 10 | 3000ms | 75ms | 89 | 9 | 97 |
| BALLAD ALT | 10 | 3000ms | 75ms | 89 | 10 | 93 |

Primary R2 binaries are also deployed at ../07_FOCUS_MELAYU_BT.k500 and ../10_FOCUS_BALLAD_BT.k500. Exact hashes / scalar changes are in ../patches.json; reproduce via python ../generate.py --check.
Four native variants have 1144 bytes, zero additive checksum, source raw2 BT, representable native-slot PEQ, and valid 656-byte native-slot shadow projection. No EQ geometry, Music EQ, Mic EQ, Alt/crossover/reserved/unknown byte was edited. Eight other 01-10 presets were preserved exactly.
Offline 48 kHz RBJ energy-response and synthetic WAV proxies are only comparative, NOT exact firmware audio or proven real-world sound.

Hardware A/B: back up physical slots. Try BALLAD R2 against original R1 and slot 01 FOCUS at equal volume, then MELAYU R2. Compare ALT in the SAME physical slot if needed (K500 only has ten slots). Keep Bluetooth source, mic distance and volume constant, start cautiously low because original EQ boosts are large. Listen for echo rhythm, vocal body/pitch control, masking, harshness, feedback.

Status: RESEARCH ONLY, DO NOT merge/promote to official presets or release until real K500 owner approval.
