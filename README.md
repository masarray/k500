# SONKUPIK STUDIO Native UI Prototype

This repository is a **standalone Qt 6 / QML native prototype** for SONKUPIK STUDIO / K500. It intentionally does not connect to K500 hardware yet. The current goal is to approve the premium mixer UI, interaction density, motion and control feel before porting the communication engine.

## Visual direction

- PEQ is the dominant hero control, with direct draggable bands and a compact selected-band control strip.
- Faders and rotary controls use restrained depth, fine highlights and small active accents rather than heavy skeuomorphism.
- Compressor behavior is visible through a live transfer curve.
- Music key remains a first-class operator control.
- Level meters use smooth motion plus peak hold and only introduce warm warning colors near headroom.
- No dashboard cards, onboarding copy, large explanatory text, mono fonts, particles or decorative animation.

## Font

The prototype uses **Segoe UI Variable Text**, matching the current web product's Windows typography while avoiding a bundled font payload.

## Requirements

- Qt 6.8 or newer (Desktop MSVC kit)
- Qt Quick + Qt Quick Controls 2
- CMake 3.21+
- Visual Studio with Desktop development with C++ / MSVC
- Ninja is optional; the smart builder falls back to NMake automatically

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

The prototype opens on the Music workspace and follows the current operator layout: PEQ and selected-band inspector above Music Input, Pitch/Tone, filters and Master Strip. Controls are interactive, but nothing is transmitted to hardware yet.

Visible Music controls are backed by the native `StudioEngine` QObject. PEQ bands use a `QAbstractListModel`; faders, pitch/tone, crossover frequencies and filter-type selectors write canonical store-style paths through the engine's `stateEdited(path, value)` signal. This establishes the boundary for preset parsing and K500 transport wiring without coupling QML controls directly to device I/O.

## Native control behavior

- Point at a PEQ graph and scroll to adjust the selected band's Q. Hold Shift for fine steps.
- Drag a PEQ node for frequency/gain; hold Shift for surgical movement.
- Hold Ctrl while dragging a PEQ node vertically to adjust Q.
- Scroll knobs, faders, value fields and sliders to change values; hold Shift for fine steps.
- Use arrow keys after focusing a control. Home resets knobs, faders and value fields.
- Double-click a knob, fader, value field or slider to reset it.

## Next phase after visual approval

1. Extract visual/state models from mock properties.
2. Port K500 frame/command/parser logic to C++.
3. Add Bluetooth serial and USB HID transports.
4. Coalesce high-rate UI edits before hardware transmission.
5. Replace mock meter input with KTV telemetry when the new device protocol exposes it.
