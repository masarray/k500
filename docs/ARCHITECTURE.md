# SonKuPik K500 Architecture

This document describes the v1 stable native architecture and the invariants future work must preserve.

## Design objectives

SonKuPik K500 controls real hardware, so architecture is organized around four priorities:

1. **hardware truth before LIVE writes**;
2. **one transport owner**;
3. **verified protocol only**;
4. **fail closed for uncertain permanent state**.

## Runtime layers

```text
QML UI
  │ user intent / display state
  ▼
StudioEngine
  │ canonical editor state + hydration boundary
  ▼
K500Controller
  │ verified LIVE command routing / coalescing
  ▼
K500DeviceManager
  │ connection, heartbeat, readback, diagnostics, write gateway
  ▼
K500WinIo
  │ Win32 USB HID / Bluetooth SPP
  ▼
K500
```

QML does not own a raw HID handle, COM port, or second protocol stack.

## Preset transaction path

Permanent preset operations deliberately use a sibling coordinator instead of pretending they are ordinary live fader edits:

```text
System UI
  ▼
K500PresetManager
  ▼
K500DeviceManager
  ▼
K500WinIo
  ▼
K500
```

`K500PresetManager` owns transaction sequencing, ACK/timeouts, Recall/readback coordination, and batch order, but still writes through `K500DeviceManager`.

## State authority model

### 1. Hardware State

Authoritative while connected. Includes:

- 939-byte active-memory readback;
- active Device slot from recall/C0 truth;
- hardware mode-name table;
- device-owned unknown bytes required for safe block writes.

### 2. Staged PC Preset

A selected Official or Local `.k500` file. Selection is non-destructive and does not become live hardware truth.

### 3. Offline Preview State

Created only after explicit Preview. The file can hydrate the editor for inspection/controlled editing, with edit tracking attached to the staged source document.

### 4. Mass Upload Staging

A reviewable PC-preset → Slot 01…10 mapping. It has no hardware effect until explicit execution after whole-batch validation.

These four states must never be collapsed into one ambiguous “current preset” concept.

## Connect sequence

The stable USB path is:

```text
transport open
  -> heartbeat
  -> handshake
  -> block reads covering 0x0000..0x03AA
  -> assemble 939-byte active memory
  -> hydrate StudioEngine while LIVE is OFF
  -> enable LIVE only after successful hydration
```

Hydration must emit zero replay writes. A malformed/partial readback is not promoted to LIVE.

## Safe live block writes

For commands containing multiple device fields, the application does not construct a complete block from generic UI defaults. It seeds the block from current K500 readback and patches only the donor-verified target fields.

This preserves neighboring/unknown bytes.

## Recall transaction

```text
LIVE OFF
 -> CMD 0x01 selected slot
 -> settle
 -> CMD 0x3F
 -> require RSP 0xC0
 -> full 939-byte readback
 -> hardware hydration
 -> LIVE ON
```

If the transaction becomes uncertain, LIVE does not resume from guessed state.

## Permanent Store

The permanent device slot image is exactly 656 bytes (`0x0290`). Store uses:

```text
CMD 0x41 begin
CMD 0x42 x11 chunks
CMD 0x43 commit
```

Single Store and Mass Upload have different proven begin-ACK behavior; see `PROTOCOL_GOLDEN_VECTORS.md` rather than generalizing one transaction into the other.

## `.k500` codec boundary

A `.k500` file is exactly 1144 bytes (`0x0478`). The permanent 656-byte slot image is **not** the first 656 bytes of that file.

The codec performs the verified scalar split, EQ compaction, tail mapping, and name projection while preserving source bytes for no-op/edit workflows.

Controlled file mutations use:

```text
original donor bytes
 -> explicit semantic change
 -> offset whitelist
 -> checksum refresh
 -> changed-byte audit
 -> atomic Save As
```

## Official preset synchronization

Official preset delivery is independent from the application release binary.

```text
Bundled official presets -----------------------+
                                                 |
GitHub official source -> download -> validate -> last-known-good cache
                                                 |
Local user folder -------------------------------+-> unified PC collection
```

Key boundaries:

- bundled presets guarantee offline availability;
- remote official files must validate before cache promotion;
- a failed sync leaves the previous valid cache/bundled file available;
- Local user files are never overwritten by official sync;
- Official and Local entries resolve to validated local/readable bytes before Preview/Upload.

## Mass Upload

UI staging is intentionally ascending and human-readable:

```text
row 1 -> Slot 01
...
row 10 -> Slot 10
```

Hardware execution follows the proven native sequence:

```text
highest selected slot -> ... -> Slot 01
```

A full bank executes **10 → 1**. The next Store carries the native inter-slot chain. After final Slot 01 commit, the application recalls Slot 01 and refreshes all 939 active-memory bytes before returning to LIVE.

Whole-batch validation occurs before the first permanent write.

## Crash-resistant QML section architecture

Processor sections do not share one graph instance that dynamically changes between incompatible band counts.

The stable baseline gives Mic A, Mic B, Reverb, Echo, Main, Surround, Center, and Sub fixed graph/model lifetimes. Navigation changes the active page/visibility, not graph model identity.

This avoids transitional Canvas/Repeater/Inspector state that previously caused heap corruption when moving between 10-band, 5-band, and 7-band sections.

Runtime CI repeatedly crosses these pages to protect this rule.

## Diagnostics boundary

Support Report JSON contains bounded operational diagnostics, not full user/device state. It redacts/excludes:

- complete active-memory payload;
- preset bytes;
- Local preset paths.

Diagnostics should help reproduce protocol issues without turning a bug report into an unnecessary device-data dump.

## Failure model

The application prefers an explicit offline/error state to pretending a destructive operation succeeded.

Examples:

- stale heartbeat -> LIVE OFF;
- short readback -> reject connection state;
- uncertain Recall -> require reconnect;
- interrupted Store/Upload -> do not claim success;
- invalid batch member -> abort before any Mass Upload write;
- invalid remote preset -> retain last-known-good official source;
- unsupported hardware field -> no guessed frame.

## Current intentional protocol gaps

Persistent hardware LCD/Equipment Mode Name rename is not implemented because no donor-verified write transaction exists. Hardware names can be read, but the write path remains disabled.

Detailed hardware writes for other fields remain read-only wherever the parity matrix says protocol evidence is missing.

## Stable change rule

A refactor may change class organization or UI composition only if these invariants remain provable. New functionality should add evidence/tests rather than removing an older regression contract to make the new design fit.
