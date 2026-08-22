# SONKUPIK STUDIO Qt Porting Parity Matrix

> **P0 regression contract.** `main` must remain releasable. A capability may move to `LOCKED ✅` only after its donor behavior/protocol is identified, automated tests pass, and hardware acceptance is recorded where the capability touches a real K500.

## Status legend

- `LOCKED ✅` — implementation is protected by regression evidence; hardware-facing rows also require recorded physical acceptance.
- `IMPLEMENTED ⚠️` — code and automated parity guards exist, but physical hardware acceptance is still required.
- `READ ONLY 🟦` — current K500 value is decoded/hydrated, but no verified donor/capture live-write command exists.
- `NOT PORTED ❌` — donor capability still needs a native implementation.
- `DONOR ONLY 🟨` — behavior is proven in `masarray/ktv-studio-mixer-pro` and remains the specification.

## P0 non-negotiable invariants

These are the golden baseline and must not regress in any later phase:

1. QML never talks directly to raw device I/O.
2. Native live path stays `QML -> StudioEngine -> K500Controller -> K500DeviceManager -> K500WinIo`.
3. On connect, the device is source of truth.
4. Connect order remains heartbeat -> handshake -> full `0x03AB` (939-byte) active-memory readback -> hydrate editor while LIVE is OFF -> LIVE ON.
5. hydration must emit **zero** `stateEdited` writes.
6. Bluetooth CMD `0x40` read mode remains `0x63`; USB HID mode remains `0x00`.
7. USB remains VID/PID `10C4:0321`, report ID 0, 64-byte HID reports.
8. Verified protocol bytes are never changed without a new golden vector and an explicit donor/capture justification.
9. Controls without verified native writes remain non-destructive (`unsupportedPath`) rather than guessing bytes.
10. PEQ/fader/caption/Plus Jakarta Sans regression guards remain mandatory.

P2 adds one deliberate sibling path for transactional preset operations:

`System UI -> K500PresetManager -> K500DeviceManager -> K500WinIo`

`K500PresetManager` coordinates ACK/timeouts/readback but never bypasses `K500DeviceManager::writeFrame()` or owns a second transport handle.

## Functional parity

| Capability | Donor Web/Electron | Native Qt `k500` | Gate to LOCKED |
|---|---:|---:|---|
| Windows native Qt/QML architecture | N/A | LOCKED ✅ | CI build + architecture guard |
| USB HID connect | DONOR ONLY 🟨 | LOCKED ✅ | protocol/parser self-test + established hardware baseline |
| Bluetooth SPP connect | DONOR ONLY 🟨 | LOCKED ✅ | protocol/parser self-test + established hardware baseline |
| Heartbeat / handshake | DONOR ONLY 🟨 | LOCKED ✅ | golden vectors |
| Full 939-byte active-memory readback | DONOR ONLY 🟨 | LOCKED ✅ | engine self-test + zero-echo invariant |
| Hydrate current KTV into editor before LIVE | DONOR ONLY 🟨 | LOCKED ✅ | zero-echo hydration assertion |
| Mute / media transport | DONOR ONLY 🟨 | LOCKED ✅ | golden vectors |
| Music master/input/key block | DONOR ONLY 🟨 | LOCKED ✅ | mirrored-scalar golden vector |
| PEQ writes: Mic A/B, Music, Main, Surround, Center, Sub, Reverb, Echo | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | USB + BT hardware acceptance |
| Music crossover live write | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | hardware acceptance matrix |
| Mic crossover live write | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | shared selector routing + hardware |
| Main crossover live write | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | selector golden vector + hardware |
| Surround crossover live write | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | selector golden vector + hardware |
| Center crossover live write | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | selector golden vector + hardware |
| Sub crossover live write | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | selector golden vector + hardware |
| Reverb/Echo crossover live write | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | selector golden vectors + hardware |
| Top Mic `CMD 0x05` | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact donor vector + mirrored scalar safety + hardware |
| Top Effect `CMD 0x09` | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact donor vector + mirrored init safety + hardware |
| Mic EQ Link | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact donor vector + UI bridge + hardware |
| Main output block | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | donor vector + raw-block preservation + hardware |
| Surround output block + L/R delay | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | donor vector + raw-block preservation + hardware |
| Center output block | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | donor vector + raw-block preservation + hardware |
| Sub output block | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | donor vector + raw-block preservation + hardware |
| Reverb detail: level/decay/predelay | file/editor only | READ ONLY 🟦 | requires a verified live command/capture; do not guess |
| Echo detail: level/repeat/delay | file/editor only | READ ONLY 🟦 | requires a verified live command/capture; do not guess |
| Mic gate live write | file/editor only | READ ONLY 🟦 | requires a verified live command/capture; do not guess |
| Recall Equipment Mode 1–10 | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | `0x01 -> 80 ms -> 0x3F/C0 -> 939-byte resync` + hardware |
| Use Init Volume | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact `CMD 0x12` / `RSP 0xED` + hardware |
| Permanent Store Begin/Chunk/Commit | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | USB `0x41/0x42/0x43`, `BD/BC` ACKs + power-cycle |
| Mass Upload transaction engine | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | `BE -> BD* -> BC`, descending slots + chain + hardware |
| `.k500` exact-size/checksum parser | DONOR ONLY 🟨 | LOCKED ✅ | synthetic + real donor corpus |
| `.k500` bit-perfect no-op serialize | DONOR ONLY 🟨 | LOCKED ✅ | byte-identical donor round trip |
| `.k500` controlled edit persistence | DONOR ONLY 🟨 | LOCKED ✅ | explicit whitelist + checksum + donor regression |
| `.k500` atomic import/export | DONOR ONLY 🟨 | LOCKED ✅ | `QSaveFile` + donor corpus |
| `.k500` -> native `0x0290` slot image | DONOR ONLY 🟨 | LOCKED ✅ | split-scalar + compact-EQ donor tests |
| PC preset single-slot Upload UI/backend | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | P4 safety gate + physical Store/power-cycle |
| Deterministic multi-file preset batch | DONOR ONLY 🟨 | LOCKED ✅ | P4.2 donor batch test; whole-batch fail closed |
| Mass Upload preset-library UI | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | validated P4.2 source + physical multi-slot/power-cycle |
| Support-report diagnostics | N/A | IMPLEMENTED ⚠️ | P5 build/UI/report schema gate |
| Installer + true portable single EXE | N/A | IMPLEMENTED ⚠️ | P5 release workflow + package smoke tests |

## P1 implementation boundary

P1 means **full parity with the donor's verified live-command surface**, not guessed support for every visible editor field. The donor proves commands for Top Music, Top Mic, Top Effect, PEQ, documented crossover selectors, Mic EQ Link and Main/Surround/Center/Sub output blocks. It does not currently provide a verified live command for Mic gate or detailed Reverb/Echo timing/level fields. Those remain safely read-only for hardware until a capture proves their command semantics.

P1 output writes obey a stronger rule than naive reconstruction: block data is seeded from current device readback and only donor-verified fields are patched. Unknown/reserved bytes remain device truth.

## P2 implementation boundary

P2 owns **device-side preset transactions**, not `.k500` file semantics.

- Recall pauses LIVE, sends slot `CMD 0x01`, waits 80 ms, sends `CMD 0x3F`, requires `RSP 0xC0`, then re-reads all 939 active-memory bytes before LIVE returns.
- Use Init Volume uses captured `CMD 0x12` and requires `RSP 0xED`.
- Current-device single-slot Save is USB-only and uses fresh active memory as device truth.
- Store uses `CMD 0x41`, eleven `CMD 0x42` chunks (10x60 + 56 bytes) with `RSP 0xBD`, then `CMD 0x43` with `RSP 0xBC`.
- Mass Upload accepts validated `0x0290` images, sorts slots descending, waits for `RSP 0xBE` on each begin, carries the native three-byte chain, and recalls slot 1 after the batch like the donor.
- Any failed Recall/Store transaction fails closed: LIVE remains OFF and transport is dropped until a clean reconnect/full readback.

P3/P4 now provide the verified PC-file source that P2 intentionally did not invent.

## Phase gates

### P0 — Regression Fortress — COMPLETE

Architecture, golden vectors, full-memory hydration, zero-echo hydration and fundamental UI interaction invariants are continuously guarded.

### P1 — Full verified LIVE parity — CODE COMPLETE, HARDWARE GATE PENDING

All donor-verified live command families are routed. Physical USB and Bluetooth qualification must pass before remaining `IMPLEMENTED ⚠️` rows become `LOCKED ✅`.

### P2 — Device preset management — CODE COMPLETE, CI/HARDWARE GATE PENDING

Recall, Use Init, permanent current-device Save and the native Mass Upload transaction engine are code/CI complete. Physical persistence and failure-recovery tests remain mandatory.

### P3 — Bit-perfect `.k500` engine — SOFTWARE COMPLETE

- exact 1144-byte parser/checksum;
- byte-identical no-op serialization;
- unknown/reserved-byte preservation;
- controlled edit whitelist;
- donor-backed `0x0290` slot conversion;
- atomic Save As.

### P4 / P4.2 — PC preset upload and batch library — SOFTWARE COMPLETE, HARDWARE GATE PENDING

- selected-slot PC preset permanent Upload;
- no device readback may replace the PC image before Store;
- deterministic 1–10 file batch validation and slot assignment;
- validated entries feed the existing native descending Mass Upload chain;
- physical single-slot and multi-slot power-cycle qualification remains pending.

### P5 — Release candidate hardening — SOFTWARE RC COMPLETE, HARDWARE ACCEPTANCE PENDING

- public documentation and GPL-3.0-or-later licensing are committed;
- bounded support diagnostics redact active-memory and preset payloads;
- release-candidate version metadata is `0.5.0`;
- installer/portable packaging, SHA-256 and machine-readable release manifest passed CI;
- full P0–P4.2 regression fortress and runtime smoke tests passed on the P5 merge gate;
- stable `v1.0` remains blocked until physical USB/BT/persistence qualification is recorded.

## Merge rule

A PR that makes the matrix look more complete but breaks any P0/P1/P2/P3/P4 invariant is a regression and must not merge. Progress is measured only by capabilities that become safer and more complete while earlier gates stay green.