# SonKuPik K500 — Capability & Protocol Parity Matrix

> **Stable regression contract.** Public stable remains v1.0.3 while `main` may advance through the v1.1 candidate line. New work must preserve the device-truth, byte-preservation, fail-closed, and runtime-stability invariants established by the v1 USB baseline.

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
- `CAPTURE-MAPPED 🟧` — exact native packet mapping is implemented and regression-guarded, but the capability is not promoted to a full release hardware-qualification claim yet.

## P0 non-negotiable invariants

1. QML never talks directly to raw device I/O.
2. LIVE path remains `QML -> StudioEngine -> K500Controller -> K500DeviceManager -> K500WinIo`.
3. Transactional preset path remains `System UI -> K500PresetManager -> K500DeviceManager -> K500WinIo`.
4. On connect/recall, the device is source of truth.
5. Connect order remains heartbeat -> handshake -> full `0x03AB` / 939-byte readback -> hydrate while LIVE is OFF -> LIVE ON.
6. hydration must emit **zero** replay edits/device writes.
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
| Mute / media transport | STABLE USB ✅ | golden vectors + captured C0/E3 runtime-state decode |
| Music master/input/key block | STABLE USB ✅ | mirrored-scalar safety vector |
| PEQ: Mic A/B, Music, Main, Surround, Center, Sub, Reverb, Echo | STABLE USB ✅ | donor-verified command family; unsupported detail fields stay read-only |
| Verified crossover selectors | STABLE USB ✅ | `CMD 0x11` golden vectors |
| Music HP/LP filter type readback | CAPTURE-MAPPED 🟧 | READ activeMemory[0x0007]/[0x0008] + existing CMD 0x11 write enum |
| Mic HP/LP filter type readback | CAPTURE-MAPPED 🟧 | READ direct activeMemory[0x0013]/[0x0014]; LP has physical multi-state sweep, HP closed by State-A reconnect; Mic CMD 0x11 tail donor = current Music Input1 raw |
| Main/Surround/Center/Sub HP/LP filter type readback | CAPTURE-MAPPED 🟧 | State-A reconnect: Main 0x002C/0x002E, Surround 0x0040/0x0042, Center 0x0054/0x0056, Sub 0x0068/0x006A |
| Top Mic `CMD 0x05` | STABLE USB ✅ | mirrored unrelated scalars + captured FBX 0..4 direct byte |
| Top Effect `CMD 0x09` | STABLE USB ✅ | current hydrated Effect Init preservation |
| Music Init / Mic Init live write | CAPTURE-MAPPED 🟧 | CMD 0x02 / 0x05 writable second scalar; 0..84 |
| Effect Init live write | CAPTURE-MAPPED 🟧 | dedicated CMD 0x0A + RSP 0xF5; 0..84 |
| USB Record Volume | CAPTURE-MAPPED 🟧 | CMD 0x3E selector 0x03; UI 1..6 -> raw 0..5 |
| UDisk Record Volume | CAPTURE-MAPPED 🟧 | CMD 0x3E raw UI-1 + 00 00; RSP 0xC1 |
| Mic Max Volume | CAPTURE-MAPPED 🟧 | Top Mic CMD 0x05 third scalar; 0..84 hard ceiling for Top Mic |
| Dance Mic Trigger | CAPTURE-MAPPED 🟧 | paired CMD 0x22; threshold -60..0 dB + hold 1..30 s; guarded structural seed |
| BT Name rename/reset | CAPTURE-MAPPED 🟧 | USB CMD 0x4E SET/RESET + RSP 0xB1 + 939-byte identity refresh |
| BLE Name rename/reset | READ ONLY 🟦 | readback exists; deliberately not inferred from BT CMD 0x4E |
| Adj Manner / VR OFF setter | CAPTURE-MAPPED 🟧 | CMD 0x07 + RSP 0xF8; reconnect state remains unknown |
| Mic EQ Link | STABLE USB ✅ | captured command vector |
| Main output block | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Main L/R Output Delay | CAPTURE-MAPPED 🟧 | READ file 0x00D4/0x00D6 -> active 0x00CB/0x00CD; WRITE CMD 0x0E data16=L/data18=R, 0..50 ms |
| Surround output block | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Surround L/R Output Delay | CAPTURE-MAPPED 🟧 | READ file 0x00D8/0x00DA -> active 0x00CF/0x00D1; WRITE exception data16=R/data18=L, 0..50 ms |
| Center output block | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Center Output Delay | CAPTURE-MAPPED 🟧 | READ file 0x00DC -> active 0x00D3; WRITE CMD 0x0E data16, 0..50 ms |
| Sub output block | STABLE USB ✅ | raw-block seed + neighboring-byte preservation |
| Subwoofer Output Delay | CAPTURE-MAPPED 🟧 | READ file 0x00DE -> active 0x00D5; WRITE CMD 0x0E data16, 0..50 ms |
| Reverb detail level/direct/decay/predelay/HPF/LPF live write | CAPTURE-MAPPED 🟧 | captured full-image CMD 0x0B; native UI is frequency-only (no HP/LP Type) |
| Echo detail level/repeat/direct/left delay/left predelay/right delay/right predelay/HPF/LPF live write | CAPTURE-MAPPED 🟧 | captured full-image CMD 0x0D; right timing -50..+50%, left predelay 0..100 ms; no HP/LP Type control |
| Music Noise Gate / Bass READ+WRITE | CAPTURE-MAPPED 🟧 | Gate READ direct 0x0005 + CMD 0x02; Bass READ direct 0x00DF + CMD 0x0C; reconnect truth captured 2026-10-04 |
| Mic FBX / anti-feedback level | CAPTURE-MAPPED 🟧 | READ activeMemory[0x001B], WRITE CMD 0x05 levels 0..4, RSP 0xFA |
| Mic gate live write | READ ONLY 🟦 | no verified live command; do not guess |
| Equipment Mode Recall 1–10 | STABLE USB ✅ | `0x01 -> settle -> 0x3F/C0 -> 939-byte resync` |
| Use Init Volume | STABLE USB ✅ | exact `CMD 0x12` / `RSP 0xED` + captured C0 connect-state bit |
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
| Official Mode 01 `KONSER NYANYI` | LOCKED SW ✅ | current official file SHA-256 `4d1f2dd4f5431de1df1819931ecf65be4242e2bc9ae4e9dbabdbee3504004cf1`; evolved from the physically proven native `CONCERT HIFI V4` donor lineage |
| Support Report diagnostics | LOCKED SW ✅ | bounded schema + payload/path redaction guard |
| Inno Setup Windows installer | LOCKED SW ✅ | actual silent install + installed-app runtime tests |
| Smart Installer distribution | LOCKED SW ✅ | machine/per-user installer runtime self-tests; portable retired from v1.1 public distribution |
| Persistent LCD/Equipment Mode rename | CAPTURE-MAPPED 🟧 | active-slot name at slot-image 0x0280..0x028F + native Store + Recall/readback |
| Mic Max Volume | CAPTURE-MAPPED 🟧 | captured CMD 0x05 scalar + Top Mic hard ceiling |
| UDisk Record Volume | CAPTURE-MAPPED 🟧 | captured CMD 0x3E raw UI-1 + RSP 0xC1 |
| Dance Mic threshold/hold | CAPTURE-MAPPED 🟧 | captured full-pair CMD 0x22; write is fail-closed without valid paired seed |
| BT Name rename/reset | CAPTURE-MAPPED 🟧 | USB CMD 0x4E + B1 ACK + authoritative identity refresh |
| BLE Name rename/reset | READ ONLY 🟦 | no BLE write capture; no BT-family inference |
| Lock/Admin credentials | READ ONLY 🟦 | intentionally device-managed/outside daily-use product scope; no guessed traffic |

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

Current official Mode 01 is `KONSER NYANYI`:

`4d1f2dd4f5431de1df1819931ecf65be4242e2bc9ae4e9dbabdbee3504004cf1`

Its rollback/provenance lineage retains the physically proven native `CONCERT HIFI V4` donor (`9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74`). The current official file and the historical donor must not be conflated. Checksum validity alone is not proof of native-equivalent behavior.

## Release phases

### P0 — Native architecture / regression fortress — COMPLETE

Connection architecture, readback, hydration, core UI interaction, and protocol invariants are guarded.

### P1 — Donor-verified LIVE command surface — COMPLETE FOR v1 USB SCOPE

Verified command families are available in the stable USB workflow. Commands without evidence remain read-only.

### P2 — Device preset transactions — COMPLETE FOR v1 USB SCOPE

Recall, Use Init, current-device Save, Store chain, and Mass Upload transaction behavior are integrated in the stable baseline. The 2026-10-04 capture set additionally maps active-slot persistent Mode Name rename through that same Store coordinator without introducing a second destructive transaction path.

### P3 / P3.2 / P3.3 / P3.4 — Bit-perfect preset engine and file UI — COMPLETE

Parser, donor corpus, file bridge, explicit Preview, controlled persistence, atomic Save As, and slot conversion are protected by regression tests.

### P4 / P4.2 — PC Upload + deterministic Mass Upload — COMPLETE FOR v1 USB SCOPE

Single Upload and multi-file transfer-list Mass Upload use the proven permanent Store path and hardware-truth resync.

### P5 — Packaging / release qualification — COMPLETE FOR v1.0; v1.1 PROMOTION GATED

Public v1.1 packaging uses the Smart Installer plus the scope-matched per-user updater package, with runtime package tests, SHA-256 hashes, release manifest, stable promotion workflow, and explicit USB-vs-Bluetooth support scope. Portable is retired from the v1.1 public distribution. The updater lifecycle is functionally accepted, but public v1.1 promotion remains fail-closed until one exact immutable RC built from current release truth has its tag, commit, and two installer SHA-256 values recorded in `V1_1_UPDATER_RC_ACCEPTANCE.md`.

## Merge rule

A PR that makes the product look more complete but weakens any existing architecture, protocol, preset, runtime, or fail-closed invariant is a regression and must not merge. Progress is measured by safer, evidence-backed capability—not by the number of writable controls.
