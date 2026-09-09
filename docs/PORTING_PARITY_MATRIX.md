# SonKuPik K500 — Capability & Protocol Parity Matrix

> **Stable regression contract.** `main` represents the public stable baseline. New work may expand capability, but it must not weaken device-truth, byte-preservation, fail-closed, or runtime-stability invariants that already protect v1.0.

## Support boundary

**v1.0 hardware-qualified scope:** Windows 10/11 x64 + K500 over USB HID.

Bluetooth SPP remains implemented and useful for engineering, but is explicitly **experimental** in the v1.0 support policy until it receives independent physical acceptance.

## Status legend

- `STABLE USB ✅` — part of the v1 public stable Windows/USB support surface and protected by automated regression tests.
- `LOCKED SW ✅` — deterministic software/file behavior protected by regression evidence; does not itself claim a physical transport test.
- `EXPERIMENTAL 🧪` — implemented but outside the current hardware-qualified support scope.
- `READ ONLY 🟦` — value can be represented/read, but no donor-verified persistent/live write exists.
- `NOT PORTED ❌` — known donor capability has no native implementation.
- `DONOR EVIDENCE 🟨` — historical donor/capture remains a specification reference, not a runtime dependency.

## P0 non-negotiable invariants

1. QML never talks directly to raw device I/O.
2. LIVE path remains `QML -> StudioEngine -> K500Controller -> K500DeviceManager -> K500WinIo`.
3. Transactional preset path remains `System UI -> K500PresetManager -> K500DeviceManager -> K500WinIo`.
4. On connect/recall, the device is source of truth.
5. Connect order remains heartbeat -> handshake -> full `0x03AB` / 939-byte readback -> hydrate while LIVE is OFF -> LIVE ON.
6. Hydration must emit **zero** replay edits/device writes.
7. USB remains VID/PID `10C4:0321`, report ID 0, 64-byte HID reports.
8. Bluetooth `CMD 0x40` read mode remains `0x63`; USB read mode remains `0x00`.
9. Verified protocol bytes never change without donor/capture evidence and golden-vector updates.
10. An `unsupportedPath` remains non-destructive instead of guessing bytes.
11. Preset no-op operations remain byte-identical and preserve unknown/reserved data.
12. Section navigation must retain stable EQ graph/model lifetimes and pass the runtime navigation stress test.

## Functional matrix

| Capability | Native Qt v1 status | Evidence / boundary |
|---|---:|---|
| Windows native Qt/QML application | STABLE USB ✅ | clean Windows build + deployed runtime tests |
| USB HID connect | STABLE USB ✅ | VID/PID + heartbeat/handshake/readback guards |
| Bluetooth SPP connect | EXPERIMENTAL 🧪 | implementation exists; independent physical qualification pending |
| Heartbeat / handshake | STABLE USB ✅ | protocol golden vectors |
| Full 939-byte active-memory readback | STABLE USB ✅ | engine/runtime tests + hardware-truth workflow |
| Hydrate K500 before LIVE | STABLE USB ✅ | zero-echo hydration invariant |
| Section navigation across Mic/Reverb/Echo/outputs/System | STABLE USB ✅ | repeated runtime stress test; fixed page/model lifetime |
| Mute / media transport | STABLE USB ✅ | golden vectors + native routing |
| Music master/input/key block | STABLE USB ✅ | mirrored-scalar safety vector |
| PEQ: Mic A/B, Music, Main, Surround, Center, Sub, Reverb, Echo | STABLE USB ✅ | donor-verified command family; unsupported detail fields stay read-only |
| Verified crossover selectors | STABLE USB ✅ | `CMD 0x11` golden vectors |
| Top Mic `CMD 0x05` | STABLE USB ✅ | mirrored scalar preservation |
| Top Effect `CMD 0x09` | STABLE USB ✅ | mirrored init preservation |
| Mic EQ Link | STABLE USB ✅ | captured command vector |
| Main output block | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Surround output block + L/R delay | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Center output block | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Sub output block | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Reverb detail level/decay/predelay live write | READ ONLY 🟦 | no verified live command; do not guess |
| Echo detail level/repeat/delay live write | READ ONLY 🟦 | no verified live command; do not guess |
| Mic gate live write | READ ONLY 🟦 | no verified live command; do not guess |
| Equipment Mode Recall 1–10 | STABLE USB ✅ | `0x01 -> settle -> 0x3F/C0 -> 939-byte resync` |
| Use Init Volume | STABLE USB ✅ | exact `CMD 0x12` / `RSP 0xED` |
| Current-device permanent Save | STABLE USB ✅ | native Store path; USB-only |
| Store Begin/Chunk/Commit | STABLE USB ✅ | `0x41/0x42/0x43`, `BD/BC` transaction guards |
| Mass Upload transaction engine | STABLE USB ✅ | batch validation + descending slot order + chain + final recall |
| `.k500` exact-size/checksum parser | LOCKED SW ✅ | synthetic + donor corpus |
| `.k500` byte-identical no-op | LOCKED SW ✅ | donor round-trip tests |
| `.k500` controlled edit persistence | LOCKED SW ✅ | whitelist + checksum + donor regression |
| `.k500` atomic Save As | LOCKED SW ✅ | `QSaveFile` + donor corpus |
| `.k500` -> native `0x0290` slot image | LOCKED SW ✅ | split scalar + compact EQ conversion tests |
| PC preset single-slot Upload | STABLE USB ✅ | validated slot image -> native Store -> Recall/readback |
| Multi-file batch validation | LOCKED SW ✅ | whole-batch fail-closed P4.2 regression |
| Unified SONKUPIK + LOCAL preset library | LOCKED SW ✅ | source provenance separated; both use same validator |
| Official GitHub preset sync/cache | LOCKED SW ✅ | validation before cache promotion; offline fallback |
| Native Mode 01 `CONCERT HIFI V4` | LOCKED SW ✅ | exact donor SHA-256 guard + physical hardware acceptance |
| Support Report diagnostics | LOCKED SW ✅ | bounded schema + payload/path redaction guard |
| Inno Setup Windows installer | LOCKED SW ✅ | actual silent install + installed-app runtime tests |
| Portable ZIP | LOCKED SW ✅ | extraction + runtime self-tests |
| Persistent LCD/Equipment Mode rename | READ ONLY 🟦 | table readback exists; write transaction not donor-verified |

## State model parity

The v1 System workspace intentionally separates:

```text
Hardware State
  actual K500 readback + active slot + device mode names

Staged PC Preset
  selected Official/Local .k500 only

Offline Preview State
  explicit Preview hydration + controlled edit tracking

Mass Upload Staging
  reviewed Slot 01…10 mapping before permanent write
```

A PC file selection must never silently become Hardware State.

## Preset transaction boundary

### Recall

- pause LIVE;
- send `CMD 0x01`;
- settle;
- send `CMD 0x3F`;
- require `RSP 0xC0`;
- read all 939 active-memory bytes;
- hydrate from hardware;
- resume LIVE only after successful refresh.

### Store

- native slot image is exactly 656 bytes;
- `CMD 0x41` begins Store;
- `CMD 0x42` sends ten 60-byte chunks plus one final 56-byte chunk;
- each chunk requires `RSP 0xBD`;
- `CMD 0x43` commits and requires `RSP 0xBC`.

### Mass Upload

- validate the complete staged batch before hardware writes;
- execute selected destinations highest-to-lowest;
- for a full bank: **10 -> 09 -> ... -> 01**;
- carry the native inter-slot chain;
- after final Slot 01 commit, Recall Slot 01 and perform full readback before LIVE.

Any uncertain destructive transaction fails closed and requires clean reconnect/readback.

## Official preset library boundary

The left Mass Upload collection is unified but provenance remains explicit:

- **SONKUPIK:** bundled official files plus validated official cache updates;
- **LOCAL:** user-owned files from the selected local folder.

Remote official files do not bypass the codec/validator and never overwrite Local user presets.

Mode 01 is the exact native donor:

`9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74`

Checksum validity alone is not treated as proof that a preset is native-equivalent.

## Release phases

### P0 — Native architecture / regression fortress — COMPLETE

Connection architecture, readback, hydration, core UI interaction, and protocol invariants are guarded.

### P1 — Donor-verified LIVE command surface — COMPLETE FOR v1 USB SCOPE

Verified command families are available in the stable USB workflow. Commands without evidence remain read-only.

### P2 — Device preset transactions — COMPLETE FOR v1 USB SCOPE

Recall, Use Init, current-device Save, Store chain, and Mass Upload transaction behavior are integrated in the stable baseline.

### P3 / P3.2 / P3.3 / P3.4 — Bit-perfect preset engine and file UI — COMPLETE

Parser, donor corpus, file bridge, explicit Preview, controlled persistence, atomic Save As, and slot conversion are protected by regression tests.

### P4 / P4.2 — PC Upload + deterministic Mass Upload — COMPLETE FOR v1 USB SCOPE

Single Upload and multi-file transfer-list Mass Upload use the proven permanent Store path and hardware-truth resync.

### P5 — Packaging / release qualification — COMPLETE

Public stable packaging is Inno Setup + ordinary portable ZIP, with runtime package tests, SHA-256 hashes, release manifest, stable release workflow, and explicit USB-vs-Bluetooth support scope.

## Merge rule

A PR that makes the product look more complete but weakens any existing architecture, protocol, preset, runtime, or fail-closed invariant is a regression and must not merge. Progress is measured by safer, evidence-backed capability—not by the number of writable controls.
