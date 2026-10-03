# Changelog

All notable SonKuPik K500 changes are documented here. Hardware-facing statements are intentionally scoped: software/CI completion and physical hardware qualification are not treated as interchangeable evidence.


## Unreleased — Smart Installer public distribution

### Changed

- Removed Portable ZIP from the v1.1 RC/stable publication contract instead of adding another single-file portable wrapper.
- The machine-wide Setup is the recommended public **Smart Installer**.
- The per-user Setup remains a scope-matched updater backend so **Update tanpa Admin** continues to work without elevation after migration.
- RC/stable manifests and acceptance provenance now require exactly the machine and per-user installer identities/hashes.
- Historical v1.0.x portable assets remain immutable release history.

### Validation

- Stable manifest parsing already accepts the two-installer artifact model and regression coverage explicitly exercises both machine and per-user scope selection.
- RC3 operator testing confirmed correct preset presentation, Program Files migration/UAC behavior, no-admin LocalAppData update, and foreground relaunch; automated migration sentinels cover Documents presets, QSettings, official cache, and uninstall-registration cleanup.

## Unreleased — Synchronize official preset filenames

### Changed

- Renamed all ten official `.k500` repository files so the physical filename matches the finalized internal preset name, using the stable `NN_PRESET_NAME.k500` convention.
- Updated packaging, preset-file bridge, CI/release guards, updater references, and current-state documentation to use the synchronized filenames.
- Binary preset contents are unchanged by this path-only rename; blob identities remain the same.



## Unreleased — Preset 02 display rename

### Changed

- Renamed official Slot 02 internal preset name from `BCAST HIFI V2` to `MC HOST RADIO`.
- Audio parameters, EQ, dynamics, Reverb/Echo, routing, and donor sonic behavior are unchanged; only the visible/internal name field and additive checksum changed.

## Unreleased — Final official preset library refresh

### Changed

- Finalized the official donor-based preset library across Slots 01 and 03–10; Slot 02 remains intentionally dry/controlled for Broadcast use.
- Mode 01: `KONSER NYANYI` — concert/Air-Focus universal karaoke.
- Mode 03: `KAR DANGDUT` — refreshed vocal FX while preserving the proven Hi-Fi Core V3 Music/Sub foundation.
- Mode 04: `POP ROCK BALLAD` — finalized pop-rock/slow-rock balance with stronger singer control.
- Mode 05: `POP KENANGAN V2` — warm romantic 70s/80s slow-pop.
- Mode 06: `SHOLAWAT SYAHDU` — fresh, soft, slow-tempo sholawat ambience.
- Mode 07: `JAZZ LOUNGE` — intimate lounge vocal with restrained echo.
- Mode 08: `BLUES CLUB` — warm vintage blues with a single slap-style repeat.
- Mode 09: `ACOUSTIC NATURAL` — natural/organic vocal with unobtrusive ambience.
- Mode 10: `REGGAE DUB` — bass/groove-led voicing with intentional rhythmic echo.
- Updated the current Mode 01 release/golden guards to `KONSER NYANYI` while retaining the native `CONCERT HIFI V4` donor as rollback/provenance evidence.

### Validation

- All refreshed files remain exact 1144-byte K500 containers with valid additive checksums and hardware-safe names.
- Genre-specific Mic/Music/Main/Sub donor voicing is preserved; the refresh targets proven FX/timing surfaces rather than normalizing all modes to one curve.

### Fixed

- Official preset lists now display the embedded name from the exact validated bytes actually selected, so cache/catalog identity differences never masquerade under a static label; freshness direction is not claimed without provenance.
- The final official catalog names/descriptions are synchronized with the device-visible identities for Slots 01–10.
- v1.1 stable promotion derives the Mode 01 manifest SHA-256 from the exact accepted RC commit rather than a source-era hard-coded donor hash.

## 1.1.0 candidate — Windows updater lifecycle P1–P4

The source version is 1.1.0 for release-candidate qualification. Public stable remains v1.0.3 until an exact RC is accepted and the fail-closed updater acceptance token is promoted in a separate reviewed change.

### Added

- Standalone Win32 update coordinator that waits for K500 to exit safely, re-verifies Setup SHA-256, checks installer exit status, performs a hardware-free post-install health check, and relaunches the application.
- Separate machine-wide and per-user Setup packages with scope-matched update routing. Per-user updates run without UAC; machine-wide updates retain normal Windows elevation.
- Explicit **Update tanpa Admin** migration from a registered Program Files installation to the current user's LocalAppData Programs directory.
- Complete executable-tree + uninstall-registration recovery snapshots for installed updates, with rollback after installer failure or failed new-version health check.
- Resumable `.part` downloads with strict HTTP Range/Content-Range validation, final byte-size checks, and SHA-256 verification.
- Explicit stale uninstall-registration repair that refuses to remove registrations while their application or uninstaller still exists.
- Windows qualification fixtures for per-user update, destructive installer rollback, failed-health rollback, machine-to-user migration, and persistence of user presets/QSettings/cache.
- Manual-only `v1.1.0-rc.N` packaging with exact-main CI gating, dual scope-matched installers, checksums, and candidate manifest.
- An explicit process-local `--update-candidate=v1.1.0-rc.N` QA channel that lets an installed candidate consume exactly one immutable prerelease without changing the normal stable channel.
- A repository-controlled `UPDATER_V1_1_ACCEPTANCE` provenance gate that records accepted RC tag, source commit, and SHA-256 for the machine and per-user installers.
- Byte-identical v1.1 stable promotion: accepted RC binaries are revalidated and republished under stable names; they are not rebuilt after acceptance.

### Safety

- Migration installs and validates the per-user copy before requesting one UAC elevation to remove the old machine-wide installation.
- Ambiguous installer/uninstaller timeouts are not raced with destructive cleanup; recovery data is preserved.
- Retry after a failed no-admin migration preserves the user's per-user package choice instead of falling back to machine-wide Setup.
- No code-signing requirement, UAC bypass, or Windows-security weakening is introduced.

## 1.0.3 — Device-validation closure and installer polish

Public stable release from commit `7d6ded580e15b652db97edf0700283e3fa931996`, preserving the Windows 10/11 x64 + USB HID support scope.

### Changed

- Integrated capture-backed runtime/device-truth work from the v1.0.3 validation line: Use Init Volume and mute/playback truth, non-disruptive live edits, Mic FBX 0..4, Music HP/LP filter-type readback, corrected EQ-bypass polarity, and the native Music Max ceiling/clamp behavior.
- Kept unproven mappings evidence-gated/read-only rather than guessing protocol bytes.
- Fixed the Inno Setup wizard header logo clipping and centralized installer brand-asset generation.

### Validation

- The v1.0.3 device-validation line advanced through RC1/RC2 with exact-build CI and physical-capture-backed fixes before the final public stable package.
- Public v1.0.3 remains the current latest non-prerelease release while v1.1 updater promotion is qualified separately.

## 1.0.2 — Smart Windows lifecycle

Stable maintenance release preserving the existing v1.0 hardware-qualified scope while improving installation, local preset storage, update delivery, and update-integrity regression coverage.

### Added

- In-app stable update discovery with a SonKuPik-styled update card and release notes.
- Fail-closed update verification against `release-manifest.json`, `SHA256SUMS.txt`, artifact filename, byte size, Windows target, stable eligibility, and locally computed SHA-256.
- Windows `runas`/UAC handoff only after the downloaded Setup package passes all integrity checks.
- Deterministic updater metadata self-test using the production manifest/checksum parsers without network access or K500 hardware.
- Update safety interlock that blocks application replacement while Save, Upload, or Mass Upload transactions are active.

### Changed

- Recommended installation now uses the canonical machine location `C:\Program Files\SonKuPik K500`.
- The default user-owned Local preset library is initialized at `Documents\SonKuPik K500\Presets` and remains separate from application/runtime files.
- Official preset cache and update staging remain internal under Qt `AppLocalDataLocation`.
- Existing custom Local preset-folder preferences are preserved instead of being overwritten when a removable/network path is temporarily unavailable.
- Upgrade from the earlier per-user installation migrates the legacy LocalAppData application install before writing the Program Files copy, while leaving personal presets and preferences intact.
- Automatic update discovery is delayed until after startup and throttled; **Nanti** deliberately enables a fresh check on the next application launch.

### Release integrity

- v1.0.2 continues the v1.0 Windows x64 + USB HID hardware-qualified support line; Bluetooth SPP remains experimental.
- Stable packaging continues to produce a standard Inno Setup installer plus ordinary portable ZIP.
- Release artifacts remain unsigned open-source builds and continue to publish `SHA256SUMS.txt` plus `release-manifest.json`.
- The stable publisher now records updater metadata integrity in its regression suite before publishing a GitHub release.

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

- Historical v1.0 Mode 01 donor recovery used the exact physical-K500 native `CONCERT HIFI V4` donor instead of the earlier reconstructed file; the current file is `resources/presets/01_KONSER_NYANYI.k500`.
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
