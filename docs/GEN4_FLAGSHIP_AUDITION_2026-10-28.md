# GEN4 Singing-Focused Flagship Candidates — 2026-10-28

Status: **audition / not owner-accepted**. Depends on GEN3 migration PR #216.

## Why GEN4 is a new candidate bank, not a replacement
The ten original 01–10 GEN3 presets in the uploaded level-matched reference were preferred over prior official modes in real K500 singing. Their broad shelves combined with substantial compensating notches are unusual and intentionally **locked** here, not flattened. 11–20 remains GEN3 UAUDIO; 21–30 remains original legacy. The new **31–40 GEN4** bank is generated from byte-locked 01–10 GEN3; the hardware still stores at most 10 slots.

## Profile map
| PC ID | Donor GEN3 | Candidate name | Working hypothesis |
|---|---|---|---|
| 31 | 01 | 31_ARTIST_GEN4 | Natural concert lead: open mids, smooth edge |
| 32 | 02 | 32_PODCAST_GEN4 | Speech clarity with tame sibilance |
| 33 | 03 | 33_DANGDUT_GEN4 | Punch and warm vocal separation |
| 34 | 04 | 34_KARAOKE_GEN4 | Vocal intelligibility, reduced boxiness |
| 35 | 05 | 35_ACOUSTIC_GEN4 | Natural instrument separation and soft highs |
| 36 | 06 | 36_QORI_GEN4 | Qori delivery: diction and silky reverb |
| 37 | 07 | 37_JAZZ_GEN4 | Close warm jazz vocal with gentle ambience |
| 38 | 08 | 38_ROCK_GEN4 | Rock lead forward without sharpness |
| 39 | 09 | 39_CERAMAH_GEN4 | Speech intelligibility, moderate wet field |
| 40 | 10 | 40_ADZAN_GEN4 | Long ambient adzan with preserved articulation |

## Guardrails and authority
- Diff whitelist: Music/Mic A/Mic B/Main **gain only**, reverb pre-delay at file `0xCA` (max +12 ms), visible name 16 bytes at `0x454`, checksum `0x475`. Q, f, typeRaw, crossover, compressor, source, route, output volumes, EQ ALT and unknown bytes are byte-identical.
- Max individual PEQ gain shift ≤ 0.8 dB. The modeled maximum **Mic→Main dry** difference versus its exact donor across the 10 candidates is approx 1.0 dB, based on a 48 kHz comparative RBJ biquad model; not real K500 or acoustic measurement.
- `tools/generate_gen4_presets.py` rejects donor hash drift, out-of-domain fields, non-projected slots and unexpected edits; CMake embeds exact 31–40 generated files offline.
- `tools/ci/test_gen4_audition.py` pins output hashes, verifies strict diffs, source unchanged, compiled catalog mapping and the 10 GEN4 native slot projections.
- K500 original GEN3 presets already have very large individual shelving boosts, so cumulative headroom/clipping **is not verified** by a file checksum or magnitude simulation. Test with real microphones, music, output meters and downstream amplifier gain. Do not claim safety based only on modeled spectral peak differences.

## Controlled real-hardware acceptance (required before promotion)
1. Back up all hardware K500 10 slots before mass upload; install GEN4 in a deliberate test slot only.
2. Compare the matching GEN3/GEN4 pair on the same source, microphone, singing passage and loudspeaker routing. Match perceived playback level rather than judging the louder preset as better.
3. For each candidate, use at least two singers and several passages: low-register sustained vowel, consonant-heavy verse, high-volume chorus, and fast lyrics, plus music-only and voice-only sections.
4. Check *body, breath, phrase intelligibility, listening fatigue, wet/dry separation, reverb tails, feedback onset, bass punch and peak clipping*. Verify the actual active hardware setting with readback.
5. Keep GEN3 if GEN4 is equal or worse; record side-by-side listening notes. Only promote per-profile after explicit owner approval.

## For engineers
From the `main` tree after PR #216:
```bash
python tools/generate_gen4_presets.py --source resources/presets --output /tmp/gen4
python tools/ci/test_gen4_audition.py
```
No extra GitHub CI workflow is needed. 31–40 are PC library catalog numbers, not hardware slots.
