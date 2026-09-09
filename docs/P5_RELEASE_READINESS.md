# v1.0 Stable Release Qualification

> **Status:** `v1.0.0` is the first public stable SonKuPik K500 release.
>
> **Hardware-qualified scope:** Windows 10/11 x64 + K500 over USB HID. Bluetooth SPP remains implemented but experimental and is not part of the v1.0 hardware-qualified support claim.

## Published stable checkpoint

| Item | v1.0.0 record |
|---|---|
| Tag | `v1.0.0` |
| Release commit | `a6c10847597616dbd22ba0271d82f5814991cb3e` |
| Qt release runtime | `6.10.2` |
| Windows target | x64 |
| Stable release | non-draft, non-prerelease |
| Installer | `SonKuPik-K500-v1.0.0-Windows-Setup.exe` |
| Portable | `SonKuPik-K500-v1.0.0-Windows-Portable.zip` |
| Setup SHA-256 | `5fb9458d8b56f047b0304a651c13aeade032ea8a250b0142b6a1c0c2134a7345` |
| Portable SHA-256 | `5e196c09fc75033f5f44532916b03aa3ff521510606470f46787099d72f00fe4` |

The release also publishes `SHA256SUMS.txt` and `release-manifest.json` with exact build provenance.

## What reached stable

The v1.0 stable baseline includes:

- native Qt 6 / QML Windows application;
- USB HID connect, heartbeat/handshake, and authoritative 939-byte device hydration;
- fail-closed LIVE/device transaction architecture;
- donor-verified live command families;
- Equipment Mode Recall and Use Init Volume transaction handling;
- permanent current-device Save;
- `.k500` bit-preserving parser/codec and controlled edit persistence;
- single PC preset Upload;
- deterministic Mass Upload using the native descending Store order;
- unified **SONKUPIK + LOCAL** preset collection;
- bundled official preset fallback plus validated GitHub-backed official preset cache/sync;
- exact native Mode 01 `CONCERT HIFI V4` donor;
- crash-resistant fixed EQ-page/model lifetime across processor sections;
- bounded/redacted Support Report diagnostics;
- branded Inno Setup installer and ordinary portable ZIP.

## Mode 01 native donor

The official Mode 01 file is preserved byte-for-byte:

```text
resources/presets/01_ALL_GENRE.k500
internal name: CONCERT HIFI V4
SHA-256: 9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

This replaced an earlier reconstructed Mode 01 after physical hardware testing exposed a music-output failure. Stable CI now treats native donor identity as an explicit regression contract.

## Stable support boundary

### Qualified in v1.0

- Windows 10/11 x64;
- USB HID K500 connection (`VID:PID 10C4:0321`);
- full device hydration before LIVE;
- verified live command families documented in the parity matrix;
- Equipment Mode Recall with full resync;
- Use Init Volume;
- current-device permanent Save;
- PC `.k500` single-slot Upload;
- 1–10 preset Mass Upload with final Slot 01 recall/readback;
- Official/Local preset library and validated remote official updates.

### Deliberately outside the qualified claim

- **Bluetooth SPP:** implemented, but independent hardware qualification is still required.
- **Persistent LCD/Equipment Mode rename:** readback exists; persistent write remains disabled until a donor-verified native rename transaction is captured.
- Any UI field for which no verified hardware write protocol exists.

Stable does not mean guessed protocol behavior is enabled.

## Release gates

Every stable Windows package must pass from the exact release commit:

1. semantic version/release truth guard;
2. exact native Mode 01 SHA-256 guard;
3. clean Qt/MSVC build;
4. P0/P1 architecture and protocol guards;
5. P2 preset protocol self-test;
6. P3 synthetic codec and P3.2 donor corpus tests;
7. P3.4 controlled edit-persistence test;
8. P4/P4.2 upload/batch regressions;
9. runtime font, protocol/RX, StudioEngine, and section-navigation stress tests;
10. direct deployed-app runtime test;
11. portable ZIP extraction + runtime tests;
12. Inno Setup build + real silent installation + installed-app runtime tests;
13. artifact SHA-256 generation;
14. machine-readable stable manifest validation;
15. GitHub release publish as `prerelease: false`.

A failed gate blocks publishing.

## Distribution and signing

SonKuPik K500 is GPL-3.0-or-later open-source software. The v1.0 Windows binaries are currently **unsigned**, so SmartScreen or antivirus reputation systems may warn on a new binary. The project does not claim Authenticode signing when none exists.

The official distribution formats are:

- standard **Inno Setup 6** installer;
- ordinary **ZIP** portable package.

The retired custom self-extracting portable executable is not part of stable distribution.

## Ongoing release discipline

`v1.0.0` is a frozen public baseline. Future changes should land through focused branches/PRs and preserve exact-head CI evidence.

A hardware/protocol change must be physically revalidated when it can alter destructive device behavior. A documentation-only change must not silently broaden support claims. Bluetooth qualification must remain independent from the USB stable claim until evidence exists.

See also:

- [Release Model](RELEASES.md)
- [Hardware Acceptance Checklist](HARDWARE_ACCEPTANCE_CHECKLIST.md)
- [Capability & Protocol Parity Matrix](PORTING_PARITY_MATRIX.md)
- [Windows Distribution Security](WINDOWS_DISTRIBUTION_SECURITY.md)
