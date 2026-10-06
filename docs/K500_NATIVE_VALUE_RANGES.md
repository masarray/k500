# K500 Native Value Ranges

This file is the repository contract for manufacturer-KTV value domains. Do not
invent wider editor ranges for convenience. When new hardware/native evidence is
captured, update this file first, then keep QML, StudioEngine, Controller and
Protocol clamps aligned.

## Evidence levels

- **Native UI observed** — directly visible at the manufacturer application's
  minimum/maximum state on physical K500 hardware.
- **Captured/legacy contract** — retained from earlier K500 reverse-engineering
  patches/captures and must not be widened without new evidence.
- **Unverified** — do not tighten or expand based on guesswork.

## Reverb — native UI observed

The 2026-09-18 physical-device acceptance screenshots show the manufacturer
Reverb page at both endpoints.

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Reverb Level | 0 | 100 | scalar/% | Native UI observed |
| Reverb Decay | 500 | 5000 | ms | Native UI observed |
| Reverb Predelay | 0 | 100 | ms | Native UI observed |
| Reverb Highpass | 20 | 1000 | Hz | Native UI observed |
| Reverb Lowpass | 4000 | 16000 | Hz | Native UI observed |
| Reverb Direct | 0 | 100 | scalar/% | Native UI observed |
| Reverb PEQ Gain | -24 | +24 | dB | Native UI observed |

The FX filter bounds also match the older filterRangeForEqKey() contract:
HPF 20..1000 Hz, LPF 4000..16000 Hz.

## Echo — capture-mapped native contract

The Qt native bridge uses the full `CMD 0x0D` image. The 2026-10-04 physical
endpoint sweeps close the previously uncertain right-channel and predelay ranges.

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Echo Level | 0 | 100 | scalar/% | Captured contract |
| Echo Repeat | 0 | 10 | scalar | Captured contract |
| Echo Left Delay | 0 | 1000 | ms | Captured contract |
| Echo Left Predelay | 0 | 100 | ms | **Native endpoint USB capture** |
| Echo Right Delay | -50 | +50 | % | **Native endpoint USB capture; raw=UI+50** |
| Echo Right Predelay | -50 | +50 | % | **Native endpoint USB capture; raw=UI+50** |
| Echo Highpass | 20 | 1000 | Hz | Captured FX frequency contract |
| Echo Lowpass | 4000 | 16000 | Hz | Captured FX frequency contract |
| Echo Direct | 0 | 100 | scalar/% | Captured contract |

Reverb and Echo expose HPF/LPF **frequency only** in the manufacturer UI. They
do not have HP/LP filter-type selectors.

## Music Tone — native capture observed

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Music Noise Gate | OFF, then -90 | -50 | dB | Native packet capture |
| Music Bass | -12.0 | +12.0 | dB | Native packet capture |

Music Noise Gate uses raw `0` for OFF and raw `1..41` for `-90..-50 dB`.
Music Bass uses CMD `0x0C`, selector `0x02`, encoded in 0.1 dB units:
`raw = round((dB + 12) * 10)`.

Connect/readback offsets for these two Music Tone controls are still evidence-gated;
the current checkpoint implements only their byte-verified live WRITE mappings.

## Mic FBX / anti-feedback — native capture observed

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| FBX Level | 0 | 4 | integer level | Paired physical USB captures |

READ truth is direct `activeMemory[0x001B]`. WRITE uses Top Mic `CMD 0x05`
with the same raw integer level `0..4`; the following command byte is fixed
`0x00` in all five captured levels.

## System Music Max — native capture observed

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Music Max | 0 | 84 | scalar | Native packet capture |
| Top Music / Master Music | 0 | Music Max | scalar | Native packet capture |

Lowering Music Max below the current Top Music value clamps Top Music in the
same native `CMD 0x02` block. Raising Music Max does not raise Top Music.

## System startup / recording — native capture observed

The focused 2026-10-04 USB captures close these manufacturer UI domains:

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Music Init | 0 | 84 | scalar | Native packet capture, CMD 0x02 |
| Mic Init | 0 | 84 | scalar | Native packet capture, CMD 0x05 |
| Effect Init | 0 | 84 | scalar | Native packet capture, CMD 0x0A |
| USB Record Volume | 1 | 6 | UI step | Native packet capture, CMD 0x3E selector 0x03 |

USB Record stores `raw = UI - 1`, therefore raw `0..5` represents UI `1..6`.

Mic Max and UDisk Record were subsequently closed by the final 2026-10-04
operational capture batch below; do not revive the older read-only assumption.

Adj Manner / VR OFF is a boolean ownership switch rather than a numeric range.
The 2026-10-06 paired connect captures prove direct `activeMemory[0x008C]` /
file scalar `0x0094`: 0 = front-panel VR active, 1 = VR OFF/software ownership.
C0 `data[19]` is explicitly non-authoritative because controlled captures show
contradictory bit-0 polarity. Live writes use CMD 0x07 with route byte `0x03`;
valid RSP 0xF8 commits current-session state. `activeMemory[0x008C]` is used
when a real connect/reconnect/Recall readback occurs, not as immediate write verification.

## Final System operational controls — native capture observed

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Mic Max | 0 | 84 | scalar | Native USB capture, CMD 0x05 |
| UDisk Record Volume | 1 | 6 | UI step | Native USB capture, CMD 0x3E |
| Dance Mic Threshold | -60 | 0 | dB | Native USB capture, CMD 0x22 |
| Dance Mic Hold Time | 1 | 30 | s | Native USB capture, CMD 0x22 |
| BT Name | 1 | 8 | printable ASCII chars | Native USB capture, CMD 0x4E |

Mic Max is a hard ceiling for Top Mic:
`TopMic = min(TopMic, MicMax)`.

Dance Mic threshold uses `raw = dB + 60`. Hold Time uses raw seconds.
Both values travel together in one CMD 0x22 frame. Their write encoding is
captured, but the old speculative reconnect seed at `0x0093/0x0094` is retired
because `0x0094` is physically proven Adj Manner VR OFF.

BT Name's captured field is exactly 8 bytes, NUL-padded. Do not expand the
writable UI to the longer readback buffer without new packet evidence.

## Output Delay — native capture observed

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| Main L Delay | 0 | 50 | ms | Native USB capture, CMD 0x0E |
| Main R Delay | 0 | 50 | ms | Native USB capture, CMD 0x0E |
| Surround L Delay | 0 | 50 | ms | Native USB capture, CMD 0x0E |
| Surround R Delay | 0 | 50 | ms | Native USB capture, CMD 0x0E |
| Center Output Delay | 0 | 50 | ms | Native USB capture, CMD 0x0E |
| Subwoofer Output Delay | 0 | 50 | ms | Native USB capture, CMD 0x0E |

Encoding is uint16 little-endian milliseconds. Main uses L at output-data
16..17 and R at 18..19. **Surround reverses native wire order**: R is at
16..17 and L at 18..19. Center/Sub use 16..17 only.

The native distance readout is display-only at approximately
`distance_m = delay_ms * 0.34`; no distance field is sent.

## Common PEQ

| Parameter | Minimum | Maximum | Unit | Evidence |
| --- | ---: | ---: | --- | --- |
| PEQ Gain | -24 | +24 | dB | Native UI observed / captured |
| PEQ Frequency | 20 | 20000 | Hz | Existing native EQ contract |

## Implementation rule

The same range must exist at every writable layer:

1. QML interaction bounds.
2. EqBandModel / StudioEngine model clamp.
3. K500Controller semantic-state clamp.
4. K500Protocol transport serialization clamp.
5. K500PresetEditMapper offline persistence validation.
6. tools/k500_preset_lab.py write-side validation for patchable proven fields.
7. Hardware-free CI guard/self-test.

A UI-only clamp is insufficient because programmatic calls could still generate
out-of-domain native writes. For offline preset persistence and AI patching,
out-of-range requests must be rejected without changing bytes; they must not be
silently clamped to a different semantic value.

This write-side rule does not retroactively normalize legacy donor bytes. Older
firmware/native presets may contain values outside a newer UI endpoint contract;
preserve those bytes unless the user explicitly edits the proven field.

## Reconciliation / live-edit rule

Ordinary live edits must not force the application through a full 939-byte
authoritative resync. Adj Manner is an explicit exception because it changes
hardware ownership: after CMD 0x07 / RSP 0xF8 the application performs one
fresh 939-byte readback before LIVE resumes. Full active-memory hydration remains the authority barrier
for connect/reconnect/Recall/recovery and explicit qualification. Transport
acceptance is still not hardware confirmation, but verification must not make
normal knob/fader editing look like a disconnect/reconnect cycle.

## Change control

If a new native-app screenshot/capture contradicts this table:

1. preserve the evidence;
2. update this document with date/source;
3. update all writable layers together;
4. add/adjust deterministic guards;
5. rerun physical-device acceptance before merging.
