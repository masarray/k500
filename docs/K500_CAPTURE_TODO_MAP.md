# K500 Capture Coverage & TODO Map

Status date: 2026-10-04
Baseline for this capture integration: `main` @ `d157cee083f1727a2943ba79111931eb98846d9a`

The focused 2026-10-04 System captures are documented in
`docs/K500_SYSTEM_CONTROLS_CAPTURE_MAP.md`. The final Output Delay sweeps are
documented in `docs/K500_OUTPUT_DELAY_CAPTURE_MAP.md`. Historical capture
references remain valid evidence, but new mapping work must start from current
main and must not revive superseded branches.

This document is the capture-planning source of truth. It separates:
- device READ mapping (connect/readback/runtime state),
- native WRITE mapping,
- value/range evidence,
- and UI exposure.

Do not promote a field from TODO to mapped from filename assumptions alone. Use checksum-valid frame deltas and, where persistence/readback matters, reconnect/readback evidence.

## Legend

- ✅ = mapped with capture/golden-vector evidence
- 🟨 = partially mapped; one direction or runtime/readback truth is missing
- ❌ = not mapped; dedicated capture required
- ⛔ = intentionally not writable until evidence exists
- SW = software-only behavior; no device capture needed

## Connection / runtime

| Function | READ/status | WRITE/action | Current status | Capture needed |
| --- | --- | --- | --- | --- |
| USB heartbeat / handshake | ✅ | ✅ | complete | no |
| 939-byte active memory | ✅ | n/a | complete | no |
| Play/Pause state | ✅ E3/C0 bit 0x04 | ✅ CMD 0x06/02 | complete | no |
| Rewind / Forward | n/a | ✅ CMD 0x06 | complete | no |
| Mute state on connect/runtime | ✅ C0 data[7] bit 0x02 | ✅ CMD 0x15 | complete | no |
| Use Init Volume | ✅ C0 data[7] bit 0x04 | ✅ CMD 0x12 + ED ACK | complete | no |
| Active Equipment Mode slot | ✅ C0 + readback | ✅ Recall CMD 0x01 | complete | no |

## Music

| Control | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| Master Music | ✅ | ✅ top Music block | complete | no |
| Source INPUT1/INPUT2/BT/UDISK/OPTIC/UAUDIO | ✅ | ✅ | complete | no |
| Input1/Input2/BT/UDISK/Digital gain | ✅ | ✅ | complete | no |
| Music Key | ✅ | ✅ | complete | no |
| PEQ bands freq/Q/gain/type | ✅ | ✅ | complete | no |
| EQ bypass | ✅ | ✅ | complete | no |
| HPF/LPF frequency | ✅ | ✅ | complete | no |
| HPF/LPF filter type on WRITE | n/a | ✅ CMD 0x11 mapping | complete | no |
| HPF/LPF filter type on CONNECT | ✅ HP=activeMemory[0x0007], LP=activeMemory[0x0008] | n/a | complete for Music | no |
| Music Noise Gate | ✅ direct activeMemory[0x0005], raw 0=OFF / 1..41=-90..-50 dB | ✅ CMD 0x02 block | capture-mapped READ/WRITE | no — reconnect truth closed 2026-10-04 |
| Music Bass | ✅ direct activeMemory[0x00DF], dB=(raw-120)/10 | ✅ CMD 0x0C selector 0x02, 0.1 dB encoding | capture-mapped READ/WRITE | no — reconnect truth closed 2026-10-04 |
| Music Mid | ❌ | ❌ controller unsupported | missing | **YES** |
| Music Mid Frequency | ❌ | ❌ controller unsupported | missing | **YES** |
| Music Treble | ❌ | ❌ controller unsupported | missing | **YES** |

## Mic

| Control | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| Master Mic | ✅ | ✅ | complete | no |
| Mic A / Mic B volume | ✅ | ✅ | complete | no |
| Compressor threshold/ratio/attack/release | ✅ | ✅ | complete | no |
| Mic PEQ A/B | ✅ | ✅ | complete | no |
| Mic EQ Link | ✅ | ✅ captured CMD 0x3C | complete | no |
| Mic HPF/LPF frequency | ✅ | ✅ | complete | no |
| Mic HPF filter type on CONNECT | ✅ direct activeMemory[0x0013] from State-A reconnect | n/a | capture-mapped | no |
| Mic LPF filter type on CONNECT | ✅ direct activeMemory[0x0014], 0..5 physically swept | ✅ CMD 0x11 selector 0x01; final byte preserves Music Input1 raw from activeMemory[0x0016] | capture-mapped READ/WRITE | no |
| Mic Noise Gate | ✅ file scalar 0x0016 -> activeMemory[0x000E] | ❌ no donor-verified write | partial | **YES — write delta** |
| FBX / anti-feedback level | ✅ direct activeMemory[0x001B], range 0..4 | ✅ CMD 0x05 direct byte + RSP 0xFA | complete | no |

## Reverb

| Control | READ | WRITE | Range | Status / capture |
| --- | --- | --- | --- | --- |
| Level | ✅ | ✅ CMD 0x0B | ✅ 0..100 | complete |
| Direct | ✅ | ✅ CMD 0x0B | ✅ 0..100 | complete |
| Decay | ✅ | ✅ CMD 0x0B | ✅ 500..5000 ms | complete |
| Predelay | ✅ | ✅ CMD 0x0B | ✅ 0..100 ms | complete |
| HPF frequency | ✅ | ✅ CMD 0x0B | ✅ 20..1000 Hz | complete |
| LPF frequency | ✅ | ✅ CMD 0x0B | ✅ 4000..16000 Hz | complete |
| PEQ bands | ✅ | ✅ | ✅ | complete |
| EQ bypass | ✅ 24-bit readback | ✅ CMD 0x0F | n/a | complete |
| HPF filter type | N/A | N/A | manufacturer UI has HPF frequency only | no capture — no native control |
| LPF filter type | N/A | N/A | manufacturer UI has LPF frequency only | no capture — no native control |

## Echo

| Control | READ | WRITE | Status / capture |
| --- | --- | --- | --- |
| Effect Level | ✅ | ✅ CMD 0x0D | complete |
| Repeat | ✅ | ✅ CMD 0x0D | complete |
| Direct | ✅ | ✅ CMD 0x0D | complete |
| Left Delay | ✅ | ✅ CMD 0x0D | complete |
| Left Predelay | ✅ | ✅ CMD 0x0D | complete |
| Right Delay % | ✅ | ✅ CMD 0x0D | complete |
| Right Predelay % | ✅ | ✅ CMD 0x0D | complete |
| HPF frequency | ✅ | ✅ CMD 0x0D | complete |
| LPF frequency | ✅ | ✅ CMD 0x0D | complete |
| PEQ bands | ✅ | ✅ | complete |
| EQ bypass | ✅ | ✅ CMD 0x0F | complete |
| HPF filter type | N/A | N/A | no native control; HPF is frequency-only |
| LPF filter type | N/A | N/A | no native control; LPF is frequency-only |
| Right Delay / Right Predelay / Left Predelay native endpoints | ✅ -50..+50%, -50..+50%, 0..100 ms | ✅ CMD 0x0D exact encoding | complete — endpoint sweep closed 2026-10-04 |

## Outputs: Main / Surround / Center / Sub

| Control family | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| Output volume(s) | ✅ | ✅ CMD 0x0E | complete | no |
| Mic/Music/Reverb/Echo mix levels | ✅ | ✅ | complete | no |
| Compressor threshold/ratio/attack/release | ✅ | ✅ | complete | no |
| PEQ bands | ✅ | ✅ | complete | no |
| EQ bypass | ✅ | ✅ | complete | no |
| HPF/LPF frequency | ✅ | ✅ | complete | no |
| HPF/LPF filter type on CONNECT | ✅ Main 0x002C/0x002E; Surround 0x0040/0x0042; Center 0x0054/0x0056; Sub 0x0068/0x006A | n/a | capture-mapped | no |
| Surround L/R Delay | ✅ file 0x00D8/0x00DA -> active 0x00CF/0x00D1 | ✅ CMD 0x0E; wire order R@data16, L@data18 | capture-mapped | no |
| Main L/R Delay | ✅ file 0x00D4/0x00D6 -> active 0x00CB/0x00CD | ✅ CMD 0x0E; L@data16, R@data18 | capture-mapped | no |
| Center Output Delay | ✅ file 0x00DC -> active 0x00D3 | ✅ CMD 0x0E data16 | capture-mapped | no |
| Sub Output Delay | ✅ file 0x00DE -> active 0x00D5 | ✅ CMD 0x0E data16 | capture-mapped | no |

## System / Equipment Mode

| Control | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| Mode names 1..10 | ✅ active memory | ✅ active-slot image 0x0280..0x028F + Store 0x41/42/43 | capture-mapped | no |
| Active Mode name/slot | ✅ | ✅ Recall | complete | no |
| Use Init Volume | ✅ C0 bit | ✅ CMD 0x12 + ED ACK; Recall semantics proven | complete | no |
| Save current slot | ✅ transaction | ✅ Store 0x41/42/43 | complete | no |
| Mass Upload | n/a | ✅ | complete | no |
| Reset All Settings | ❌ | ❌ disabled | missing | **YES — destructive, last priority** |
| Music Init Vol | ✅ 0x000B | ✅ Top Music CMD 0x02 second scalar | capture-mapped | no |
| Music Max Vol | ✅ 0x000C | ✅ Top Music CMD 0x02 + hard ceiling clamp | complete | no |
| Mic Init Vol | ✅ 0x0012 | ✅ Top Mic CMD 0x05 second scalar | capture-mapped | no |
| Mic Max Vol | ✅ 0x0013 | ✅ Top Mic CMD 0x05 third scalar + hard ceiling | capture-mapped | no |
| Effect Init Level | ✅ 0x001D | ✅ dedicated CMD 0x0A + F5 ACK | capture-mapped | no |
| UDisk Record Vol | ✅ 0x0095 + 1 | ✅ CMD 0x3E raw=UI-1, tail 00 00 | capture-mapped | no |
| USB Record Vol | ✅ 0x0096 + 1 | ✅ CMD 0x3E selector 0x03, raw=UI-1 | capture-mapped | no |
| Dance/Mic Trigger Threshold | 🟨 0x0093 structural seed, range-guarded | ✅ CMD 0x22 raw=dB+60 | capture-mapped write | no further daily-use capture |
| Dance/Mic Trigger Hold Time | 🟨 0x0094 structural seed, range-guarded | ✅ CMD 0x22 raw seconds | capture-mapped write | no further daily-use capture |
| Adj Manner / VR OFF | ❌ reconnect/readback unknown | ✅ CMD 0x07 + F8 ACK | partial | **YES — readback only if persistent status desired** |

## Identity / security

| Function | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| BT Name | ✅ active memory 0x0385 | ✅ CMD 0x4E SET/RESET + B1 ACK + readback | capture-mapped | no |
| BLE Name | ✅ active memory 0x0398 | ⛔ intentionally read-only; do not infer from BT | non-blocking | no |
| BT Reset | ✅ readback after operation | ✅ CMD 0x4E op 0x00 | capture-mapped | no |
| Lock state | ❌ | ⛔ intentionally unsupported | out of daily-use scope | no |
| Lock password / Modify | ❌ | ⛔ intentionally unsupported | out of daily-use scope | no |
| Admin/User mode state | ❌ | ⛔ intentionally unsupported | out of daily-use scope | no |
| Admin password / Modify | ❌ | ⛔ intentionally unsupported | out of daily-use scope | no |

## Highest-value capture order

### P0 — capture first

1. **Music Tone remaining WRITE** — Mid, Mid Frequency, Treble. Noise Gate and Bass READ/WRITE are now closed.
2. **Mic Noise Gate write**.
4. **Top-Music preservation tail only** — Mic `CMD 0x11` final state-byte donor is now closed as Music Input1 Gain raw / direct `activeMemory[0x0016]`. Top-Music `CMD 0x02` trailing-byte source still needs one focused capture.

### P1 — useful next

6. Output Delay READ+WRITE is now closed for all six semantic controls; reconnect capture 5/10/15/20/25/30 proves the contiguous high-scalar block.
7. Adj Manner / VR OFF **readback** only if reconnect-persistent status is required; setter/ACK is mapped.
8. BT Name rename/reset is mapped. BLE identity remains intentionally read-only; no inference planned.
9. Equipment Mode rename needs no further packet capture; perform physical acceptance of the mapped Store workflow instead.

### P2 — last / potentially destructive

10. Reset All Settings.
11. Lock state/password flows — **scope closed / intentionally unsupported**.
12. Admin/User mode and password flows — **scope closed / intentionally unsupported**.

## Recommended capture method

For an ordinary scalar/control mapping:

```text
CONNECT
wait until native app is fully ready
record baseline value
change exactly one control by a small known step
wait 1–2 s
change the same control by another known step
wait 1–2 s
restore original value
wait 1–2 s
DISCONNECT
stop capture
```

For a connect-time READ/status mapping:

```text
set state A
disconnect
start/continue capture
connect and wait for all startup traffic/readback
disconnect

set state B
connect again and wait for all startup traffic/readback
disconnect
stop capture
```

For min/max ranges, screenshots of the native UI at both endpoints should accompany the packet capture. Avoid Reset/Store/credential operations until ordinary live/runtime mappings are finished.
