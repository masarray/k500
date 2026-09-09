# SonKuPik K500 v1 — User Guide

This guide covers the public stable Windows workflow for SonKuPik K500.

## Supported environment

**Hardware-qualified v1.0 scope:** Windows 10/11 x64 + K500 over USB HID.

Bluetooth SPP is available in the application but remains experimental until independently qualified. For permanent Save/Upload/Mass Upload, use the supported USB workflow.

## Install or run portable

Download from the official GitHub Release page:

- `SonKuPik-K500-v1.0.0-Windows-Setup.exe` — normal installation.
- `SonKuPik-K500-v1.0.0-Windows-Portable.zip` — extract and run `SonKuPik-K500.exe`.

The binaries are currently unsigned open-source builds, so Windows may display a reputation warning. Verify the artifact against `SHA256SUMS.txt` and `release-manifest.json` from the same release.

## Connect to K500

1. Close manufacturer software or other tools that may already own the K500 transport.
2. Connect the K500 by USB.
3. Open SonKuPik K500.
4. Select/connect the USB device.
5. Wait for the application to complete synchronization.
6. Begin LIVE editing only after the device is ONLINE/LIVE.

The application reads the complete K500 active state before LIVE editing. The values shown after connect are hardware truth, not startup defaults.

## Editing the connected K500

Use the processor sections for Music, Mic, Reverb, Echo, Main, Surround, Center, and Sub. Verified controls are routed to the device through the native backend.

Some visible values intentionally remain read-only where no proven K500 write packet exists. This is a safety feature, not a missing “force write” option.

Persistent LCD/Equipment Mode Name rename is currently read-only. Do not expect editing a PC preset name to rename the hardware LCD slot label.

## Device Mode vs PC Presets

The System workspace separates two authorities:

### Device Mode

Represents the actual K500:

- active slot;
- hardware mode names;
- recalled state;
- device-side Save/Recall operations.

### PC Presets

Represents `.k500` files staged on the computer. Selecting one does **not** change the K500.

This separation prevents an accidental file selection from changing live audio.

## Official SonKuPik presets

The application contains bundled official presets, so the library works offline.

Use **Sync** to check the official project preset source for validated updates. A successful update is cached locally. If the network is unavailable or a downloaded file fails K500 validation, the app keeps using the last-known-good cached/bundled copy.

Official sync never overwrites your Local preset folder.

## Local presets

Choose a Local preset folder in System/Mass Upload. Valid `.k500` files from that folder appear in the same collection as official presets, with a distinct LOCAL source label.

Your Local files remain user-owned and writable. Use **Save As** when you want to create another preset file without replacing the original source.

## Preview and offline editing

Selecting a PC preset stages it only. To inspect it in the editor, use **Preview**.

Preview creates an explicit offline preset editing session. Supported controlled edits are tracked against the staged `.k500` document. The UI can show that the preset is edited and how many bytes changed.

Important behavior:

- selecting another preset resets the current edit session;
- connecting a K500 restores hardware truth as the editor authority;
- LIVE device edits do not silently write into a staged PC file;
- Save As is atomic and preserves unknown/reserved preset bytes outside the proven edit whitelist.

## Single preset Upload

Use Upload when one staged PC preset should permanently replace one K500 slot.

1. Select a valid SONKUPIK or LOCAL preset.
2. Select the intended destination slot.
3. Confirm the K500 is connected over the supported USB path.
4. Start Upload.
5. Wait for Store to finish.
6. The app activates/recalls the destination slot and performs a full device refresh.
7. Resume editing only after LIVE returns.

Do not disconnect USB during a permanent transaction. If transport becomes uncertain, the application fails closed and requires a clean reconnect/readback.

## Mass Upload

Mass Upload is a review-before-write workflow.

### Left: PC Preset Collection

Contains validated presets from:

- **SONKUPIK** official bundled/cached presets;
- **LOCAL** user presets.

You may mix both sources.

### Right: K500 Device Slots

Build an explicit Slot 01…10 transfer list using Add / Add All / Remove / Clear. At most ten destination slots can be staged.

The right-side list is shown in natural ascending order, but the proven hardware transaction executes selected slots from highest to lowest. A full bank is written:

```text
Slot 10 -> 09 -> 08 -> ... -> 01
```

After the final Slot 01 commit, the app recalls Slot 01 and performs a complete hardware readback before LIVE returns.

The entire selected batch is validated before the first device write. One invalid member aborts the batch before permanent Store begins.

## Mode 01

The stable official Mode 01 uses the exact native K500 `CONCERT HIFI V4` donor. This replaced an earlier reconstructed preset after physical hardware testing found that the old Mode 01 could leave the music path silent after Mass Upload.

If you previously cached an older official Mode 01, use **Sync Official Presets** before uploading it.

## If something goes wrong

### Device goes offline

Reconnect USB and allow the full synchronization to complete. Do not assume the old editor state is still authoritative.

### A section freezes/closes the app

v1.0 contains a fix and runtime regression test for the earlier Mic/Reverb section-switch crash. If a crash still reproduces, record the exact version, sequence of clicks, connection state, and Support Report/trace if available, then file a bug report.

### Upload/Mass Upload interrupted

Reconnect before doing anything else. Let the application read the complete K500 state. Never assume a partially interrupted permanent transaction succeeded.

### Official Sync fails

You can continue using bundled/last-known-good official presets and Local presets. Do not manually replace cache files with unvalidated data.

## Support Report

Use **Support** in the top toolbar to save a JSON diagnostic report.

The report includes useful build/runtime and protocol metadata while deliberately excluding:

- the complete 939-byte active-memory image;
- `.k500` preset payload bytes;
- Local preset paths.

Attach the report to a reproducible GitHub issue when appropriate.

## Verify a release

PowerShell example:

```powershell
Get-FileHash .\SonKuPik-K500-v1.0.0-Windows-Setup.exe -Algorithm SHA256
Get-FileHash .\SonKuPik-K500-v1.0.0-Windows-Portable.zip -Algorithm SHA256
```

Compare the output with `SHA256SUMS.txt` from the same GitHub release tag.

## More documentation

- [Architecture](ARCHITECTURE.md)
- [Release Model](RELEASES.md)
- [Capability & Protocol Parity Matrix](PORTING_PARITY_MATRIX.md)
- [Hardware Acceptance Checklist](HARDWARE_ACCEPTANCE_CHECKLIST.md)
- [Windows Distribution Security](WINDOWS_DISTRIBUTION_SECURITY.md)
