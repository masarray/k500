# P3.2 — `.k500` File Bridge + Real Corpus

Status: **LOCKED ✅ — integrated in the v1.0 stable application**

P3.2 connects the byte-preserving P3 codec to the Qt application without weakening the device-control baseline.

## Current role

`K500PresetFileBridge` is the application-facing file boundary for validated `.k500` documents. It supports:

- local file import;
- exact `0x0478` / 1144-byte validation;
- additive checksum validation;
- source-byte-preserving document ownership;
- `.k500 -> 0x0290` native slot conversion through the P3 codec;
- explicit offline Preview hydration;
- controlled edit persistence through the P3.4 whitelist mapper;
- atomic `QSaveFile` Save As;
- validated source paths for single Upload and Mass Upload.

A real donor fixture remains in the test corpus and protects name/checksum, no-op byte identity, split scalar mapping, compact PEQ conversion, tail mapping, and 16-byte hardware name projection.

## State-authority boundary

The file bridge never becomes hardware truth merely because a file is selected.

```text
Hardware editor truth
K500 -> full 939-byte readback -> StudioEngine -> QML

PC staging truth
System QML -> K500PresetManager / K500PresetFileBridge -> validated .k500 document
```

Selecting a PC preset is staging-only. It does not change K500 audio, active slot, device mode names, faders, or PEQ.

Only an explicit **Preview** hydrates the editor from the staged file while offline/preview semantics are active. Connecting a K500 restores hardware readback as authority and disables PC-file edit tracking for LIVE hardware edits.

## Controlled persistence

The v1 path is no longer a source-byte-only Save As prototype. Verified PEQ/fader edits may be persisted through explicit byte whitelists.

The invariant remains:

```text
source bytes
  -> permitted semantic edit
  -> whitelist patch
  -> checksum refresh
  -> changed-byte audit
  -> atomic Save As
```

Unknown/reserved bytes are not normalized.

## Upload integration

The file bridge supplies validated 656-byte slot images to the proven permanent Store path. It does **not** write raw hardware frames itself.

Single Upload:

```text
validated staged .k500
  -> verified slot-image conversion
  -> K500PresetManager
  -> Store transaction
  -> destination Recall
  -> full 939-byte readback
  -> LIVE
```

Mass Upload validates every selected entry before the first hardware write and uses the same transaction coordinator.

## Official + Local sources

The v1 preset library can resolve staged files from two sources:

- **SONKUPIK** official bundled/cache presets;
- **LOCAL** user-owned files.

Both reach the same validation/codec path before Preview, Upload, or Mass Upload. The source label changes provenance and update behavior, not binary safety rules.

Official remote sync never writes into the Local user folder.

## Stable guarantees

- no-op file operations remain byte-identical;
- invalid size/checksum is rejected before Preview/Upload;
- file selection remains non-destructive;
- Preview is explicit;
- permanent Upload is explicit and USB/store-gated;
- device state is re-read after permanent activation;
- file/backend code never bypasses `K500DeviceManager` for raw I/O.
