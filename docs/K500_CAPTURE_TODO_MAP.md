# K500 Capture Coverage & TODO Map

Status date: 2026-09-18
Branch reference: `fix/p3-1-nondisruptive-native-ranges` / PR #79

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
| Mute state on connect/runtime | ❌ | ✅ CMD 0x15 | partial | **YES — Mute OFF connect vs Mute ON connect** |
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
| HPF/LPF filter type on WRITE | n/a | ✅ existing CMD 0x11 mapping | partial | optional reconfirm only |
| HPF/LPF filter type on CONNECT | ❌ currently assumed/defaulted | n/a | partial | **YES** |
| Music Noise Gate | ❌ | ❌ controller unsupported | missing | **YES** |
| Music Bass | ❌ | ❌ controller unsupported | missing | **YES** |
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
| Mic HPF/LPF filter type on CONNECT | ❌ assumed/defaulted | n/a | partial | **YES** |
| Mic Noise Gate | ✅ active memory 0x0016 | ❌ no donor-verified write | partial | **YES — write delta** |
| FBE/FBX level | ✅ active memory 0x001B | ❌ current top-Mic block preserves device bytes | partial | **YES — write delta** |

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
| HPF filter type | ❌ | ❌ | ❌ | **YES** |
| LPF filter type | ❌ | ❌ | ❌ | **YES** |

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
| HPF filter type | ❌ | ❌ | **YES** |
| LPF filter type | ❌ | ❌ | **YES** |
| Exact native min/max for Right Delay / Right Predelay / Left Predelay | 🟨 encoding known, UI endpoint not fully frozen | ✅ mapping exists | **YES, low risk range sweep** |

## Outputs: Main / Surround / Center / Sub

| Control family | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| Output volume(s) | ✅ | ✅ CMD 0x0E | complete | no |
| Mic/Music/Reverb/Echo mix levels | ✅ | ✅ | complete | no |
| Compressor threshold/ratio/attack/release | ✅ | ✅ | complete | no |
| PEQ bands | ✅ | ✅ | complete | no |
| EQ bypass | ✅ | ✅ | complete | no |
| HPF/LPF frequency | ✅ | ✅ | complete | no |
| HPF/LPF filter type on CONNECT | ❌ assumed/defaulted | n/a | partial | **YES** |
| Surround L/R Delay | ✅ | ✅ CMD 0x0E | complete | no |
| Main L/R Delay | ❌ currently UI read-only/fallback | ❌ output block does not patch | missing | **YES** |
| Center Output Delay | ❌ | ❌ | missing | **YES** |
| Sub Output Delay | ❌ | ❌ | missing | **YES** |

## System / Equipment Mode

| Control | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| Mode names 1..10 | ✅ active memory | ❌ persistent rename not mapped | partial | **YES — rename one sacrificial slot** |
| Active Mode name/slot | ✅ | ✅ Recall | complete | no |
| Use Init Volume | ✅ | ✅ | complete | no |
| Save current slot | ✅ transaction | ✅ Store 0x41/42/43 | complete | no |
| Mass Upload | n/a | ✅ | complete | no |
| Reset All Settings | ❌ | ❌ disabled | missing | **YES — destructive, last priority** |
| Music Init Vol | ✅ 0x000B | ❌ preserved-only | partial | **YES** |
| Music Max Vol | ✅ 0x000C | ❌ preserved-only | partial | **YES** |
| Mic Init Vol | ✅ 0x0012 | ❌ preserved-only | partial | **YES** |
| Mic Max Vol | ✅ 0x0013 | ❌ preserved-only | partial | **YES** |
| Effect Init Level | ✅ 0x001D | ❌ current write path is effectively mirrored/preserved | partial | **YES** |
| UDisk Record Vol | ✅ 0x0095 + 1 | ❌ UI local-only | partial | **YES** |
| USB Record Vol | ✅ 0x0096 + 1 | ❌ UI local-only | partial | **YES** |
| Dance/Mic Trigger Threshold | ❌ UI currently hardcoded | ❌ | missing | **YES** |
| Dance/Mic Trigger Hold Time | ❌ UI currently hardcoded | ❌ | missing | **YES** |
| Adj Manner / VR OFF | ❌ not represented | ❌ | missing | **YES if feature desired** |

## Identity / security

| Function | READ | WRITE | Status | Capture needed |
| --- | --- | --- | --- | --- |
| BT Name | ✅ active memory 0x0385 | ❌ rename/reset | partial | **YES** |
| BLE Name | ✅ active memory 0x0398 | ❌ rename/reset | partial | **YES** |
| BT/BLE Reset | ❌ | ❌ | missing | **YES** |
| Lock state | ❌ | ❌ | missing | **YES, cautious** |
| Lock password / Modify | ❌ | ❌ | missing | **YES, cautious/destructive** |
| Admin/User mode state | ❌ | ❌ | missing | **YES, cautious** |
| Admin password / Modify | ❌ | ❌ | missing | **YES, cautious/destructive** |

## Highest-value capture order

### P0 — capture first

1. **Mute OFF-connect vs ON-connect** — closes another false-local-runtime-status risk like Play/Pause.
2. **Music Tone block** — Noise Gate, Bass, Mid, Mid Frequency, Treble, one parameter per capture.
3. **Mic Noise Gate write**.
4. **FBE/FBX write**.
5. **System Startup Limits** — Music Init, Music Max, Mic Init, Mic Max, Effect Init.
6. **Recording + Mic Trigger** — UDisk Rec, USB Rec, Threshold, Hold Time.
7. **Crossover filter-type readback** — connect after changing one representative non-FX type to a non-default value.
8. **Reverb HPF/LPF type** and **Echo HPF/LPF type** — dedicated write deltas.

### P1 — useful next

9. Main L/R delay, Center delay, Sub delay.
10. Equipment Mode Name rename.
11. BT Name and BLE Name rename/reset.
12. Adj Manner / VR OFF.

### P2 — last / potentially destructive

13. Reset All Settings.
14. Lock state/password flows.
15. Admin/User mode and password flows.

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
