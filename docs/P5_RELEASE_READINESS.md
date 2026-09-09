# v1.0 Stable Release Readiness

> Status: **public stable approved by maintainer after physical K500 validation of the current Windows/USB workflow**.
>
> SonKuPik K500 v1.0.0 is the first public stable release. The stable claim covers the tested Windows x64 + USB HID workflow. Bluetooth SPP remains available in the software but is not part of the v1.0 hardware-qualified support claim and should be treated as experimental until independently accepted.

## Stable checkpoint

The v1.0 stable baseline preserves all software gates completed during the RC cycle and includes the hardware fixes accepted on a physical K500:

- crash-proof section navigation across Mic/Reverb/Echo/Main/Surround/Center/Sub/System;
- authoritative 939-byte device readback and hardware-truth slot state;
- permanent single-preset Upload and deterministic Mass Upload transaction flow;
- unified Official + Local PC preset library;
- remote Official Preset sync with validation and last-known-good cache;
- native donor Mode 01 `CONCERT HIFI V4`, replacing the earlier reconstructed blob;
- branded Inno Setup installer and ordinary portable ZIP;
- complete Windows runtime regression suite and installed-app/portable self-tests.

The official Mode 01 donor is intentionally preserved byte-for-byte and guarded by SHA-256:

`9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74`

## Stable support boundary

### Qualified for v1.0

- Windows 10/11 x64.
- USB HID K500 connection (`VID:PID 10C4:0321`).
- Full device hydration before LIVE editing.
- Verified live command families already present in the parity matrix.
- Equipment Mode Recall with full resync.
- Use Init Volume transaction.
- Current-device Save through the proven Store path.
- PC `.k500` single-slot Upload.
- 1–10 preset Mass Upload using the proven descending hardware order 10 → 1 followed by Slot 01 recall/readback.
- Official SonKuPik preset library, local user presets, offline cache and remote preset updates.

### Intentionally not claimed as v1.0 hardware-qualified

- Bluetooth SPP transport. It remains implemented but requires independent physical acceptance before it is promoted to the same support level as USB.
- Persistent LCD/Equipment Mode rename. The readback field is available, but writing remains disabled until a donor-verified native rename transaction is captured.
- Any hardware command that does not yet have a verified donor/native protocol mapping.

These boundaries are deliberate. Stable does not mean guessed protocol behavior is enabled.

## Software release gates

Every public stable Windows package must pass all of the following from the exact release commit:

1. Clean Qt/MSVC build.
2. P0/P1 architecture and protocol guards.
3. P2 permanent preset protocol self-test.
4. P3 synthetic codec tests.
5. P3.2 donor corpus tests.
6. P3.4 controlled edit-persistence tests.
7. P4/P4.2 single and batch preset guards.
8. Recovered-progress and official-preset-library guards.
9. Runtime font, protocol/RX, StudioEngine and section-navigation stress tests.
10. Portable ZIP extraction + runtime self-tests.
11. Inno Setup installation + installed-app runtime self-tests.
12. SHA-256 generation and release-manifest validation.

A failed gate blocks publishing.

## Release artifacts

The stable Windows release contains:

- `SonKuPik-K500-v1.0.0-Windows-Setup.exe`
- `SonKuPik-K500-v1.0.0-Windows-Portable.zip`
- `SHA256SUMS.txt`
- `release-manifest.json`

The release manifest records the exact packaged commit, target, Qt version, hardware-acceptance scope and artifact hashes.

## Distribution and signing

SonKuPik K500 is free/open-source software. The Windows packages are currently unsigned and therefore Windows SmartScreen or third-party antivirus reputation systems may still warn about a new binary. The installer uses standard Inno Setup 6 and the portable package is a normal ZIP rather than a custom self-extracting executable.

## Ongoing acceptance

Future changes to device protocol, preset conversion, Mass Upload, transport behavior or hardware state truth must preserve the existing regression fortress and be physically revalidated when they change destructive hardware behavior. Stable v1.0 is a baseline, not permission to weaken fail-closed behavior.
