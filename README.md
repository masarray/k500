<p align="center">
  <img src="assets/SonKuPik-k500-logo.png" alt="SonKuPik K500" width="190">
</p>

<h1 align="center">SonKuPik K500</h1>

<p align="center">
  A clear, native Windows app for controlling, tuning, and managing presets on the K500 karaoke processor.
</p>

<p align="center">
  <a href="https://sonkupik-k500.pages.dev"><strong>Product website</strong></a>
  ·
  <a href="https://github.com/masarray/k500/releases/latest"><strong>Download</strong></a>
  ·
  <a href="docs/USER_GUIDE.md"><strong>User guide</strong></a>
</p>

<p align="center">
  <a href="https://github.com/masarray/k500/releases/latest"><img alt="Latest release" src="https://img.shields.io/github/v/release/masarray/k500?display_name=tag&sort=semver"></a>
  <a href="https://github.com/masarray/k500/actions/workflows/windows-stable-release.yml"><img alt="Windows stable release" src="https://github.com/masarray/k500/actions/workflows/windows-stable-release.yml/badge.svg"></a>
  <a href="LICENSE"><img alt="GPL-3.0-or-later" src="https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg"></a>
  <img alt="Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-0078D4">
  <img alt="Qt 6" src="https://img.shields.io/badge/UI-Qt%206%20%2F%20QML-41CD52">
</p>

<p align="center">
  <img src="assets/k500%20screenshot/Main%20Output%20Section.png" alt="SonKuPik K500 Main Output interface" width="100%">
</p>

> **Stable release:** `v1.0.0` is the first public stable SonKuPik K500 release. Hardware-qualified support is **Windows 10/11 x64 + USB HID**. Bluetooth SPP is implemented but remains experimental until independently accepted on physical hardware.

## What is SonKuPik K500?

SonKuPik K500 replaces a fragmented tuning workflow with one focused desktop application. Music, microphones, vocal effects, speaker outputs, device modes, and PC presets are organized as clear sections inside the same native Windows interface.

You do **not** need to understand the K500 protocol to use the application. Connect the processor, let SonKuPik read the current device state, then work from the section you want to tune.

Typical uses include:

- shaping music tone and PEQ;
- tuning microphone response;
- adjusting reverb and echo;
- balancing Main, Center, Surround, and Subwoofer outputs;
- recalling K500 equipment modes;
- previewing and organizing `.k500` presets;
- uploading individual presets or a complete 10-slot bank.

## Download

| Package | Best for | Link |
|---|---|---|
| **Windows Setup** | Recommended for most users; normal installation, Start Menu shortcut, uninstall support | [Download v1.0.0 Setup](https://github.com/masarray/k500/releases/download/v1.0.0/SonKuPik-K500-v1.0.0-Windows-Setup.exe) |
| **Portable ZIP** | Run without installation; extract first, then launch | [Download v1.0.0 Portable](https://github.com/masarray/k500/releases/download/v1.0.0/SonKuPik-K500-v1.0.0-Windows-Portable.zip) |
| **Release metadata** | SHA-256 checksums and provenance | [Latest release](https://github.com/masarray/k500/releases/latest) |

Official Windows binaries are currently unsigned open-source builds. Windows SmartScreen or third-party antivirus reputation systems may therefore warn on a newly downloaded binary. Published releases include `SHA256SUMS.txt` and `release-manifest.json` for verification. See [Windows distribution & security](docs/WINDOWS_DISTRIBUTION_SECURITY.md).

## Quick start

1. **Install SonKuPik K500** using the Windows Setup package.
2. **Connect the K500 over USB.** The app performs the device handshake and reads the current processor state before enabling LIVE editing.
3. **Choose a section and tune.** Music, Mic, Reverb, Echo, Main, Center, Surround, Subwoofer, and System use a consistent workflow.

For Preview, Save As, Upload, Mass Upload, preset Sync, and diagnostics, read the [User Guide](docs/USER_GUIDE.md).

## The interface

The application is organized around the real signal path instead of exposing protocol details to the user.

<p align="center">
  <img src="assets/k500%20screenshot/Music%20Setting%20Section.png" alt="Music Setting section" width="49%">
  <img src="assets/k500%20screenshot/Mic%20Setting%20Section.png" alt="Microphone Setting section" width="49%">
</p>

<p align="center">
  <img src="assets/k500%20screenshot/Vocal%20Effect%20Reverb%20Section.png" alt="Vocal Reverb section" width="49%">
  <img src="assets/k500%20screenshot/Vocal%20Echo%20Delay%20Section.png" alt="Vocal Echo Delay section" width="49%">
</p>

### Music and microphones

Music and microphone processing have dedicated workspaces so tone shaping remains easy to follow. PEQ response, controls, and the active section stay visible together.

### Vocal effects

Reverb and Echo are separate sections. This makes it easier to understand whether you are changing vocal space, decay, delay, or tonal balance rather than mixing unrelated controls into one page.

### Speaker outputs

Main, Center, Surround, and Subwoofer follow the same visual model, reducing the learning curve when moving around the system.

<p align="center">
  <img src="assets/k500%20screenshot/Main%20Output%20Section.png" alt="Main Output section" width="49%">
  <img src="assets/k500%20screenshot/Subwoofer%20Section.png" alt="Subwoofer section" width="49%">
</p>

### System and presets

The System section keeps device mode actions and PC preset workflows together while preserving an important distinction: selecting a preset is not the same as permanently writing it to the K500.

<p align="center">
  <img src="assets/k500%20screenshot/System%20Section.png" alt="System and preset management section" width="100%">
</p>

## Presets without surprises

SonKuPik separates four states that are easy to confuse in ordinary device software:

1. **Hardware State** — what is actually active in the connected K500.
2. **Staged PC Preset** — the `.k500` file selected in the application.
3. **Offline Preview State** — an explicit preview/edit session that does not silently overwrite hardware.
4. **Mass Upload Staging** — a reviewable Slot 01…10 mapping before permanent transfer.

The PC preset library combines:

- **SONKUPIK** — bundled official presets plus validated GitHub cache updates;
- **LOCAL** — user-owned presets from a selected local folder.

Official sync validates downloaded preset files before they replace the last-known-good cache. Local files are never overwritten by official sync.

## Built around device truth

The connected K500 is authoritative. SonKuPik does not enable LIVE editing from stale UI defaults.

The connection flow is deliberately:

```text
Connect
  → heartbeat / handshake
  → complete hardware readback
  → hydrate the editor
  → enable LIVE editing
```

Permanent preset actions are also explicit. Unsupported hardware commands remain read-only instead of inventing packets just to make a UI control appear complete.

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

**Intentionally read-only / unsupported:** persistent LCD/Equipment Mode rename remains disabled until a donor-verified native rename transaction is captured.

## Engineering details

The user interface intentionally hides most binary details, but the underlying contracts are strict:

| Contract | Stable truth |
|---|---:|
| Complete active-memory readback | **939 bytes / `0x03AB`** |
| Native `.k500` file | **1144 bytes / `0x0478`** |
| Permanent slot image | **656 bytes / `0x0290`** |
| Proven Mass Upload order | **Slot 10 → … → Slot 01** |

The `.k500` engine validates exact container size and checksum, preserves unknown/reserved bytes, supports byte-identical no-op round trips, and converts files through the verified codec into the native permanent slot representation.

The official Mode 01 file in `resources/presets/01_ALL_GENRE.k500` is the exact hardware-accepted native `CONCERT HIFI V4` donor:

```text
SHA-256
9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

## Architecture

Normal LIVE editing follows one ownership path:

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
5. **No-op means byte-identical.** Preset normalization must not rewrite unrelated bytes.
6. **Stable means protected.** New work should use a branch/PR and must not weaken earlier regression guards to make new behavior pass.

See [CONTRIBUTING.md](CONTRIBUTING.md) before changing hardware-facing code or official presets.

## Support and diagnostics

Use **Support** in the top toolbar to save a bounded JSON diagnostics report. It includes build/runtime metadata, connection state, last diagnostic TX/RX, and protocol history while deliberately excluding full active memory, preset bytes, and local preset paths.

For bug reports, use the repository issue templates and include the exact application version, commit when known, Windows version, transport, reproduction steps, and Support Report/trace where appropriate.

## Security

Protocol integrity and permanent device writes are treated as safety-sensitive surfaces. Please read [SECURITY.md](SECURITY.md) before reporting vulnerabilities or proposing destructive hardware changes.

## License

SonKuPik K500 is released under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. See [LICENSE](LICENSE).
