# Windows distribution and antivirus policy

SonKuPik K500 is a GPL-3.0-or-later open-source application. The official Windows release pipeline is designed to minimize heuristic antivirus false positives without requiring a commercial code-signing certificate.

## Distribution formats

Official release candidates use:

- **Installer:** standard Inno Setup 6 installer.
- **Portable:** ordinary ZIP archive containing the deployed Qt application.

The project intentionally does **not** ship a custom self-extracting portable executable. Earlier RC packaging extracted a Qt runtime to a temporary folder, launched another executable, and removed the temporary runtime after exit. Although that mechanism was legitimate, the behavior resembles patterns used by droppers and can increase heuristic antivirus detections.

## Open-source unsigned status

Release manifests explicitly record `codeSigning: unsigned-open-source` until an open-source signing service is adopted. No release workflow may imply that an unsigned artifact is Authenticode-signed.

Every release includes:

- exact Git commit SHA;
- SHA-256 hashes for installer and portable archive;
- machine-readable `release-manifest.json`;
- Qt runtime version;
- build/runtime regression evidence.

## Qt runtime policy

The Windows release workflow tracks a current public Qt release rather than relying indefinitely on the original Qt 6.8.3 development runtime. This is security-maintenance hygiene and is independent of antivirus reputation.

## False-positive handling

If an official artifact is detected heuristically:

1. Verify its SHA-256 hash against `SHA256SUMS.txt`.
2. Confirm the hash/commit in `release-manifest.json`.
3. Reproduce the detection with the ordinary portable ZIP/direct executable, not the retired self-extracting wrapper.
4. Submit the exact official artifact to the antivirus vendor for false-positive/reputation re-analysis.

Do not weaken application behavior, protocol safety checks, or malware protection exclusions merely to make a heuristic warning disappear.

## Optional future open-source signing

Commercial signing is not required by this project. If maintainers later want Authenticode reputation without purchasing a commercial certificate, an eligible open-source signing program may be integrated as a separate provenance-controlled step. Until then, the release pipeline remains explicitly unsigned and transparent.
