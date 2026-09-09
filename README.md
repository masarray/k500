<p align="center">
  <img src="assets/SonKuPik-k500-logo.png" alt="SonKuPik K500" width="240">
</p>

# SonKuPik K500

Native **Qt 6 / QML** editor and control application for the K500 karaoke processor.

The project ports the verified K500 behavior from the earlier Web/Electron donor into a Windows-native C++ stack. Normal operation does **not** require Electron, Node.js, Web Serial, WebHID, a browser, or a localhost bridge.

> **Release status:** **v1.0 public stable**. The Windows x64 + USB HID workflow has been accepted on physical K500 hardware. Bluetooth SPP remains implemented but is not part of the v1.0 hardware-qualified support claim and should be treated as experimental until independently accepted.

## Architecture

```text
QML controls
  -> StudioEngine canonical paths
  -> K500Controller live routing / coalescing
  -> K500DeviceManager
  -> K500WinIo
  -> Win32 Bluetooth SPP or USB HID
  -> K500 hardware
```

Preset transactions use one deliberate sibling coordinator:

```text
System UI
  -> K500PresetManager
  -> K500DeviceManager
  -> K500WinIo
```

No QML component owns a raw transport handle.

## Implemented software surface

### Native device connection

- Windows USB HID, VID/PID `10C4:0321`, report ID 0, 64-byte HID reports.
- Windows Bluetooth SPP COM probing at `115200 8N1` (experimental support boundary for v1.0).
- Heartbeat and handshake validation before LIVE is allowed.
- Full active-memory synchronization: `0x03AB` / **939 bytes**.
- Readback in verified `0x003A` blocks with 35 ms pacing.
- Device state hydrates the editor while LIVE is OFF; hydration emits zero echo writes.
- Heartbeat watchdog fails closed and disables LIVE on a stale connection.

### Verified live-command surface

The native router implements the donor-verified commands for:

- Music master, inputs and key.
- Top Mic and Top Effect blocks.
- Mic A/B, Music, Main, Surround, Center, Sub, Reverb and Echo PEQ.
- Verified HPF/LPF selectors.
- Mic EQ Link.
- Main / Surround / Center / Sub output blocks while preserving unknown device-owned bytes.
- Surround L/R delay.
- Mute and media transport.

Fields for which no verified live command exists remain non-destructive rather than guessing protocol bytes. See `docs/PORTING_PARITY_MATRIX.md` for the exact boundary.

### Device preset management

- Equipment Mode Recall 1–10 with authoritative full 939-byte resync before LIVE resumes.
- Use Init Volume transaction with verified ACK handling.
- USB-only permanent Save from fresh device RAM.
- Native Store Begin / Chunk / Commit transaction handling.
- Fail-closed behavior on uncertain destructive transactions.
- Native Mass Upload engine with verified descending slot order and store-chain handling.

Persistent LCD/Equipment Mode rename remains read-only until a donor-verified native write transaction is captured.

### `.k500` file workflow

- Exact 1144-byte (`0x0478`) file validation.
- Additive checksum validation and update.
- Byte-identical no-edit round trip.
- Unknown/reserved-byte preservation.
- Raw PEQ type alias preservation.
- Controlled edits through explicit verified byte whitelists.
- Atomic Save As.
- Verified `.k500` -> native 656-byte (`0x0290`) slot-image conversion.
- Single PC preset permanent upload to the selected K500 slot.
- Deterministic multi-file Mass Upload: selected Slot 01–10 mapping -> native descending device transaction 10 → 1 -> final Slot 01 recall/readback.
- Whole batch aborts before device writes if any member is invalid.

**Important:** a K500 permanent slot image is not the first `0x0290` bytes of a `.k500` file. The native codec performs the verified scalar split and compact EQ conversion.

### Official + local preset library

- Built-in SonKuPik presets remain available offline.
- Official presets can be synchronized from `resources/presets` on this repository.
- New or updated official `.k500` files are validated before entering the local cache.
- A failed download never replaces the last-known-good cached preset.
- User-created/local presets are shown together with Official presets in the PC Preset Collection.
- Official and Local presets can be mixed freely in the 1–10 Mass Upload transfer list.
- Native Mode 01 `CONCERT HIFI V4` is preserved byte-for-byte and guarded against accidental reconstruction/regression.

### AI / research preset engineering

A fresh ChatGPT/Codex/AI thread can reconstruct the full preset-engineering workflow directly from this repository. Start with:

- `AGENTS.md` — mandatory AI entry point and safety rules.
- `docs/K500_AI_PRESET_ENGINEERING_PLAYBOOK.md` — sonic research method, output roles, psychoacoustic guardrails, simulation, iteration and hardware-feedback loop.
- `docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md` — authoritative binary map and proven K500 signal flow.
- `tools/k500_preset_lab.py` — validation, decode, donor-based surgical patching, Mic/Music/Main/FX/output response graphs, CSV/JSON export and A/B comparison.

Typical research commands:

```bash
python -m pip install -r tools/requirements-preset-lab.txt
python tools/k500_preset_lab.py inspect preset.k500 --json preset.json
python tools/k500_preset_lab.py plot preset.k500 --out-dir analysis/
python tools/k500_preset_lab.py compare donor.k500 candidate.k500 --out-dir comparison/
python tools/k500_preset_lab.py patch donor.k500 tools/k500_patch_example.json candidate.k500
```

Preset simulation is intentionally **comparative**. Real K500 hardware listening remains authoritative, and all generated presets must remain donor-based, checksum-valid and byte-audited.

## Safety model

The project follows four non-negotiable rules:

1. **Device truth on connect.** Current K500 state is read before LIVE is enabled.
2. **No guessed hardware writes.** Unsupported paths are intentionally non-destructive.
3. **Preserve neighboring bytes.** Block writes are seeded from device truth or verified file bytes and patch only proven fields.
4. **Fail closed.** A destructive preset transaction with uncertain state drops the transport and requires a fresh reconnect/readback.

## Windows build

Requirements:

- Windows 10/11 x64.
- Qt 6.8+ Desktop MSVC kit.
- Visual Studio / MSVC C++ toolchain.
- CMake 3.21+.

Normal local build:

```bat
build-windows.cmd
```

Build, deploy Qt runtime and launch:

```bat
build-windows.cmd -Run
```

Clean rebuild:

```bat
build-windows.cmd -Clean
```

The smart builder detects the repository and Qt installation without requiring a hard-coded C: or D: path.

## Automated regression fortress

The Windows CI build protects the complete software stack, including:

- UI interaction and embedded Plus Jakarta Sans typography invariants.
- P0 connection/hydration architecture.
- P1 donor-verified live routing.
- P2 permanent preset protocol vectors.
- P3 synthetic bit-perfect codec tests.
- P3.2 real donor `.k500` corpus tests.
- P3.4 controlled-edit persistence tests.
- P4/P4.2 upload and donor batch-library tests.
- Official preset library and recovered-progress guards.
- Deployed runtime font, protocol/RX, StudioEngine and section-navigation stress tests.
- Portable ZIP and installed Inno application runtime validation.

Later milestones may extend this fortress, but must not weaken earlier invariants to make a new feature pass.

## Support diagnostics

Use the **Support** action in the top toolbar to save a JSON report for hardware acceptance or bug reports.

The report contains build/runtime metadata, transport/status, last error, last TX/RX and a bounded protocol event history. It deliberately excludes:

- the 939-byte active-memory image;
- `.k500` preset bytes;
- local preset paths.

For live console tracing, the internal regression-stable executable is currently:

```bat
SONKUPIK-STUDIO-Native-UI.exe --trace-k500
```

Built-in runtime tests:

```bat
SONKUPIK-STUDIO-Native-UI.exe --font-self-test
SONKUPIK-STUDIO-Native-UI.exe --protocol-self-test
SONKUPIK-STUDIO-Native-UI.exe --engine-self-test
```

The user-facing product identity, window title, installer, shortcuts and release packages are **SonKuPik K500**. The internal executable target remains intentionally stable for regression harness compatibility.

## Windows packages

Stable CI publishes two separate branded artifacts:

- `SonKuPik-K500-v<version>-Windows-Setup.exe`
- `SonKuPik-K500-v<version>-Windows-Portable.zip`

`SHA256SUMS.txt` and `release-manifest.json` are generated for every public stable release. The Windows build is currently unsigned open-source software, so SmartScreen or antivirus reputation warnings can still occur on new binaries.

## Web / landing page branding

The static landing page is in `docs/index.html`. Its favicon, web logo, repository brand image, application window icon and application header all use the same authoritative source image:

`assets/SonKuPik-k500-logo.png`

The published web copies under `docs/` intentionally remain byte-identical to that source image.

## Hardware acceptance

The v1.0 stable hardware-qualified support boundary is **Windows x64 + USB HID**. The stable baseline was promoted after maintainer acceptance on physical K500 hardware, including the corrected native Mode 01 preset and crash-proof section navigation.

Bluetooth SPP remains available but is explicitly outside the v1.0 qualified hardware claim until independently accepted. Any future destructive protocol change must be revalidated against a physical K500 before the stable support boundary is expanded.

See `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md` and `docs/P5_RELEASE_READINESS.md` for acceptance boundaries and regression requirements.

## License

SonKuPik K500 is released under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. See `LICENSE`.

Qt and other third-party components remain subject to their respective licenses.
