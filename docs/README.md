# SonKuPik K500 Documentation

This directory is the authoritative documentation set for the SonKuPik K500 v1 stable line.

## Start by role

### Users

- [User Guide](USER_GUIDE.md) — install, connect, edit, Preview, Save As, Upload, Mass Upload, preset Sync, diagnostics.
- [Release Model](RELEASES.md) — stable/prerelease policy, package verification, official preset updates.
- [Windows Distribution Security](WINDOWS_DISTRIBUTION_SECURITY.md) — unsigned binaries, hashes, antivirus/reputation guidance.

### Contributors

- [Architecture](ARCHITECTURE.md) — state authority, class boundaries, transaction flow, failure model, preset sync, crash-resistant UI lifetime.
- [Capability & Protocol Parity Matrix](PORTING_PARITY_MATRIX.md) — what is stable, experimental, read-only, or unsupported.
- [Protocol Golden Vectors](PROTOCOL_GOLDEN_VECTORS.md) — packet-level regression contracts.
- [Hardware Acceptance Checklist](HARDWARE_ACCEPTANCE_CHECKLIST.md) — physical-device validation and regression procedure.
- [v1.0 Stable Release Qualification](P5_RELEASE_READINESS.md) — exact stable release record and build gates.

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
| Public stable | `v1.0.0` |
| Qualified platform | Windows 10/11 x64 |
| Qualified transport | USB HID |
| Bluetooth SPP | Implemented, experimental |
| Active-memory truth | 939 bytes / `0x03AB` |
| `.k500` file | 1144 bytes / `0x0478` |
| Native slot image | 656 bytes / `0x0290` |
| Official Mode 01 | Native `CONCERT HIFI V4` |
| Mode 01 SHA-256 | `9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74` |
| Windows distribution | Inno Setup + normal portable ZIP |
| Code signing | unsigned open-source |

## Documentation principles

Documentation follows the same evidence hierarchy as the application:

1. current exact source and golden tests;
2. real K500 donor/capture evidence;
3. physical hardware acceptance for hardware claims;
4. simulation/modeling as comparative engineering evidence only.

If two documents conflict, the narrower evidence-backed technical contract wins. For current release/support status, prefer `README.md`, this index, `RELEASES.md`, the parity matrix, and the release manifest. For protocol bytes, prefer `PROTOCOL_GOLDEN_VECTORS.md` and tests. For preset bytes, prefer the exact repository file and its validated hash.

## Historical milestone names

Some documents retain P0–P5 / P3.2 / P4.2 labels because those milestones identify regression contracts and are useful when tracing implementation history. They should not be interpreted as “unfinished” merely because the historical label remains. The v1 parity matrix states the current support status.
