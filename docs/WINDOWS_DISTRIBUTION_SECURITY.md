# Windows Distribution & Antivirus Policy

SonKuPik K500 is a GPL-3.0-or-later open-source application. The official Windows pipeline is designed for transparent, reproducible release provenance and to avoid packaging patterns that can unnecessarily increase heuristic antivirus detections.

## Stable distribution formats

The v1 stable line ships two Windows x64 packages:

- **Installer:** standard Inno Setup 6 executable.
- **Portable:** ordinary ZIP archive containing the deployed Qt application.

The project intentionally does **not** ship a custom self-extracting portable executable. An earlier RC used a temporary extraction/launcher wrapper; although legitimate, that behavior resembles patterns used by droppers and can attract additional heuristic scrutiny. It is retired from the stable distribution model.

The normal installer uses the canonical machine location `C:\Program Files\SonKuPik K500`. Personal `.k500` files live separately under `Documents\SonKuPik K500\Presets` by default and are not owned by the uninstaller.

## Current signing status

Official v1.0 Windows packages are **unsigned open-source builds**. Release metadata records:

```text
codeSigning = unsigned-open-source
```

Do not interpret an installer icon, GitHub release, successful CI build, or SHA-256 verification as Authenticode publisher signing. Windows SmartScreen and third-party antivirus products may still display reputation warnings for a new unsigned binary.

The project does not require users to disable security software or add broad exclusions.

## Release provenance

Every public stable release includes:

- exact Git commit SHA in `release-manifest.json`;
- SHA-256 for Setup and Portable packages;
- exact artifact byte sizes;
- target architecture and Qt runtime version;
- hardware-qualified support scope;
- signing status;
- release-channel metadata;
- official Mode 01 native-donor SHA;
- regression-suite summary.

Use the metadata published with the exact GitHub release tag rather than copying hashes from third-party mirrors.

## In-app update security boundary

Starting with v1.0.2, the Windows app may discover and install a newer public stable release without sending the user to a browser. The updater is deliberately fail-closed:

1. only the latest non-draft, non-prerelease release is considered;
2. the expected Setup filename must exactly match the discovered semantic version;
3. `release-manifest.json` must identify `SonKuPik K500`, channel `stable`, target `windows-x64`, `stableReleaseEligible=true`, and `Inno Setup 6`;
4. the Setup artifact filename, byte size, and SHA-256 in the manifest must match the GitHub release asset metadata;
5. `SHA256SUMS.txt` must independently contain the same SHA-256 for that exact Setup filename;
6. the downloaded file is streamed atomically to internal LocalAppData staging, checked for the expected byte size and Windows executable header, then hashed locally;
7. any mismatch aborts execution and removes the rejected package;
8. the installer is launched with the normal Windows `runas` elevation verb, so UAC remains visible and user-controlled;
9. after a successful update install, Setup reopens the new Program Files binary.

The updater accepts only HTTPS GitHub API/release-asset endpoints and checks the final redirected asset host before using downloaded metadata/package bytes. Network failure is non-fatal to K500 control.

Integrity verification protects against corruption or a package that does not match the repository release metadata. Because the current build is not Authenticode-signed, it does **not** provide an independent cryptographic publisher identity if the repository/release account itself were compromised. Future code signing can strengthen that trust boundary without changing the current fail-closed updater design.

## Verification workflow

When validating a downloaded release manually:

1. download from the project's GitHub Release page;
2. download `SHA256SUMS.txt` and `release-manifest.json` from the same tag;
3. compute SHA-256 locally;
4. compare the exact filename/hash;
5. confirm the manifest version, release commit, target, and support scope.

Example PowerShell:

```powershell
Get-FileHash .\SonKuPik-K500-vX.Y.Z-Windows-Setup.exe -Algorithm SHA256
Get-FileHash .\SonKuPik-K500-vX.Y.Z-Windows-Portable.zip -Algorithm SHA256
```

## Qt runtime policy

The release pipeline validates a current supported Qt runtime rather than remaining indefinitely on the original development kit. The v1 stable line is packaged and runtime-tested against the release pipeline's pinned Qt version (currently **Qt 6.10.2**).

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
