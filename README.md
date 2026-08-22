# SONKUPIK STUDIO — Native K500

Native **Qt 6 / QML** editor and control application for the K500 karaoke processor.

The project ports the verified K500 behavior from the earlier Web/Electron donor into a Windows-native C++ stack. Normal operation does **not** require Electron, Node.js, Web Serial, WebHID, a browser, or a localhost bridge.

> **Release status:** P5 release-candidate hardening. The software regression suite is automated; physical K500 qualification is still required before hardware-facing features are promoted to `LOCKED ✅` or the project is tagged stable `v1.0`.

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
- Windows Bluetooth SPP COM probing at `115200 8N1`.
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
- Deterministic multi-file Mass Upload: filename sort -> sequential selected slots -> native descending device transaction.
- Whole batch aborts before device writes if any member is invalid.

**Important:** a K500 permanent slot image is not the first `0x0290` bytes of a `.k500` file. The native codec performs the verified scalar split and compact EQ conversion.

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
- P4.2 donor batch-library tests.
- deployed runtime font, protocol/RX and StudioEngine self-tests.

Later milestones may extend this fortress, but must not weaken earlier invariants to make a new feature pass.

## Support diagnostics

Use the **Support** action in the top toolbar to save a JSON report for hardware acceptance or bug reports.

The report contains build/runtime metadata, transport/status, last error, last TX/RX and a bounded protocol event history. It deliberately excludes:

- the 939-byte active-memory image;
- `.k500` preset bytes;
- local preset paths.

For live console tracing:

```bat
SONKUPIK-STUDIO-Native-UI.exe --trace-k500
```

Built-in runtime tests:

```bat
SONKUPIK-STUDIO-Native-UI.exe --font-self-test
SONKUPIK-STUDIO-Native-UI.exe --protocol-self-test
SONKUPIK-STUDIO-Native-UI.exe --engine-self-test
```

## Windows packages

CI builds two separate artifacts:

- `SONKUPIK-STUDIO-v<version>-Windows-Setup.exe`
- `SONKUPIK-STUDIO-v<version>-Windows-Portable-Single.exe`

`SHA256SUMS.txt` is generated for release verification. P5 also adds a machine-readable release manifest that explicitly records whether physical hardware acceptance is complete.

## Hardware acceptance

Software CI cannot prove electrical/device persistence behavior. Before stable `v1.0`, test the exact release candidate against a physical K500 using:

- `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md`
- `docs/P5_RELEASE_READINESS.md`

USB and Bluetooth must be qualified independently. Permanent Save, PC Upload and Mass Upload must survive reconnect and power cycle with non-target slots preserved.

## License

SONKUPIK STUDIO Native K500 is released under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. See `LICENSE`.

Qt and other third-party components remain subject to their respective licenses.
