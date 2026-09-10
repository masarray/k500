# SonKuPik K500 Release Model

This document defines how application releases and Official Preset updates are published and supported.

## Channels

### Stable

Stable releases are intended for general use within the explicitly documented hardware-qualified scope.

Current stable line: **v1.0.x**.

Current qualified scope: **Windows 10/11 x64 + USB HID**.

Bluetooth SPP is implemented but experimental until independent physical acceptance is recorded.

### Prerelease / RC

Prereleases are used when a change needs wider testing before becoming part of the stable support contract. A prerelease may have complete software CI while still awaiting physical hardware evidence.

## Versioning

Application binaries follow semantic versioning:

```text
MAJOR.MINOR.PATCH
```

- **PATCH** — compatible bug fixes, documentation/release hardening, narrowly compatible behavior corrections.
- **MINOR** — compatible feature expansion or meaningful workflow changes.
- **MAJOR** — support/architecture/API expectations that intentionally reset compatibility expectations.

Official presets can update independently through the in-app sync path and therefore do not require an application version bump for every validated preset revision.

## v1.0.0 stable record

- Tag: `v1.0.0`
- Release commit: `a6c10847597616dbd22ba0271d82f5814991cb3e`
- Qt: `6.10.2`
- Setup SHA-256: `5fb9458d8b56f047b0304a651c13aeade032ea8a250b0142b6a1c0c2134a7345`
- Portable SHA-256: `5e196c09fc75033f5f44532916b03aa3ff521510606470f46787099d72f00fe4`

The GitHub release remains the canonical source for downloadable artifacts and machine-readable provenance.

## Windows artifacts

A stable release publishes:

```text
SonKuPik-K500-v<version>-Windows-Setup.exe
SonKuPik-K500-v<version>-Windows-Portable.zip
SHA256SUMS.txt
release-manifest.json
```

The installer is standard Inno Setup. The portable package is an ordinary ZIP. Stable releases do not use the retired custom self-extracting portable wrapper.

## Release manifest

`release-manifest.json` records release facts including:

- product/channel/version;
- exact commit;
- Windows target and Qt version;
- hardware acceptance/support scope;
- Bluetooth qualification state;
- signing status;
- packaging technology;
- Official Preset source;
- native Mode 01 golden SHA;
- artifact hashes;
- regression-suite summary.

Use the manifest and `SHA256SUMS.txt` together when verifying a binary.

## Stable publishing gate

The stable workflow must pass before GitHub publishes a non-prerelease tag. Current gates include:

- stable version/support truth;
- exact native Mode 01 hash;
- clean Qt/MSVC build;
- preset protocol/codec/persistence/batch regressions;
- direct deployed-app runtime tests;
- section-navigation runtime stress tests;
- branded Inno packaging;
- portable ZIP runtime validation;
- actual silent install + installed-app runtime validation;
- manifest and artifact hash validation.

CI can verify software behavior; hardware-facing support claims still require real-device evidence appropriate to the changed path.

## Official Preset update channel

Official presets live in:

```text
resources/presets/
```

The application includes a bundled copy for offline use and can check the official repository source for newer/changed files.

Remote update rules:

1. discover official preset files;
2. download asynchronously;
3. validate exact K500 format/checksum;
4. promote only valid files to the official cache;
5. preserve last-known-good cache/bundled fallback when validation/network fails;
6. never overwrite the Local user preset folder.

This allows a validated preset correction to reach users without forcing a complete application reinstall.

## Stable application vs mutable official data

A stable application binary can consume a later validated Official Preset revision. Therefore bug reports involving presets should record both:

- application version;
- exact preset name/hash when relevant.

For the v1 baseline, Mode 01's rollback authority is:

```text
CONCERT HIFI V4
SHA-256 9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

## Signing policy

Current Windows artifacts are unsigned open-source builds. The release manifest states that explicitly. If code signing is adopted later, the release workflow and documentation must change together; the project must never imply a signature that does not exist.

## Post-release development

The release tag is immutable historical provenance even when `main` continues with documentation or future work.

Recommended flow:

```text
stable tag
  remains fixed

main
  -> focused branch
  -> PR
  -> exact-head CI
  -> hardware validation if required
  -> merge
  -> next version/release when appropriate
```

Do not republish a stable tag merely because documentation on `main` changed.

## User verification

Download only from the project's GitHub Releases page and verify SHA-256 when provenance matters. See [Windows Distribution & Antivirus Policy](WINDOWS_DISTRIBUTION_SECURITY.md).
