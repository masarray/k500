# SONKUPIK STUDIO Native K500

This repository is the **Qt 6 / QML native SONKUPIK STUDIO editor for K500**. The premium mixer GUI is now wired to a native C++ K500 control stack; Electron, Web Serial, WebHID and the localhost Node bridge are not required by this build.

## Current native architecture

```text
QML controls
  -> StudioEngine canonical paths
  -> K500Controller live router / coalescing
  -> K500Protocol frame builders
  -> K500DeviceManager connection + readback state machine
  -> Win32 Bluetooth SPP COM or USB HID
  -> K500 hardware
```

Incoming device data follows the reverse direction through `K500ResponseParser`. LIVE writes remain disabled until the selected transport answers the K500 heartbeat and a verified scalar readback of device bytes `0x00..0x3F` has completed.

## Native K500 transport

### Bluetooth SPP

- Native Win32 COM transport; no QtSerialPort dependency.
- `115200 8N1`, matching the verified donor implementation.
- COM ports are protocol-probed with heartbeat `AA 01 1C E3`; a port is accepted only after a valid K500 `0xE3` response.
- The last verified K500 COM port is remembered and tried first on the next launch.

Pair `KTV_BT` in Windows first so its Bluetooth SPP COM port exists, then choose **BT** and **CONNECT** in the toolbar.

### USB HID

- Native Windows HID transport using SetupAPI / HID APIs.
- Device identity: **VID `0x10C4`, PID `0x0321`** (`USB HID DSP AUDIO`).
- Shared BT-style commands are converted at the transport boundary to USB `AA len16LE ... checksum` framing.
- HID writes use report id `0` plus the captured 64-byte payload size, with multi-report splitting for larger frames.
- USB read-block mode is converted from BT `0x63` to the verified USB `0x00` value.

Choose **USB** and **CONNECT**. Close the manufacturer's K500 application first if it already owns the HID interface.

## Safe connection sequence

The app deliberately does **not** enable live parameter transmission just because the OS handle opened. The native connection sequence is:

1. Open the selected BT COM or USB HID transport.
2. Probe with heartbeat and require K500 response `0xE3`.
3. Send handshake `0x3F`.
4. Read scalar block `0x0000..0x003F` and require response `0xBF` with at least 64 bytes.
5. Mirror device-owned bytes into `K500Controller`.
6. Enable LIVE writes.
7. Send keep-alive heartbeat every 3.2 s; stop live writes if the connection becomes stale.

This protects fields in the Music/crossover command blocks that must be mirrored from current device state rather than guessed from GUI defaults.

## Currently wired live controls

The first native engine pass wires the Music controls already backed by `StudioEngine`:

- Music PEQ live band writes.
- Music HPF / LPF crossover frequency and filter type.
- Music master volume.
- Music key.
- Input 1 / Input 2 / Bluetooth / U-Disk / Digital gains.
- Media previous / play-pause / next.
- Device mute.

High-rate edits are coalesced before transport: Music PEQ at 45 ms and block-style writes at 55 ms.

Controls whose protocol mapping has not yet been verified remain editable in the GUI but are intentionally **not guessed onto hardware**.

## Visual direction

- PEQ is the dominant hero control, with direct draggable bands and a compact selected-band control strip.
- Faders and rotary controls use restrained depth, fine highlights and small active accents rather than heavy skeuomorphism.
- Compressor behavior is visible through a live transfer curve.
- Music key remains a first-class operator control.
- Level meters use smooth motion plus peak hold and only introduce warm warning colors near headroom.
- No dashboard cards, onboarding copy, large explanatory text, mono fonts, particles or decorative animation.

## Font

The app uses **Segoe UI Variable Text**, matching the current web product's Windows typography while avoiding a bundled font payload.

## Requirements

- Windows 10/11.
- Qt 6.8 or newer (Desktop MSVC kit).
- Qt Quick + Qt Quick Controls 2.
- CMake 3.21+.
- Visual Studio with Desktop development with C++ / MSVC.
- Ninja is optional; the smart builder falls back to NMake automatically.

No `QtSerialPort`, `hidapi`, Node.js, Electron or browser runtime is required for K500 device I/O.

## Smart Windows build

The repository is drive-agnostic. It can live in either:

```text
C:\Git\k500
D:\Git\k500
```

and Qt can live in either:

```text
C:\Qt\6.8.3\msvc2022_64
D:\Qt\6.8.3\msvc2022_64
```

No hard-coded C: or D: repository path is required. `build-smart.ps1` uses its own repository location (`$PSScriptRoot`) and automatically detects Qt on C: or D:.

From Command Prompt or PowerShell, the normal one-command build is:

```bat
build-windows.cmd
```

To build, deploy Qt runtime, and immediately launch the app:

```bat
build-windows.cmd -Run
```

To force a clean rebuild:

```bat
build-windows.cmd -Clean
```

If Qt is ever installed somewhere else, override detection explicitly:

```bat
build-windows.cmd -QtRoot "E:\Qt\6.8.3\msvc2022_64"
```

The smart builder performs these steps automatically:

1. Uses the folder containing the script as the repository root, whether that is on C: or D:.
2. Detects a valid Qt Desktop MSVC installation from `QT_ROOT`, `CMAKE_PREFIX_PATH`, `C:\Qt`, or `D:\Qt`.
3. Finds Visual Studio using `vswhere.exe` and activates the MSVC x64 Developer Shell when needed.
4. Uses Ninja when available, otherwise falls back to `NMake Makefiles`.
5. Detects incompatible CMake cache when the generator or Qt path changes and cleans it automatically.
6. Builds Release.
7. Runs `windeployqt` and creates a runnable `package\SONKUPIK-STUDIO-Native-UI.exe`.

## Diagnostics

Trace protocol TX/RX while testing real hardware:

```bat
SONKUPIK-STUDIO-Native-UI.exe --trace-k500
```

Run protocol frame and RX parser golden tests:

```bat
SONKUPIK-STUDIO-Native-UI.exe --protocol-self-test
```

The existing StudioEngine model test remains available:

```bat
SONKUPIK-STUDIO-Native-UI.exe --engine-self-test
```

## Native control behavior

- Point at a PEQ graph and scroll to adjust the selected band's Q. Hold Shift for fine steps.
- Drag a PEQ node for frequency/gain; hold Shift for surgical movement.
- Hold Ctrl while dragging a PEQ node vertically to adjust Q.
- Scroll knobs, faders, value fields and sliders to change values; hold Shift for fine steps.
- Use arrow keys after focusing a control. Home resets knobs, faders and value fields.
- Double-click a knob, fader, value field or slider to reset it.

## Next engine phases

1. Expand canonical state/readback mapping beyond the currently wired Music controls.
2. Port Mic / Reverb / Echo / Main / Surround / Center / Sub live blocks whose donor mappings are already verified.
3. Port Equipment Mode recall, permanent Save and Mass Upload.
4. Hydrate the GUI from full device readback after connect/recall rather than only preserving device-owned scalar safety bytes.
5. Replace mock meter input with K500 telemetry if/when a verified meter protocol is available.
