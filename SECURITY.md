# Security Policy

SonKuPik K500 controls real audio-processing hardware and includes permanent device-write operations. Protocol integrity, preset parsing, remote preset updates, and destructive Store/Upload transactions are treated as safety-sensitive surfaces.

## Supported versions

| Version | Status |
|---|---|
| `1.0.x` | Supported stable line |
| `< 1.0` | Historical release candidates; upgrade recommended |
| `main` | Development baseline; support depends on current CI state |

The v1.0 hardware-qualified support scope is Windows 10/11 x64 + USB HID. Bluetooth SPP is implemented but remains experimental until independently accepted.

## Reporting a vulnerability

Do **not** publish credentials, private device captures containing unrelated personal data, or sensitive local paths in a public issue.

For ordinary reproducible crashes, parser failures, connection failures, preset-sync failures, or device-write regressions, use the repository bug-report template and attach a Support Report JSON when practical. The Support Report intentionally excludes active-memory contents, preset payload bytes, and local preset file paths.

For a vulnerability that should remain private until a fix exists, use GitHub private vulnerability reporting / Security Advisory when enabled for the repository.

Please include:

- affected SonKuPik K500 version and commit when known;
- Windows version;
- USB HID or Bluetooth SPP transport;
- minimum reproducible steps;
- whether permanent device storage was touched;
- expected vs actual behavior;
- relevant Support Report / trace with unnecessary private data removed.

## Hardware-write safety policy

A change must not:

- invent or guess an unverified K500 command, offset, ACK, or persistent Mode Name transaction;
- replace device-owned unknown bytes with editor defaults;
- enable LIVE before authoritative hydration completes;
- bypass `K500DeviceManager` to own a second transport handle;
- weaken established regression checks merely to make new code pass;
- report a destructive Store/Upload transaction as successful after uncertain transport state;
- reintroduce model-lifetime patterns known to cause section-navigation heap/UI crashes.

Unknown protocol behavior must remain read-only or unsupported until donor/capture evidence exists.

## Remote Official Preset security

Official preset sync is intentionally narrower than arbitrary remote file download:

- the source is the project's official preset repository path;
- downloaded entries must be valid `.k500` containers before cache promotion;
- invalid/failed updates do not replace the last-known-good official cache;
- bundled presets remain an offline fallback;
- official sync never overwrites the user's Local preset folder.

Preset checksum validation is an integrity-format check, not a cryptographic authenticity claim. Release binaries and published artifacts should be verified using the SHA-256 values in `SHA256SUMS.txt` / `release-manifest.json`.

## Windows binary signing

Official v1.0 Windows binaries are currently **unsigned open-source builds**. SmartScreen or antivirus reputation warnings may therefore occur. Do not disable security software as a project requirement. Verify the downloaded artifact hash against the release metadata and report reproducible false positives to the relevant vendor.

See [Windows Distribution Security](docs/WINDOWS_DISTRIBUTION_SECURITY.md).

## Release integrity

Public stable releases are built from an exact commit by the stable Windows workflow. Publishing is blocked unless the build, protocol/preset regression suite, direct runtime tests, portable ZIP test, Inno installed-app test, manifest validation, and Mode 01 native-donor guard pass.

See [v1.0 Stable Release Readiness](docs/P5_RELEASE_READINESS.md) and [Hardware Acceptance Checklist](docs/HARDWARE_ACCEPTANCE_CHECKLIST.md).
