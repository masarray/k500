# SONKUPIK STUDIO Qt Porting Parity Matrix

> **P0 regression contract.** `main` must remain releasable. A capability may move to `LOCKED ✅` only after its donor behavior/protocol is identified, automated tests pass, and hardware acceptance is recorded where the feature touches a real K500.

## Status legend

- `LOCKED ✅` — implemented and protected by automated regression tests; hardware-facing behavior is already part of the accepted baseline.
- `IMPLEMENTED ⚠️` — code exists, but parity/hardware acceptance is incomplete.
- `READ ONLY 🟦` — current K500 value is decoded/hydrated, but native Qt does not yet have a verified write path.
- `NOT PORTED ❌` — donor capability still needs a native Qt implementation.
- `DONOR ONLY 🟨` — proven in `masarray/ktv-studio-mixer-pro`; donor is the specification until native parity is complete.

## P0 non-negotiable invariants

These are the golden baseline and must not regress in any later phase:

1. QML never talks directly to device I/O.
2. Native path stays `QML -> StudioEngine -> K500Controller -> K500DeviceManager -> K500WinIo`.
3. On connect, the device is source of truth.
4. Connect order remains heartbeat -> handshake -> full `0x03AB` (939-byte) active-memory readback -> hydrate editor while LIVE is OFF -> LIVE ON.
5. Hydration must emit **zero** `stateEdited` writes.
6. Bluetooth CMD `0x40` read mode remains `0x63`; USB HID mode remains `0x00`.
7. USB remains VID/PID `10C4:0321`, report ID 0, 64-byte HID reports.
8. Verified protocol bytes are never changed without a new golden vector and an explicit donor/capture justification.
9. Controls without verified native writes must remain non-destructive (`unsupportedPath`) rather than guessing bytes.
10. PEQ/fader/caption/Plus Jakarta Sans regression guards remain mandatory.

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
| Mic crossover live write | DONOR ONLY 🟨 | READ ONLY 🟦 | controller routing + tests + hardware |
| Main crossover live write | DONOR ONLY 🟨 | READ ONLY 🟦 | controller routing + tests + hardware |
| Surround crossover live write | DONOR ONLY 🟨 | READ ONLY 🟦 | controller routing + tests + hardware |
| Center crossover live write | DONOR ONLY 🟨 | READ ONLY 🟦 | controller routing + tests + hardware |
| Sub crossover live write | DONOR ONLY 🟨 | READ ONLY 🟦 | controller routing + tests + hardware |
| Reverb/Echo crossover live write | DONOR ONLY 🟨 | READ ONLY 🟦 | controller routing + tests + hardware |
| Top Mic `CMD 0x05` | DONOR ONLY 🟨 | NOT PORTED ❌ | exact donor vector + mirrored scalar safety + hardware |
| Top Effect `CMD 0x09` | DONOR ONLY 🟨 | NOT PORTED ❌ | exact donor vector + mirrored scalar safety + hardware |
| Mic EQ Link | DONOR ONLY 🟨 | NOT PORTED ❌ | exact donor vector + UI mirror + hardware |
| Main output block | DONOR ONLY 🟨 | READ ONLY 🟦 | exact donor vector + raw-block preservation + hardware |
| Surround output block | DONOR ONLY 🟨 | READ ONLY 🟦 | exact donor vector + raw-block preservation + hardware |
| Center output block | DONOR ONLY 🟨 | READ ONLY 🟦 | exact donor vector + raw-block preservation + hardware |
| Sub output block | DONOR ONLY 🟨 | READ ONLY 🟦 | exact donor vector + raw-block preservation + hardware |
| Recall Equipment Mode 1-10 | DONOR ONLY 🟨 | NOT PORTED ❌ | transactional recall + full resync + hardware |
| Use Init Volume | DONOR ONLY 🟨 | NOT PORTED ❌ | exact donor vector + hardware |
| Permanent Store Begin/Chunk/Commit | DONOR ONLY 🟨 | NOT PORTED ❌ | transactional store + verification + power-cycle test |
| Mass Upload | DONOR ONLY 🟨 | NOT PORTED ❌ | single-slot store must be LOCKED first |
| `.k500` parse | DONOR ONLY 🟨 | NOT PORTED ❌ | fixture corpus |
| `.k500` bit-perfect no-op serialize | DONOR ONLY 🟨 | NOT PORTED ❌ | byte-identical round trip |
| `.k500` import/export | DONOR ONLY 🟨 | NOT PORTED ❌ | codec + checksum + whitelist tests |
| Preset library | DONOR ONLY 🟨 | NOT PORTED ❌ | codec must be LOCKED first |

## Phase gates

### P0 — Regression Fortress

- parity matrix committed;
- current protocol vectors frozen;
- full-memory hydration test expanded;
- CI checks P0 invariants;
- hardware acceptance checklist committed.

### P1 — Full LIVE parity

Top Mic, Top Effect, all output blocks, all crossover routing, Mic EQ Link, then USB/BT hardware qualification.

### P2 — Device preset management

Recall, Use Init Volume, permanent Store, then Mass Upload. All operations transactional and followed by resync/verification.

### P3 — Bit-perfect `.k500` engine

Parser/serializer/patcher with unknown-byte preservation and byte-identical no-op round trips.

### P4+ — Preset library and public hardening

Offline workflow, preset library, disconnect/reconnect recovery, diagnostics, public QA, then v1.0.

## Merge rule

A PR that makes the matrix look more complete but breaks any P0 invariant is a regression and must not merge. Progress is measured only by capabilities that become safer and more complete while all prior `LOCKED ✅` rows remain green.
