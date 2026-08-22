# SONKUPIK STUDIO Qt Porting Parity Matrix

> **P0 regression contract.** `main` must remain releasable. A capability may move to `LOCKED ✅` only after its donor behavior/protocol is identified, automated tests pass, and hardware acceptance is recorded where the feature touches a real K500.

## Status legend

- `LOCKED ✅` — implemented and protected by automated regression tests; hardware-facing behavior is already part of the accepted baseline.
- `IMPLEMENTED ⚠️` — code exists and automated parity guards pass, but physical hardware acceptance and/or a declared upstream dependency is still required.
- `READ ONLY 🟦` — current K500 value is decoded/hydrated, but no verified donor/capture live-write command exists yet.
- `NOT PORTED ❌` — donor capability still needs a native Qt implementation.
- `DONOR ONLY 🟨` — proven in `masarray/ktv-studio-mixer-pro`; donor is the specification until native parity is complete.

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
9. Controls without verified native writes must remain non-destructive (`unsupportedPath`) rather than guessing bytes.
10. PEQ/fader/caption/Plus Jakarta Sans regression guards remain mandatory.

P2 adds one deliberate sibling path for transactional preset operations:

`System UI -> K500PresetManager -> K500DeviceManager -> K500WinIo`

`K500PresetManager` may coordinate ACK/timeouts/readback but never bypasses `K500DeviceManager::writeFrame()` or owns a second transport handle.

## Functional parity

| Capability | Donor Web/Electron | Native Qt `k500` | Gate to LOCKED |
|---|---:|---:|---|
| Windows native Qt/QML architecture | N/A | LOCKED ✅ | CI build + architecture guard |
| USB HID connect | DONOR ONLY 🟨 | LOCKED ✅ | protocol/parser self-test + hardware baseline |
| Bluetooth SPP connect | DONOR ONLY 🟨 | LOCKED ✅ | protocol/parser self-test + hardware baseline |
| Heartbeat / handshake | DONOR ONLY 🟨 | LOCKED ✅ | golden vectors |
| Full 939-byte active-memory readback | DONOR ONLY 🟨 | LOCKED ✅ | engine self-test + CI invariant guard |
| Hydrate current KTV into editor before LIVE | DONOR ONLY 🟨 | LOCKED ✅ | zero-echo hydration assertion |
| Mute / media transport | DONOR ONLY 🟨 | LOCKED ✅ | golden vectors |
| Music master/input/key block | DONOR ONLY 🟨 | LOCKED ✅ | mirrored-scalar golden vector |
| PEQ writes: Mic A/B, Music, Main, Surround, Center, Sub, Reverb, Echo | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | hardware acceptance matrix |
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
| Main output block | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact donor vector + raw-block preservation + hardware |
| Surround output block + L/R delay | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact donor vector + raw-block preservation + hardware |
| Center output block | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact donor vector + raw-block preservation + hardware |
| Sub output block | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact donor vector + raw-block preservation + hardware |
| Reverb detail: level/decay/predelay | file/editor only | READ ONLY 🟦 | requires a verified live command/capture; do not guess |
| Echo detail: level/repeat/delay | file/editor only | READ ONLY 🟦 | requires a verified live command/capture; do not guess |
| Mic gate live write | file/editor only | READ ONLY 🟦 | requires a verified live command/capture; do not guess |
| Recall Equipment Mode 1-10 | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | `0x01 -> 80 ms -> 0x3F/C0 -> 939-byte resync` + hardware |
| Use Init Volume | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | exact `CMD 0x12` / `RSP 0xED` vector + hardware |
| Permanent Store Begin/Chunk/Commit | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | USB-only `0x41/0x42/0x43`, `BD/BC` ACKs + power-cycle hardware test |
| Mass Upload transaction engine | DONOR ONLY 🟨 | IMPLEMENTED ⚠️ | `BE -> BD* -> BC`, descending slots + chain; raw slot-image source arrives P3/P4 |
| Mass Upload preset-library UI | DONOR ONLY 🟨 | NOT PORTED ❌ | requires P3 codec + P4 preset source; no fake enabled button in P2 |
| `.k500` parse | DONOR ONLY 🟨 | NOT PORTED ❌ | fixture corpus |
| `.k500` bit-perfect no-op serialize | DONOR ONLY 🟨 | NOT PORTED ❌ | byte-identical round trip |
| `.k500` import/export | DONOR ONLY 🟨 | NOT PORTED ❌ | codec + checksum + whitelist tests |
| Preset library | DONOR ONLY 🟨 | NOT PORTED ❌ | codec must be LOCKED first |

## P1 implementation boundary

P1 means **full parity with the donor's verified live-command surface**, not guessed support for every visible editor field. The donor has proven commands for Top Music, Top Mic, Top Effect master, PEQ, all documented crossover selectors, Mic EQ Link and Main/Surround/Center/Sub output blocks. It does not currently provide a verified live command for Mic gate or the detailed Reverb/Echo timing/level fields. Those controls remain safely read-only for hardware until a capture proves their command semantics.

P1 output writes obey a stronger native rule than a naive reconstruction: all 35 output data bytes are seeded from the current device readback and only donor-verified fields are patched. Unknown/reserved bytes remain device truth.

## P2 implementation boundary

P2 owns **device-side preset transactions**, not `.k500` file semantics.

- Recall uses the captured destination mask `0x03`, pauses LIVE, sends slot `CMD 0x01`, waits 80 ms, sends recall handshake `CMD 0x3F`, requires `RSP 0xC0`, then re-reads all 939 active-memory bytes before LIVE returns.
- Use Init Volume uses captured `CMD 0x12` and requires `RSP 0xED`.
- Single-slot Save is USB-only. Before Store it performs a fresh active-memory read and takes the first `0x0290` bytes directly as the permanent slot image. This makes the K500 RAM itself the source of truth and avoids a premature `.k500` serializer dependency.
- Store uses `CMD 0x41`, eleven `CMD 0x42` chunks (10x60 + 56 bytes) with `RSP 0xBD`, then `CMD 0x43` with `RSP 0xBC`.
- Mass Upload accepts already-built `0x0290` images, sorts slots descending, waits for `RSP 0xBE` on each begin, carries the native three-byte chain, and recalls slot 1 after the batch like the donor.
- Any failed Recall/Store transaction fails closed: LIVE remains OFF and the transport is dropped until a clean reconnect/full readback.
- PC preset file buttons stay disabled until P3/P4 provides a bit-perfect slot-image source. P2 must not simulate a Mass Upload that it cannot source safely.

## Phase gates

### P0 — Regression Fortress — COMPLETE

- parity matrix committed;
- current protocol vectors frozen;
- full-memory hydration test expanded;
- CI checks P0 invariants;
- hardware acceptance checklist committed.

### P1 — Full verified LIVE parity — CODE COMPLETE, HARDWARE GATE PENDING

- Top Mic protocol + routing;
- Top Effect protocol + routing;
- Main/Surround/Center/Sub output block protocol + routing;
- all donor-verified crossover routing;
- Mic EQ Link protocol + UI routing;
- QML rack controls enter only through `StudioEngine.editDevicePath()`;
- protocol golden vectors and CI guards required;
- physical USB and Bluetooth hardware qualification must pass before these rows become `LOCKED ✅`.

### P2 — Device preset management — CODE COMPLETE, CI/HARDWARE GATE PENDING

- isolated preset protocol golden vectors;
- transactional Recall + authoritative full resync;
- Use Init Volume ACK transaction;
- fresh-device-RAM single-slot permanent Save;
- raw slot-image Mass Upload engine with native chain/order;
- System UI wired only for operations that have a safe source now (Recall, Use Init, Save);
- PC preset import/upload UI remains disabled until P3/P4.

### P3 — Bit-perfect `.k500` engine

Parser/serializer/patcher with unknown-byte preservation and byte-identical no-op round trips. P3 will also become the raw slot-image source consumed by the already-built P2 Mass Upload engine.

### P4+ — Preset library and public hardening

Offline workflow, preset library, disconnect/reconnect recovery, diagnostics, public QA, then v1.0.

## Merge rule

A PR that makes the matrix look more complete but breaks any P0/P1 invariant is a regression and must not merge. Progress is measured only by capabilities that become safer and more complete while all prior `LOCKED ✅` rows remain green.
