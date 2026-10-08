# SonKuPik K500 Documentation

This directory is the authoritative documentation set for the SonKuPik K500 v1 stable line.

## Start by role

### Users

- [User Guide](USER_GUIDE.md) — install, connect, edit, Preview, Save As, Upload, Mass Upload, preset Sync, diagnostics.
- [Release Model](RELEASES.md) — stable/prerelease policy, package verification, official preset updates.
- [v1.1.1 Patch RC Acceptance](V1_1_1_UPDATER_RC_ACCEPTANCE.md) — accepted RC1 provenance and byte-identical v1.1.1 stable-promotion record.
- [v1.1.0 Updater RC Acceptance](V1_1_UPDATER_RC_ACCEPTANCE.md) — historical fail-closed updater qualification and stable-promotion record.
- [Windows Distribution Security](WINDOWS_DISTRIBUTION_SECURITY.md) — unsigned binaries, hashes, antivirus/reputation guidance.

### Contributors

- [Architecture](ARCHITECTURE.md) — state authority, class boundaries, transaction flow, failure model, preset sync, crash-resistant UI lifetime.
- [Capability & Protocol Parity Matrix](PORTING_PARITY_MATRIX.md) — what is stable, experimental, read-only, or unsupported.
- [Protocol Golden Vectors](PROTOCOL_GOLDEN_VECTORS.md) — packet-level regression contracts.
- [Hardware Acceptance Checklist](HARDWARE_ACCEPTANCE_CHECKLIST.md) — physical-device validation and regression procedure.
- [v1.0 Stable Release Qualification](P5_RELEASE_READINESS.md) — exact hardware-qualified baseline and build gates.

### Preset engineers / AI agents

- [Proven Sonic Baseline](K500_PROVEN_SONIC_BASELINE.md) — current hardware-proven listening references and Mode 01 authority.
- [AI Preset Engineering Playbook](K500_AI_PRESET_ENGINEERING_PLAYBOOK.md) — research workflow and psychoacoustic strategy.
- [Bit-perfect AI Preset Guide](K500_BIT_PERFECT_AI_PRESET_GUIDE.md) — binary map, preservation rules, simulation boundary.
- [Preset Name Limit](K500_PRESET_NAME_LIMIT.md) — 16-character hardware-safe name rule.
- [P3 Codec Contract](P3_K500_CODEC.md) — application codec and device-slot conversion guarantees.
- [P3.2 File Bridge](P3_2_FILE_BRIDGE.md) — file/backend state boundary.
- [P3.3 Safe File UI](P3_3_PRESET_FILE_UI.md) — Official + Local library, Preview, Upload, Mass Upload.

## Stable v1 snapshot

| Contract | Current truth |
|---|---|
| Hardware-qualified baseline | `v1.0.0` |
| Public stable | `v1.1.1` via [GitHub latest stable](https://github.com/masarray/k500/releases/latest) |
| Stable source line | `v1.1.1`, promoted byte-identically from accepted `v1.1.1-rc.1` at `df7a6dd3504f4c7ffbf19b12d9e0c2e1b9243b13` |
| Product download router | [Smart Installer](https://sonkupik-k500.pages.dev/download/windows) |
| Qualified platform | Windows 10/11 x64 |
| Qualified transport | USB HID |
| Bluetooth SPP | Implemented, experimental |
| Active-memory truth | 939 bytes / `0x03AB` |
| `.k500` file | 1144 bytes / `0x0478` |
| Native slot image | 656 bytes / `0x0290` |
| Official Mode 01 | `KONSER NYANYI` (native-donor lineage) |
| Mode 01 SHA-256 | `66ba788daadf56212e4de6673a702404cfc915dc934e98fe3b479ee972f978a1` |
| Official Mode 02 | `MC HOST RADIO` — SHA-256 `adf3c868cfa471b4b0975bb58ffb4e8c05c071a7ed0896fc0a89b72933a88fa7` |
| Windows distribution | Smart Installer + scope-matched per-user updater package |
| Code signing | unsigned open-source |

The public download **routes** remain version-agnostic and resolve GitHub's canonical latest non-prerelease release. The landing buttons show the resolved stable tag (currently `v1.1.1`) and refresh that visible version from `/api/release`, so visitors can immediately see which installer is being offered without hard-coding future routes.

## Documentation principles

Documentation follows the same evidence hierarchy as the application:

1. current exact source and golden tests;
2. real K500 donor/capture evidence;
3. physical hardware acceptance for hardware claims;
4. simulation/modeling as comparative engineering evidence only.

If two documents conflict, the narrower evidence-backed technical contract wins. For current release/support status, prefer `README.md`, this index, `RELEASES.md`, the parity matrix, and the release manifest. For protocol bytes, prefer `PROTOCOL_GOLDEN_VECTORS.md` and tests. For preset bytes, prefer the exact repository file and its validated hash.

## Historical milestone names

Some documents retain P0–P5 / P3.2 / P4.2 labels because those milestones identify regression contracts and are useful when tracing implementation history. They should not be interpreted as “unfinished” merely because the historical label remains. The v1 parity matrix states the current support status.
