# SonKuPik K500 Release Model

This document defines how application releases and Official Preset updates are published and supported.

## Channels

### Stable

Stable releases are intended for general use within the explicitly documented hardware-qualified scope.

Current public stable: **v1.1.6** (Windows 10/11 x64 + USB HID; no-admin Per-User installer).

Owner-accepted `v1.1.3-rc.6` was promoted byte-identically from application commit `2cfd0060d5c4d192d35347ce1ae34b250d98d7ff`. [Stable Promotion #38406359639](https://github.com/masarray/k500/actions/runs/38406359639) verified all three installer SHA-256 digests and successfully published `v1.1.3` as the GitHub Latest Release. No installer bytes were rebuilt. The site routes resolve this latest non-prerelease release dynamically; v1.1.2 remains a historical stable.

Stable **v1.1.1** was promoted byte-identically from accepted **v1.1.1-rc.1** at exact source commit `df7a6dd3504f4c7ffbf19b12d9e0c2e1b9243b13`. Stable **v1.1.0** remains historical and was promoted byte-identically from accepted **v1.1.0-rc.5** at exact commit `01c545ed27d9a4316fb255765a25ae41f0e1457a`.

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

## v1.1.6 stable — published 2026-11-08 (WIB)

- [Official stable v1.1.6](https://github.com/masarray/k500/releases/tag/v1.1.6) is GitHub Latest. Promoted **without rebuilding** from [owner-accepted v1.1.6-rc.1](V1_1_6_UPDATER_RC_ACCEPTANCE.md) by [comment-triggered workflow #42454469526](https://github.com/masarray/k500/actions/runs/42454469526).
- Exact immutable installer source: `39f9cc926a53900e62f439c44f61878880c6d1d2`; original Per-User installer SHA-256: `937a28a4bb715f60ba7ed06c437bbce165357d519bab824b77fa84c39b284899` (55,615,005 bytes).
- Public aliases: `sonkupik-k500-setup.exe` and `sonkupik-k500-setup.zip`; ZIP contains `SonKuPik-K500-v1.1.6-Windows-Setup.exe` with the **identical** installer bytes. Versioned `SonKuPik-K500-v1.1.6-Windows-Setup-PerUser.exe` remains published for installed updater compatibility.
- The app ships **51 bundled PC presets**: approved FOCUS BT Music R6 01–10; previous GEN5 UAUDIO 11–20; GEN3 BT 21–30; derived GEN3 UAUDIO 31–40; protected original GOLDEN 41; legacy BT 42–51. The real device still has 10 slots, and installation itself does not upload or overwrite device presets.
- Windows updater adds guarded recovery snapshots/rollback, a second-GUI-instance preflight and clearer USB HID errors. Owner tested RC and approved stable; detailed individual fault and USB readback field logs remain separate evidence. Windows 10/11 x64 + USB HID is the supported platform/transport; Bluetooth SPP remains experimental. No stable macOS/Linux assets are published for v1.1.6.
- [Smart development CI](SMART_CI_COMMENT_WORKFLOW.md) runs cheap Fast Contracts automatically and targeted native tests by authorized comment. Stable approval still requires an independent exact-SHA RC and explicit owner release comment, not an automatic push-triggered native build.

## v1.1.5 stable — historical release published 2026-11-05 (WIB)

- [Official stable](https://github.com/masarray/k500/releases/tag/v1.1.5) was promoted from [owner-accepted v1.1.5-rc.1](V1_1_5_UPDATER_RC_ACCEPTANCE.md) by [single-pass CI #41911424715](https://github.com/masarray/k500/actions/runs/41911424715), without recompiling Qt/Inno.
- Exact RC source commit: `89a1e59535c891190d226b9eb636a5c967dd4c73`. Verified installer SHA-256: `72651326cd871ae3278b92b713b2bb9db03dd3761970fab2e7ec0d5bba62cb3f`.
- User fixes: adaptive short/high-DPI console, reachable System controls, faders and QRIS dismissal.
- Public `sonkupik-k500-setup.zip` contains `SonKuPik-K500-v1.1.5-Windows-Setup.exe`; public direct EXE is `sonkupik-k500-setup.exe`. Both have byte-identical Per-User binary contents and do not normally request UAC.
- Versioned `-Windows-Setup-PerUser.exe` remains in GitHub Release for previously shipped updater clients. Do not publish a top-level versioned `-Windows-Setup.exe` asset: old machine-scope clients misclassify that filename.
- Windows x64 USB HID is hardware-qualified; Bluetooth SPP remains experimental. No stable macOS/Linux packages for v1.1.5. Prior release assets remain immutable.
- Release aliases and checksum files are produced by the existing stable promotion job, not a separate runner or build.

## v1.1.3 stable record — 2026-10-13

- Stable tag: [`v1.1.3`](https://github.com/masarray/k500/releases/tag/v1.1.3) (**Latest Release**, non-prerelease).
- Accepted immutable candidate: [`v1.1.3-rc.6`](https://github.com/masarray/k500/releases/tag/v1.1.3-rc.6).
- Exact application source SHA: `2cfd0060d5c4d192d35347ce1ae34b250d98d7ff`.
- Owner acceptance and immutable fingerprints: [v1.1.3 RC Acceptance](V1_1_3_UPDATER_RC_ACCEPTANCE.md).
- Machine Setup SHA-256: `282b0d5111879786c8db6c21e3542e81d43ec6cc475d130d12356950c494cde6`.
- Per-user Setup SHA-256: `6f9af6dcf5b8fcf4540ee417f322873dd2ecdd33cd9c7686f21a44b71ed9cc14`.
- Public Smart Installer SHA-256: `760531e9ce9bf741db5d39f589c599101e2b950b2bad04c81e12e52191cea680`.
- [Stable Promotion #38406359639](https://github.com/masarray/k500/actions/runs/38406359639) completed successfully after the promotion-specific Smart no-override reinstall correction. All three stable packages match the RC.6 hashes and sizes exactly; no rebuild, no retagging of historical releases.
- Normal public Smart installation targets `%LOCALAPPDATA%\Programs\SonKuPik K500` without UAC. Migration from a registered machine-wide copy requires an explicit guided handoff and an authorized UAC for protected old files.
- Reverb/Echo HPF/LPF are native always-on frequency-only filters; their PEQ visualization is screenshot-calibrated, not a measured claim about DSP coefficients. Other type-capable sections preserve existing Type/Bypass handling.
- Bluetooth SPP is experimental. Public hardware-qualified transport is **USB HID on Windows x64**.
- Prior RC.1 scope error and the resolved first stable-promotion workflow failure are preserved in CI history and the [release gate](https://github.com/masarray/k500/issues/202); they do not change these accepted final package bytes.

## v1.1.2 stable record

- Stable tag: `v1.1.2`
- Accepted RC: `v1.1.2-rc.2`
- Exact binary source SHA: `8e3be5d4e567463e5bae4aa911537417aeba11b1`
- Acceptance merge commit: `7a67acd674f70fb0c30ff37fb2dd72b84e468ce1`
- Machine Setup SHA-256: `decd7d8e54837d664fa586a7425b502faeec27dee280d9dc48f6f6989a23b8ab`
- Per-user Setup SHA-256: `f9a1f29a9b209f47ffeb4f289b0d28b38a9df10764927dcef304397c65b3171b`
- [Stable promotion run #38046752955](https://github.com/masarray/k500/actions/runs/38046752955) succeeded on 2026-10-10, revalidated both installers and published **byte-identical** assets without rebuild.
- [GitHub Latest release](https://github.com/masarray/k500/releases/tag/v1.1.2) includes both setup EXEs, checksums and release manifest; Bluetooth SPP remains experimental.

## v1.1.1 stable record

- Stable tag: `v1.1.1`
- Accepted RC: `v1.1.1-rc.1`
- Exact binary source commit: `df7a6dd3504f4c7ffbf19b12d9e0c2e1b9243b13`
- Acceptance metadata merge: `ab32ee9d7560fc88926b255852345644b4dffdc4`
- Machine Setup SHA-256: `122689e86e5624e7ce251108b9551f98216b62fcab976ea1d912c400c0d83b03`
- Per-user Setup SHA-256: `0387799511a2c05269f536112ec4a17d12d05487814ba16cce4a9774d4f8b389`
- Publication: byte-identical promotion; no installer rebuild after RC acceptance
- Public latest channel: GitHub `/releases/latest` and the SonKuPik landing download route

The v1.1.1 binary includes the accepted current-main Mode 01/02 presets, premium System toggle UX, capture-backed VR OFF corrections, disconnect/offline handoff, semantic preset hardening, and consolidated regression closure.

## v1.0.0 stable record

- Tag: `v1.0.0`
- Release commit: `a6c10847597616dbd22ba0271d82f5814991cb3e`
- Qt: `6.10.2`
- Setup SHA-256: `5fb9458d8b56f047b0304a651c13aeade032ea8a250b0142b6a1c0c2134a7345`
- Portable SHA-256: `5e196c09fc75033f5f44532916b03aa3ff521510606470f46787099d72f00fe4`

The GitHub release remains the canonical source for downloadable artifacts and machine-readable provenance.

## Windows artifacts

For the **current v1.1.6 stable** the exact published assets are:

```text
sonkupik-k500-setup.exe
sonkupik-k500-setup.exe.sha256
sonkupik-k500-setup.zip
sonkupik-k500-setup.zip.sha256
SonKuPik-K500-v1.1.6-Windows-Setup-PerUser.exe
SHA256SUMS.txt
release-manifest.json
```

The ZIP is an **installer archive, not a portable edition**. It contains the
same Per-User EXE bytes under the friendly name `SonKuPik-K500-v1.1.6-Windows-Setup.exe`.
There is no newly published machine-wide or Smart Installer binary. Historical
v1.1.2/v1.1.3 packages remain immutable and may retain their old installer
variants. Fresh user installs go to LocalAppData without routine UAC; older
Program Files installations require a separately approved scope migration.

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
- current official Mode 01 preset SHA (stored in the legacy-compatible `mode01NativeSha256` manifest field);
- artifact hashes;
- regression-suite summary.

Use the manifest and `SHA256SUMS.txt` together when verifying a binary.

## Stable publishing gate

The public stable workflow is an **owner-comment approval of an immutable RC**, not an automatic rebuild after every push.

- A separately tagged RC is built once from an exact source commit and passes its Windows/Qt/Inno and updater lifecycle qualification.
- The owner tests the RC and records the accepted tag, full source commit and 64-hex SHA-256 in the versioned release acceptance document.
- Fast Contracts on the exact current `main` commit must pass, and the owner issues the exact release command on issue #241.
- Stable promotion checks GitHub RC tag→commit, published asset size/digest, accepted manifest and a fresh SHA-256 of downloaded installer bytes.
- The identical RC Per-User setup is installed, reinstalled and uninstalled on Windows CI; user-owned data must survive.
- The stable publisher creates friendly EXE/ZIP aliases from those same bytes. **No Qt or Inno rebuild**, no reuse or overwrite of old stable tags.

For v1.1.6, promotion was triggered by `/k500 release stable v1.1.6` on issue #241, after the owner's positive RC trial. Software CI is not a substitute for independently logged hardware failure scenarios. The installed app still uses GitHub's latest non-prerelease release and the process-local `--update-candidate=<tag>` opt-in for RC qualification.

Candidate source and a green CI matrix do not by themselves authorize a public stable release.

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
