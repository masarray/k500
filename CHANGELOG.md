# Changelog

All notable native Qt milestones are summarized here. Hardware-facing completion means code/CI completion unless physical acceptance is explicitly stated.

## 0.5.0 — Release candidate

### Added

- P5 bounded JSON Support Report export from the top toolbar.
- Runtime version metadata sourced from CMake.
- GPL-3.0-or-later project licensing and public contribution/security policy.
- Machine-readable Windows `release-manifest.json`.
- Explicit release-candidate / hardware-acceptance status in packaging.
- P4/P4.2 physical acceptance and failure-injection runbook.
- P5 release-readiness CI gate.

### Changed

- Public README and parity matrix now reflect P0–P4.2 software reality.
- Windows release packaging reruns the full preset regression suite before installer/portable creation.
- GitHub release publishing is forced to **prerelease** while physical K500 acceptance remains pending.

### Safety

- Support reports exclude active-memory contents, preset bytes and local preset paths.
- Stable `v1.0` is explicitly blocked by policy until USB/BT and permanent-storage acceptance is recorded on real hardware.

## P4.2 — Deterministic multi-file Mass Upload

- Validated 1–10 `.k500` files before any device write.
- Deterministic filename sorting and sequential selected-slot assignment.
- Whole-batch rejection on invalid size/checksum/conversion.
- Existing P2 native descending slot order and Store chain reused unchanged.

## P4 — PC preset permanent Upload

- Selected-slot `.k500` permanent upload over USB HID.
- Verified `0x0290` image from P3 working document.
- No pre-Store device readback that could silently replace the selected PC preset.

## P3.4 — Controlled edit persistence

- Canonical StudioEngine edits persisted into `.k500` through explicit byte whitelists.
- Checksum refresh, unknown-byte preservation and raw PEQ alias preservation.
- Atomic edited Save As.

## P3 / P3.2 / P3.3 — Bit-perfect `.k500` engine and UI

- Exact 1144-byte parser and checksum validation.
- Byte-identical no-edit round trip.
- Correct scalar split and compact-EQ conversion into 656-byte native device slot images.
- Offline preset open/preview/import/export bridge.

## P2 — Device preset transactions

- Recall with full authoritative 939-byte resync.
- Use Init Volume transaction.
- USB permanent current-device Save.
- Native Mass Upload transaction engine with `BE/BD/BC` ACK chain and descending slot order.

## P1 — Full donor-verified LIVE surface

- Top Mic, Top Effect, output blocks, crossovers, Mic EQ Link and donor-verified PEQ routing.
- Unknown output-block bytes preserved from device truth.

## P0 — Regression fortress

- Native Windows Qt/QML architecture.
- USB HID and Bluetooth SPP transports.
- Full 939-byte device hydration before LIVE.
- Zero echo-write hydration invariant.
- Protocol/parser/engine/UI regression guards.
