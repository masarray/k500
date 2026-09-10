# Windows Distribution & Antivirus Policy

SonKuPik K500 is a GPL-3.0-or-later open-source application. The official Windows pipeline is designed for transparent, reproducible release provenance and to avoid packaging patterns that can unnecessarily increase heuristic antivirus detections.

## Stable distribution formats

The v1 stable line ships two Windows x64 packages:

- **Installer:** standard Inno Setup 6 executable.
- **Portable:** ordinary ZIP archive containing the deployed Qt application.

The project intentionally does **not** ship a custom self-extracting portable executable. An earlier RC used a temporary extraction/launcher wrapper; although legitimate, that behavior resembles patterns used by droppers and can attract additional heuristic scrutiny. It is retired from the stable distribution model.

## Current signing status

Official v1.0 Windows packages are **unsigned open-source builds**. Release metadata records:

```text
codeSigning = unsigned-open-source
```

Do not interpret an installer icon, GitHub release, or successful CI build as Authenticode signing. Windows SmartScreen and third-party antivirus products may still display reputation warnings for a new unsigned binary.

The project does not require users to disable security software or add broad exclusions.

## Release provenance

Every public stable release includes:

- exact Git commit SHA in `release-manifest.json`;
- SHA-256 for Setup and Portable packages;
- target architecture and Qt runtime version;
- hardware-qualified support scope;
- signing status;
- release-channel metadata;
- official Mode 01 native-donor SHA;
- regression-suite summary.

For v1.0.0, use the metadata published with the GitHub release rather than copying hashes from third-party mirrors.

## Verification workflow

When validating a downloaded release:

1. download from the project's GitHub Release page;
2. download `SHA256SUMS.txt` and `release-manifest.json` from the same tag;
3. compute SHA-256 locally;
4. compare the exact filename/hash;
5. confirm the manifest version, release commit, target, and support scope.

Example PowerShell:

```powershell
Get-FileHash .\SonKuPik-K500-v1.0.0-Windows-Setup.exe -Algorithm SHA256
Get-FileHash .\SonKuPik-K500-v1.0.0-Windows-Portable.zip -Algorithm SHA256
```

## Qt runtime policy

The release pipeline validates a current supported Qt runtime rather than remaining indefinitely on the original development kit. v1.0.0 was packaged and runtime-tested with **Qt 6.10.2**.

A future Qt upgrade must still pass the same protocol, preset, direct-deployment, portable, installed-app, and section-navigation regression gates before release.

## Antivirus false-positive handling

If an official artifact is detected heuristically:

1. verify its SHA-256 against official release metadata;
2. confirm the release commit/channel in `release-manifest.json`;
3. reproduce with the standard package, not a third-party repack;
4. record the antivirus product, engine/database version, exact detection name, and hash;
5. submit the exact official artifact to the antivirus vendor for false-positive/reputation review when appropriate;
6. open a project issue only if there is reproducible evidence useful to the project.

Do not weaken device safety checks, protocol validation, preset integrity, or operating-system security controls merely to suppress a heuristic warning.

## Optional future signing

The project may later adopt an eligible open-source signing service or other provenance mechanism. Such a change must be explicit in the release workflow and manifest. Until then, the correct public statement is **unsigned open-source Windows distribution**.

## Download policy

The canonical binary distribution is the repository's GitHub Releases page. Avoid redistributing modified installers under the same filenames because users rely on release hashes and manifest provenance to distinguish official artifacts.
