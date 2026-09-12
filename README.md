<p align="center">
  <img src="assets/SonKuPik-k500-logo.png" alt="SonKuPik K500" width="200">
</p>

<h1 align="center">SonKuPik K500</h1>

<p align="center">
  A free native Windows app for controlling, tuning, and managing presets on the K500 karaoke processor.
</p>

<p align="center">
  <a href="https://github.com/masarray/k500/releases/latest"><img alt="Latest release" src="https://img.shields.io/github/v/release/masarray/k500?display_name=tag&sort=semver"></a>
  <a href="https://github.com/masarray/k500/actions/workflows/windows-stable-release.yml"><img alt="Windows stable release" src="https://github.com/masarray/k500/actions/workflows/windows-stable-release.yml/badge.svg"></a>
  <a href="LICENSE"><img alt="GPL-3.0-or-later" src="https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg"></a>
  <img alt="Windows x64" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-0078D4">
  <img alt="Qt 6" src="https://img.shields.io/badge/native-Qt%206%20%2F%20QML-41CD52">
</p>

<p align="center">
  <a href="https://sonkupik-k500.pages.dev"><strong>Product website</strong></a>
  ·
  <a href="https://github.com/masarray/k500/releases/latest"><strong>Download</strong></a>
  ·
  <a href="docs/USER_GUIDE.md"><strong>User guide</strong></a>
</p>

---

## What is SonKuPik K500?

SonKuPik K500 gives K500 owners a modern Windows interface for the parts of the processor they use every day:

- tune **music** EQ and tonal balance;
- adjust **microphone** tone and dynamics;
- shape **reverb** and **echo**;
- control **Main, Center, Surround, and Subwoofer** outputs;
- manage **official and local presets**;
- preview settings before making a permanent preset upload.

The app is native Qt 6 / QML — no browser, Electron, Node.js, WebHID, or localhost bridge is required for normal use.

> **Stable support:** Windows 10/11 x64 + K500 over USB HID. Bluetooth SPP is implemented but remains experimental until independently hardware-qualified.

## See the real app

<p align="center">
  <img src="assets/k500%20screenshot/Music%20Setting%20Section.png" alt="SonKuPik K500 Music Setting screen" width="900">
</p>

### Music

See the complete PEQ response while adjusting frequency, gain, and Q. The graph stays visible so changes are easier to understand instead of becoming a list of numbers.

<p align="center">
  <img src="assets/k500%20screenshot/Mic%20Setting%20Section.png" alt="SonKuPik K500 Microphone Setting screen" width="760">
</p>

### Microphone

Microphone tuning is kept separate from music and speaker-output controls, making it easier to focus on vocal clarity and comfort.

<table>
<tr>
<td width="50%"><img src="assets/k500%20screenshot/Vocal%20Effect%20Reverb%20Section.png" alt="Vocal Reverb screen"></td>
<td width="50%"><img src="assets/k500%20screenshot/Vocal%20Echo%20Delay%20Section.png" alt="Vocal Echo screen"></td>
</tr>
<tr>
<td align="center"><strong>Reverb</strong></td>
<td align="center"><strong>Echo / Delay</strong></td>
</tr>
</table>

<table>
<tr>
<td width="50%"><img src="assets/k500%20screenshot/Main%20Output%20Section.png" alt="Main Output screen"></td>
<td width="50%"><img src="assets/k500%20screenshot/Subwoofer%20Section.png" alt="Subwoofer screen"></td>
</tr>
<tr>
<td align="center"><strong>Main output</strong></td>
<td align="center"><strong>Subwoofer</strong></td>
</tr>
</table>

More screenshots are available in [`assets/k500 screenshot`](assets/k500%20screenshot/), including Center, Surround, System, Mic B, and the physical K500 device.

## Download

| Package | Best for | Download |
|---|---|---|
| **Windows Setup** | Recommended for most users. Normal installation, Start Menu shortcut, uninstall support. | [Download Setup](https://github.com/masarray/k500/releases/download/v1.0.0/SonKuPik-K500-v1.0.0-Windows-Setup.exe) |
| **Portable ZIP** | No installation. Extract the ZIP and run the application. | [Download Portable](https://github.com/masarray/k500/releases/download/v1.0.0/SonKuPik-K500-v1.0.0-Windows-Portable.zip) |
| **Latest release** | Release notes, SHA-256 checksums, and current packages. | [Open latest release](https://github.com/masarray/k500/releases/latest) |

Official Windows binaries are currently unsigned open-source builds. Windows SmartScreen or antivirus reputation systems may therefore warn when opening a new download. Release packages include SHA-256 verification metadata. See [Windows distribution & security](docs/WINDOWS_DISTRIBUTION_SECURITY.md).

## Getting started

1. **Install SonKuPik K500** using the Windows Setup package.
2. **Connect the K500 by USB.** The app reads the processor state before LIVE editing is enabled.
3. **Choose a section** such as Music, Mic, Reverb, Main, or Subwoofer.
4. **Tune the sound** or select a preset.
5. For presets, use **Preview** before choosing a permanent upload.

For a complete walkthrough, see the [User Guide](docs/USER_GUIDE.md).

## Why the app is careful with your K500

A friendly interface should not hide risky behavior. SonKuPik K500 follows a strict device-truth model:

- **Hardware state comes first.** The app reads the connected processor before enabling LIVE editing.
- **Preset files are validated.** Native `.k500` containers are checked before use.
- **Unknown commands stay read-only.** The project never invents a packet merely to make a control appear complete.
- **Permanent writes are deliberate.** Staging, preview, and device upload are separate states.
- **Readback verifies reality.** After important device operations, the processor is read again before LIVE editing resumes.

This design is intended to make the application approachable for everyday users while keeping hardware-facing behavior conservative and inspectable.

## Preset workflow

The application keeps four concepts separate:

1. **Hardware State** — what the connected K500 actually reports.
2. **Staged PC Preset** — the `.k500` file selected on the PC; selecting it alone does not change hardware.
3. **Offline Preview State** — an explicit preview/edit session.
4. **Mass Upload Staging** — a reviewable Slot 01–10 mapping before permanent writes begin.

The PC collection combines:

- **SONKUPIK** — bundled official presets plus validated GitHub cache updates;
- **LOCAL** — presets owned by the user in a selected local folder.

Official downloads must pass K500 file validation before replacing the last-known-good cache, and official sync never overwrites Local user files.

## Stable feature surface

- Native Qt 6 / QML Windows application.
- USB HID transport for K500 `VID:PID 10C4:0321`, report ID 0, 64-byte HID reports.
- Bluetooth SPP implementation at `115200 8N1` (**experimental** in the current stable support policy).
- Device-truth hydration and heartbeat watchdog before LIVE editing.
- Music, Mic, Effect, PEQ, crossover, routing, mute/media, and supported output-block command families.
- Equipment Mode Recall 1–10 with authoritative post-recall readback.
- USB-only permanent Save, single PC preset Upload, and deterministic Mass Upload.
- Offline preset Preview/edit tracking with controlled persistence and atomic Save As.
- Official preset sync, bundled offline fallback, last-known-good cache, and Local preset library.
- Support Report JSON with sensitive preset payload/path information excluded.
- Branded Windows installer and portable ZIP.

**Intentionally unsupported:** persistent LCD / Equipment Mode rename remains disabled until a donor-verified native rename transaction is captured.

## Engineering notes

For contributors and protocol work, the important invariants are intentionally stricter than the simple user-facing workflow above.

### Hardware truth before editing

Connection is promoted to LIVE only after heartbeat, handshake, and a complete **939-byte (`0x03AB`) active-memory readback**. The editor hydrates from the K500 while LIVE writes are disabled, preventing stale UI defaults from being replayed into the processor during startup.

### Native preset integrity

The `.k500` engine validates the exact **1144-byte (`0x0478`)** container, preserves unknown/reserved bytes, supports byte-identical no-op round trips, and converts a file through the verified codec into the K500's **656-byte (`0x0290`)** permanent slot image.

### Permanent upload path

Single Upload and Mass Upload reuse the verified Store transaction path. Mass Upload validates the full batch before device writes, executes the proven descending order **Slot 10 → … → Slot 01**, recalls Slot 01, then performs a fresh full readback before returning to LIVE.

### Crash-resistant section navigation

Mic, Reverb, Echo, Main, Surround, Center, and Sub use stable EQ-page lifetimes rather than hot-swapping incompatible 10/7/5-band models through a single graph instance. Runtime CI crosses these sections repeatedly to guard the navigation crash class fixed before v1.0.

## Preset integrity reference

The official Mode 01 file at `resources/presets/01_ALL_GENRE.k500` is the exact hardware-accepted native `CONCERT HIFI V4` donor.

```text
SHA-256
9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

Its identity is guarded in CI. Remote official preset updates are validated before becoming usable.

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

Preset transactions use a sibling coordinator:

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

QML never owns a raw transport handle. See [Architecture](docs/ARCHITECTURE.md) for the complete state and transaction model.

## Documentation

| Topic | Document |
|---|---|
| End-user workflow | [User Guide](docs/USER_GUIDE.md) |
| Documentation index | [Docs](docs/README.md) |
| Native architecture & state authority | [Architecture](docs/ARCHITECTURE.md) |
| Releases and packages | [Release Model](docs/RELEASES.md) |
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

The stable release pipeline validates protocol, codec, preset persistence, batch upload, runtime navigation, portable-package, and installed-package regressions.

## Contributing

Before changing hardware-facing code or official presets, read [CONTRIBUTING.md](CONTRIBUTING.md) and [`AGENTS.md`](AGENTS.md).

Core rules:

1. **Device truth wins.**
2. **Never guess hardware writes.**
3. **Preserve neighboring / unknown bytes.**
4. **Fail closed on uncertain destructive actions.**
5. **No-op means byte-identical.**
6. **Protect the stable baseline with regression tests.**

## Support

Use **Support** in the application toolbar to save a bounded diagnostics JSON report. For bugs, open a GitHub issue and include the application version, Windows version, transport, reproduction steps, and Support Report where appropriate.

## License

SonKuPik K500 is released under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. See [LICENSE](LICENSE).
