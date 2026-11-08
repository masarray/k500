# SonKuPik K500 Documentation

This directory is the authoritative documentation set for the SonKuPik K500 v1 stable line.

## Start by role

### Users

- [User Guide](USER_GUIDE.md) — install, connect, edit, Preview, Save As, Upload, Mass Upload, preset Sync, diagnostics.
- [Release Model](RELEASES.md) — stable/prerelease policy, package verification, official preset updates.
- [v1.1.6 RC Acceptance](V1_1_6_UPDATER_RC_ACCEPTANCE.md) — approved current stable, exact RC source/installer SHA-256 and field evidence boundary.
- [v1.1.5 RC Acceptance](V1_1_5_UPDATER_RC_ACCEPTANCE.md) — historical accepted RC.1 and prior stable provenance.
- [v1.1.4 RC Acceptance](V1_1_4_UPDATER_RC_ACCEPTANCE.md) — historical v1.1.4 qualification.
- [v1.1.3 RC Acceptance](V1_1_3_UPDATER_RC_ACCEPTANCE.md) — accepted RC.6 provenance and byte-identical v1.1.3 stable publication.
- [v1.1.2 RC Acceptance](V1_1_2_UPDATER_RC_ACCEPTANCE.md) — historical RC.2 and v1.1.2 stable publication.
- [v1.1.2 Candidate Scope](V1_1_2_CANDIDATE_SCOPE.md) — UAUDIO 11–20, multiselect, bypass warning and title.
- [v1.1.1 Patch RC Acceptance](V1_1_1_UPDATER_RC_ACCEPTANCE.md) — accepted RC1 provenance and byte-identical v1.1.1 stable-promotion record.
- [v1.1.0 Updater RC Acceptance](V1_1_UPDATER_RC_ACCEPTANCE.md) — historical fail-closed updater qualification and stable-promotion record.
- [Windows Distribution Security](WINDOWS_DISTRIBUTION_SECURITY.md) — unsigned binaries, hashes, antivirus/reputation guidance.

### Contributors

- [Architecture](ARCHITECTURE.md) — state authority, class boundaries, transaction flow, failure model, preset sync, crash-resistant UI lifetime.
- [Capability & Protocol Parity Matrix](PORTING_PARITY_MATRIX.md) — what is stable, experimental, read-only, or unsupported.
- [Protocol Golden Vectors](PROTOCOL_GOLDEN_VECTORS.md) — packet-level regression contracts.
- [Hardware Acceptance Checklist](HARDWARE_ACCEPTANCE_CHECKLIST.md) — physical-device validation and regression procedure.
- [v1.0 Stable Release Qualification](P5_RELEASE_READINESS.md) — historical hardware-qualified baseline and build gates.
- [Smart CI comment workflow](SMART_CI_COMMENT_WORKFLOW.md) — owner-triggered targeted native suites, fast PR/main validation.

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
| Public stable | `v1.1.6` via [GitHub latest stable](https://github.com/masarray/k500/releases/latest) |
| Stable provenance | Owner-accepted `v1.1.6-rc.1` at source `39f9cc926a53900e62f439c44f61878880c6d1d2`; [comment-triggered promotion](https://github.com/masarray/k500/actions/runs/42454469526) |
| Product download router | [Verified ZIP](https://sonkupik-k500.pages.dev/download/windows-preferred) · [Direct EXE](https://sonkupik-k500.pages.dev/download/windows) |
| Qualified platform | Windows 10/11 x64 |
| Qualified transport | USB HID |
| Bluetooth SPP | Implemented, experimental |
| Active-memory truth | 939 bytes / `0x03AB` |
| `.k500` file | 1144 bytes / `0x0478` |
| Native slot image | 656 bytes / `0x0290` |
| Official PC bank 01–10 | `FOCUS ARTIST` / `FOCUS MC RADIO` / genre FOCUS BT R6; owner-approved |
| Official PC bank 11–51 | Archived GEN5, GEN3 and historical presets; D2R GOLDEN #41 stays protected |
| Physical device slots | Exactly 10; PC catalog offers 51 independently selectable presets |
| Windows distribution | Per-User Setup, verified ZIP/EXE and versioned compatibility asset for old updater clients |
| Code signing | unsigned open-source |

The public download **routes** remain version-agnostic and resolve GitHub's canonical latest non-prerelease release. The landing buttons use stable, version-independent labels from the initial HTML paint; `/api/release` upgrades only verified download links, not visible text, so refreshes no longer cause text flicker. The GitHub Release page identifies the exact available version (currently `v1.1.6`). Its ZIP contains `SonKuPik-K500-v1.1.6-Windows-Setup.exe` with the same SHA-verified Per-User installer bytes as the direct EXE; current stable installer SHA-256: `937a28a4bb715f60ba7ed06c437bbce165357d519bab824b77fa84c39b284899`.

## Documentation principles

Documentation follows the same evidence hierarchy as the application:

1. current exact source and golden tests;
2. real K500 donor/capture evidence;
3. physical hardware acceptance for hardware claims;
4. simulation/modeling as comparative engineering evidence only.

If two documents conflict, the narrower evidence-backed technical contract wins. For current release/support status, prefer `README.md`, this index, `RELEASES.md`, the parity matrix, and the release manifest. For protocol bytes, prefer `PROTOCOL_GOLDEN_VECTORS.md` and tests. For preset bytes, prefer the exact repository file and its validated hash.

## Historical milestone names

Some documents retain P0–P5 / P3.2 / P4.2 labels because those milestones identify regression contracts and are useful when tracing implementation history. They should not be interpreted as “unfinished” merely because the historical label remains. The v1 parity matrix states the current support status.
