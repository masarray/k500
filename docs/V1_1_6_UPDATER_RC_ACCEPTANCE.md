# SonKuPik K500 v1.1.6 — independent updater recovery acceptance

# K500_RELEASE_COMMENT_GATE_V1
# New release line following the immutable public v1.1.5. Do not reuse RC.1
# from the previous version. The P0 updater fixes in PRs #239/#240/#242
# have green automated Windows/Linux/macOS CI but have NOT yet been proved
# by actual K500 USB hardware after an in-place v1.1.5 -> v1.1.6 upgrade.
#
# Approval must be recorded by the owner *after* a real RC candidate is
# published, installed and tested. No CI or ChatGPT agent may replace this
# evidence with guesses, synthetic USB tests or a green compile.
UPDATER_V1_1_6_ACCEPTANCE=accepted
UPDATER_V1_1_6_ACCEPTED_TAG=v1.1.6-rc.1
UPDATER_V1_1_6_ACCEPTED_COMMIT=39f9cc926a53900e62f439c44f61878880c6d1d2
UPDATER_V1_1_6_USER_SHA256=937a28a4bb715f60ba7ed06c437bbce165357d519bab824b77fa84c39b284899

## Owner physical acceptance checklist (issue #241)
- [ ] Download the exact prerelease RC and verify SHA-256 against its manifest.
- [ ] On a real Windows laptop with official public v1.1.5 installed, update
      in place to RC v1.1.6; personal presets, QSettings and registry survive.
- [ ] USB Connect / disconnect / reconnect on real K500 DSP, complete 939-byte
      readback before LIVE; no unintended Save/Store/Upload writes or changes.
- [ ] Inno cancellation/abort, existing second K500 process, locked files,
      interrupted installation, failed relaunch and protected backup verified.
- [ ] Reinstall/update same-scope and uninstall preserve user-owned data.
- [ ] Confirm that RC installer launches on Windows 10/11 x64 without requiring
      Administrator for the normal per-user install; SmartScreen is not bypassed.
- [ ] Record owner acceptance on this file through reviewed PR: set
      ACCEPTANCE=accepted, the exact RC tag, SHA-40 source commit, and the
      exact 64-hex SHA-256 from published immutable prerelease manifest.
      Never change accepted RC bytes to get a green check.

## Workflow behavior
1. Comment `/k500 release rc v1.1.6-rc.1` on issue #241 as repository owner.
   Existing K500 CI must have completed successfully on the exact current main.
   Reuse the existing single Windows updater RC build and publish a prerelease.
2. Physical acceptance is **not** inferred from a green Actions run.
3. After the owner approves, merge an acceptance-only PR with the four
   machine-readable values above and confirm exact-main CI on the acceptance
   commit. Then comment `/k500 release stable v1.1.6` on issue #241.
4. Stable promotion downloads and checks the **identical** RC installer bytes,
   creates friendly EXE and ZIP aliases and publishes the stable release.
   Stable publication never recompiles app or installer.
5. No comment ever mutates or retags public v1.1.5. For future versions, bump
   CMake version and create a corresponding pending acceptance file first.

**Support boundaries:** Windows x64 / USB HID is the target for release QA;
Linux/macOS native CI is additional evidence, not a claim of K500 physical USB
qualification on those platforms. Bluetooth is experimental.

## Owner acceptance — 2026-11-08

Following a trial of the published v1.1.6-rc.1 candidate, the repository owner stated:

> ok saya coba sudah bagus, bisa rilis stable

The owner therefore explicitly approves promotion of **the same immutable v1.1.6-rc.1 Windows Per-User installer** to public stable v1.1.6 without rebuilding. The GitHub RC release, annotated tag source commit, asset digest and CI are recorded by the machine-readable values above.

**Evidence boundary:** this release approval records the owner's positive hands-on trial and permission to publish. It does **not** assert that every separate scenario in the checklist (all 939-byte readback traces, antivirus/locked files, power loss, reinstall matrices) was independently documented. Leave specific unrecorded checks unchecked; retain issue #241 for those follow-up field diagnostics. CI is not a substitute for device evidence.

**Immutable source:** https://github.com/masarray/k500/releases/tag/v1.1.6-rc.1

**Gate:** promote exactly the 55,615,005-byte Per-User EXE whose SHA-256 is `937a28a4bb715f60ba7ed06c437bbce165357d519bab824b77fa84c39b284899`; the stable `v1.1.6` release must only copy and rename these verified bytes. Historical public v1.1.5 stays untouched.
