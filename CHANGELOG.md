# Changelog

All notable SonKuPik K500 changes are documented here. Hardware-facing statements are intentionally scoped: software/CI completion and physical hardware qualification are not treated as interchangeable evidence.

## 1.0.0 — Public stable

First public stable release. Hardware-qualified support scope: **Windows 10/11 x64 + K500 over USB HID**. Bluetooth SPP remains implemented but experimental until independently accepted.

### Added

- Unified PC preset collection combining **SONKUPIK** official presets and **LOCAL** user presets.
- Background Official Preset sync from the repository with strict `.k500` validation and last-known-good local cache.
- Bundled official preset fallback so the Mass Upload collection remains usable offline.
- Manual Sync/Refresh workflow for official and local preset sources.
- Stable Windows release pipeline producing a standard Inno Setup installer and ordinary portable ZIP.
- Runtime section-navigation stress test covering repeated 10-band ↔ 5-band ↔ 7-band workspace transitions.
- Branded Windows installer icon and wizard artwork from the authoritative SonKuPik K500 identity.

### Changed

- Mode 01 `resources/presets/01_ALL_GENRE.k500` now uses the exact physical-K500 native `CONCERT HIFI V4` donor instead of the earlier reconstructed file.
- Mode 01 golden SHA-256 is now `9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74`.
- Mass Upload source selection now resolves both official cached/bundled files and user-local files into the same validated transfer list.
- Reverb/Mic/Main section navigation now keeps fixed EQ-page/model lifetimes rather than hot-swapping different band-count models through one graph instance.
- Windows distribution moved away from the retired self-extracting portable executable to a normal ZIP to reduce unnecessary antivirus heuristics.
- Public identity standardized on **SonKuPik K500** across application title, installer, release artifacts, support metadata, repository, and web assets.

### Fixed

- Fixed a runtime crash/heap-corruption class triggered by section changes such as Mic ↔ Reverb.
- Fixed Mode 01 Mass Upload behavior by restoring the exact native donor bytes; the previous reconstructed preset could leave the music path unusable on hardware.
- Fixed System/Mass Upload usability when no local preset folder had been selected.
- Fixed installer branding so Setup, wizard visuals, shortcuts, and application identity use the K500 brand assets consistently.

### Safety and release integrity

- Device readback remains authoritative after connect/recall.
- QML still never owns raw device I/O.
- Official remote presets must pass exact K500 size/checksum validation before cache promotion.
- Local user presets are never overwritten by official sync.
- Permanent Upload/Mass Upload remains USB/store-gated and fail-closed.
- Public stable artifacts include `SHA256SUMS.txt` and `release-manifest.json` with the exact release commit and build provenance.
- Persistent LCD/Equipment Mode rename remains read-only until a donor-verified write transaction exists.

## 0.6.1 — Release candidate

- Introduced the unified Official + Local preset library and GitHub-backed official preset cache/update path.
- Added installer branding improvements and normal portable ZIP packaging.
- Added crash-proof section-navigation architecture and runtime stress testing.
- Replaced Mode 01 with the exact native `CONCERT HIFI V4` donor after physical hardware testing.

## 0.5.0 — Release-candidate hardening

### Added

- Bounded JSON Support Report export from the top toolbar.
- Runtime version metadata sourced from CMake.
- GPL-3.0-or-later project licensing and public contribution/security policy.
- Machine-readable Windows `release-manifest.json`.
- Hardware-acceptance status in packaging.
- P4/P4.2 physical acceptance and failure-injection runbook.
- Release-readiness CI gate.

### Safety

- Support reports exclude active-memory contents, preset bytes, and local preset paths.
- Stable promotion was gated on physical K500 validation instead of CI-only evidence.

## P4.2 — Deterministic multi-file Mass Upload

- Validated 1–10 `.k500` files before any device write.
- Deterministic selected-slot mapping.
- Whole-batch rejection on invalid size/checksum/conversion.
- Existing native descending slot order and Store chain reused unchanged.

## P4 — PC preset permanent Upload

- Selected-slot `.k500` permanent upload over USB HID.
- Verified `0x0290` slot image produced by the P3 codec.
- No pre-Store device readback that could silently replace the selected PC preset.

## P3.4 — Controlled edit persistence

- Canonical StudioEngine edits persisted into `.k500` through explicit byte whitelists.
- Checksum refresh, unknown-byte preservation, and raw PEQ alias preservation.
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
- Native Mass Upload transaction engine with verified ACK chain and descending slot order.

## P1 — Donor-verified LIVE surface

- Top Mic, Top Effect, output blocks, crossovers, Mic EQ Link, and donor-verified PEQ routing.
- Unknown output-block bytes preserved from device truth.

## P0 — Regression fortress

- Native Windows Qt/QML architecture.
- USB HID and Bluetooth SPP transports.
- Full 939-byte device hydration before LIVE.
- Zero echo-write hydration invariant.
- Protocol/parser/engine/UI regression guards.
