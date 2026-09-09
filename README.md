<p align="center">
  <img src="assets/SonKuPik-k500-logo.png" alt="SonKuPik K500" width="220">
</p>

<h1 align="center">SonKuPik K500</h1>

<p align="center">
  Native Windows control, preset management, and engineering software for the K500 karaoke processor.
</p>

<p align="center">
  <a href="https://github.com/masarray/k500/releases/latest"><img alt="Latest release" src="https://img.shields.io/github/v/release/masarray/k500?display_name=tag&sort=semver"></a>
  <a href="https://github.com/masarray/k500/actions/workflows/windows-stable-release.yml"><img alt="Windows stable release" src="https://github.com/masarray/k500/actions/workflows/windows-stable-release.yml/badge.svg"></a>
  <a href="LICENSE"><img alt="GPL-3.0-or-later" src="https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg"></a>
  <img alt="Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-0078D4">
  <img alt="Qt 6" src="https://img.shields.io/badge/UI-Qt%206%20%2F%20QML-41CD52">
</p>

> **Stable release:** `v1.0.0` is the first public stable SonKuPik K500 release. The hardware-qualified support scope is **Windows 10/11 x64 + USB HID**. Bluetooth SPP is implemented but remains experimental until independently accepted on physical hardware.

SonKuPik K500 is an independent open-source implementation. It is built around a strict device-truth model: the connected processor is authoritative, destructive writes are transaction-gated, unknown bytes are preserved, and unsupported hardware commands remain read-only instead of being guessed.

## Download

| Package | Recommended for | Link |
|---|---|---|
| Windows Setup | Normal installation, Start Menu shortcut, uninstall support | [Download v1.0.0 Setup](https://github.com/masarray/k500/releases/download/v1.0.0/SonKuPik-K500-v1.0.0-Windows-Setup.exe) |
| Portable ZIP | No installer; extract and run | [Download v1.0.0 Portable](https://github.com/masarray/k500/releases/download/v1.0.0/SonKuPik-K500-v1.0.0-Windows-Portable.zip) |
| Release metadata | SHA-256 + machine-readable provenance | [Latest release](https://github.com/masarray/k500/releases/latest) |

Official Windows binaries are currently unsigned open-source builds. Windows SmartScreen or third-party antivirus reputation systems can therefore warn on a new binary. Always verify downloads against `SHA256SUMS.txt` and `release-manifest.json`. See [Windows distribution & security](docs/WINDOWS_DISTRIBUTION_SECURITY.md).

## Why this project is different

### Hardware truth before editing

Connection is promoted to LIVE only after heartbeat, handshake, and a complete **939-byte (`0x03AB`) active-memory readback**. The editor hydrates from the K500 while LIVE writes are disabled, so opening the app cannot silently replay stale UI defaults into the processor.

### Native preset workflow

The `.k500` engine validates the exact **1144-byte (`0x0478`)** container, preserves unknown/reserved bytes, supports byte-identical no-op round trips, and converts a file through the verified codec into the K500's **656-byte (`0x0290`)** permanent slot image.

### Unified Official + Local library

Mass Upload no longer depends on an empty user-selected folder. The PC collection combines:

- **SONKUPIK** — bundled official presets plus validated GitHub updates cached locally;
- **LOCAL** — user presets from a writable local folder.

Official updates fail safely: a downloaded preset must pass K500 file validation before it replaces the last-known-good cache. Local user files are never overwritten by official sync.

### Proven permanent upload path

Single Upload and Mass Upload reuse the native Store transaction path. Mass Upload validates the entire batch before device writes, then executes the proven descending hardware order **Slot 10 → … → Slot 01**, recalls Slot 01, and performs a fresh full readback before returning to LIVE.

### Crash-resistant section navigation

Mic, Reverb, Echo, Main, Surround, Center, and Sub use stable EQ-page lifetimes instead of hot-swapping 10/7/5-band models through one Canvas instance. Runtime CI repeatedly crosses those sections to catch the class of heap/UI crash that was fixed before v1.0.

## Stable v1.0 feature surface

- Native Qt 6 / QML Windows application — no Electron, browser, Node.js, WebHID, or localhost bridge required for normal operation.
- USB HID transport for K500 `VID:PID 10C4:0321`, report ID 0, 64-byte HID reports.
- Bluetooth SPP transport implementation at `115200 8N1` (**experimental in v1.0 support policy**).
- Device-truth hydration and heartbeat watchdog before LIVE editing.
- Music, Mic, Effect, PEQ, crossover, routing, mute/media, and output-block command families where donor/native protocol evidence exists.
- Equipment Mode Recall 1–10 with authoritative post-recall readback.
- Use Init Volume transaction with verified ACK handling.
- USB-only permanent Save, single PC preset Upload, and deterministic Mass Upload.
- Offline preset Preview/edit tracking with controlled byte-whitelist persistence and atomic Save As.
- Official preset sync, offline bundled fallback, last-known-good cache, and Local preset library.
- Bounded Support Report JSON with active-memory/preset payload/path redaction.
- Branded Inno Setup installer and ordinary portable ZIP.

**Intentionally read-only / unsupported:** persistent LCD/Equipment Mode rename remains disabled until a donor-verified native rename transaction is captured. The project never invents a packet to make a UI field look complete.

## Preset integrity

The official Mode 01 file in `resources/presets/01_ALL_GENRE.k500` is the exact hardware-accepted native `CONCERT HIFI V4` donor.

```text
SHA-256
9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

Its identity is guarded in CI. Official presets can be updated independently of the application binary through **Sync Official Presets**, but every remote file is validated before becoming usable.

## Architecture

```text
QML controls
    │
    ▼
StudioEngine — canonical editor state
    │
    ▼
K500Controller — live routing / coalescing
    │
    ▼
K500DeviceManager — connection, readback, diagnostics
    │
    ▼
K500WinIo — Win32 USB HID / Bluetooth SPP
    │
    ▼
K500 hardware
```

Preset transactions deliberately use a sibling coordinator:

```text
System UI
    │
    ▼
K500PresetManager
    │
    ▼
K500DeviceManager
    │
    ▼
K500WinIo
```

QML never owns a raw transport handle. See [Architecture](docs/ARCHITECTURE.md) for state layers, transaction boundaries, failure behavior, and preset sync design.

## State model

The application keeps four concepts separate:

1. **Hardware State** — actual K500 readback, active slot, and device mode names.
2. **Staged PC Preset** — selected local/official `.k500`; selection alone never changes hardware.
3. **Offline Preview State** — editor hydration only after explicit Preview; edits are tracked against the staged file.
4. **Mass Upload Staging** — reviewable Slot 01…10 mapping before any permanent device write.

This separation is a core safety invariant, not just a UI convention.

## Documentation

Start with the [Documentation Index](docs/README.md).

| Topic | Document |
|---|---|
| End-user workflow | [User Guide](docs/USER_GUIDE.md) |
| Native architecture & state authority | [Architecture](docs/ARCHITECTURE.md) |
| Release channels, packages & preset updates | [Release Model](docs/RELEASES.md) |
| Capability / protocol boundary | [Porting Parity Matrix](docs/PORTING_PARITY_MATRIX.md) |
| Golden command vectors | [Protocol Golden Vectors](docs/PROTOCOL_GOLDEN_VECTORS.md) |
| `.k500` codec contract | [Bit-perfect Codec](docs/P3_K500_CODEC.md) |
| Hardware validation | [Hardware Acceptance Checklist](docs/HARDWARE_ACCEPTANCE_CHECKLIST.md) |
| v1 stable qualification | [v1.0 Release Readiness](docs/P5_RELEASE_READINESS.md) |
| Preset research / AI workflow | [AI Preset Engineering Playbook](docs/K500_AI_PRESET_ENGINEERING_PLAYBOOK.md) |
| Windows signing / antivirus policy | [Windows Distribution Security](docs/WINDOWS_DISTRIBUTION_SECURITY.md) |

## Build from source

Requirements:

- Windows 10/11 x64;
- Qt 6.8+ Desktop MSVC kit;
- Visual Studio / MSVC C++ toolchain;
- CMake 3.21+.

```bat
build-windows.cmd
```

Build, deploy the Qt runtime, and launch:

```bat
build-windows.cmd -Run
```

Clean rebuild:

```bat
build-windows.cmd -Clean
```

The release pipeline currently validates against Qt **6.10.2** and runs protocol, codec, preset persistence, batch upload, runtime navigation, portable-package, and installed-package regression tests.

## Engineering rules

1. **Device truth wins.** Read hardware before enabling LIVE.
2. **No guessed writes.** Unknown command semantics stay read-only.
3. **Preserve neighboring bytes.** Patch only donor-verified fields.
4. **Fail closed.** Uncertain destructive transactions require reconnect/readback.
5. **No-op means byte-identical.** Preset normalization is not allowed to rewrite unrelated bytes.
6. **Stable means protected.** New work should use a branch/PR and must not weaken earlier regression guards to make new behavior pass.

See [CONTRIBUTING.md](CONTRIBUTING.md) before changing hardware-facing code or official presets.

## Support and diagnostics

Use **Support** in the top toolbar to save a bounded JSON diagnostics report. It includes build/runtime metadata, connection state, last diagnostic TX/RX, and protocol history while deliberately excluding full active memory, preset bytes, and local preset paths.

For bug reports, use the repository issue templates and include the exact application version, commit when known, Windows version, transport, reproduction steps, and Support Report/trace where appropriate.

## Security

Protocol integrity and permanent device writes are treated as safety-sensitive surfaces. Please read [SECURITY.md](SECURITY.md) before reporting vulnerabilities or proposing destructive hardware changes.

## License

SonKuPik K500 is released under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. See [LICENSE](LICENSE).
