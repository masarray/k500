# K500 v1.1.5: updater abort, rollback and USB failure (2026-11-08)

## Field evidence
- User photo of external updater: "Installer exited with code 5. Rollback also failed: Could not remove the failed installation." Backup and update log were preserved under LocalAppData/MasArray/SonKuPik K500/updates/v1.1.5.
- User photo of GUI: USB selected, device status ERROR.
- User reports clean reinstall restored normal operation.
- No Inno sublog was supplied, so the exact cancelled/aborted operation is UNKNOWN. Inno code 5 means Cancel during install or Abort after an error, not proof of a faulty laptop. Reference: https://jrsoftware.org/ishelp/topic_setupexitcodes.htm

## Audited source failure
In packaging/windows/update_helper.cpp, the former restoreTree started rollback with recursive remove_all(target). If Windows still held the failed installation open, rollback stopped before restoring previous app files. In src/main.cpp, the health check verifies process, version and font, NOT physical HID readback. The USB ERROR may be related to a partial installation but cannot be causally proven without hardware evidence and the original updater/Inno logs.

## Hardening candidate
1. Retain the verified downloaded installer SHA-256, exact scope, Inno registry snapshot, prior application backup and user consent boundaries.
2. For failed in-place updates, atomically rename the failed K500 installation into a unique sibling .failed-update archive on its own volume, with bounded lock retries. Never begin by recursively deleting unknown files. If locked, preserve both prior snapshot and failed directory and give actionable guidance.
3. Restore previous files and Inno uninstall registration, verify prior app version, relaunch, and only then remove redundant backup. Archive retains unknown user files for review.
4. Classify Inno code 5 as Cancel/Abort, log the exact Inno sublog path, and use simple user language rather than bare internal error numbers.
5. For USB, classify absent HID device, Windows sharing/access denial and disconnected HID. Clicking the red ERROR indicator opens the precise reason and Coba Lagi / Simpan Laporan. No unverified firmware packets or automatic write retries.
6. Preserve v1.1.5 public installer/tag/hash unchanged. Release only a new separately qualified version.

## Test matrix, efficient CI
- Existing helper native selftest: failed installation archive retains unknown user bytes, original EXE restored.
- Existing Installer Smoke: scoped real Inno-installed test app, fake installer damages EXE and returns code 5, verify original EXE hash, HKCU registration, user file, failed archive, and log classification.
- RC-only existing deep updater acceptance also reproduces code 5. Do not create new workflows or mass-trigger heavy RC builds.

## Physical pre-release requirements
- Public v1.1.5 -> separately versioned candidate in-app update on Windows x64. Inspect exact Inno log and ensure user-owned presets/settings/cache persist.
- Test interruption, locked executable/DLL, second app instance, permissions, insufficient disk space, restart pending, and correct rollback without silent data loss.
- On real K500 hardware, verify USB Connect, full 939-byte readback, reconnect, unplug and busy-HID recovery; no implicit Store/Upload, no partial-state LIVE.
- Clean install success alone and green CI do not prove update or USB qualification.
- If rollback is blocked by Windows, preserve existing recovery files and ask for redacted update-handoff.log and update-handoff.log.inno.log (remove personal usernames) rather than asking the user to delete LocalAppData.

## Follow-up hardening — installer concurrency + Linux QA (2026-11-08)

The installed Inno Setup script already declared `AppMutex=SonKuPikK500.K500.Native` while the interactive K500 GUI did **not** actually create that Win32 named mutex. According to Inno's AppMutex contract, a matching runtime mutex is mandatory for installer/uninstaller protection; the declaration alone is inert. A second K500 process could therefore survive the first GUI's update handoff and continue using installation DLLs while Inno attempted replacement.

The native Windows GUI now creates the exact declared mutex before loading the normal QML application and retains its OS handle through process exit. Non-interactive `--update-health-check` and protocol/font self-tests do **not** hold it, avoiding a false installation block during updater verification. Existing Windows regression suite verifies the string parity with the Inno script and runs `--app-mutex-self-test` against the real compiled app. This is concurrency prevention, not proof it caused the original user's code 5.

The subsequent `main` CI also identified an unrelated **Linux package QA false-negative**: the `tar -tzf | grep -q` check under `set -o pipefail` closed the listing pipe early, causing `tar: stdout: write error`. It is replaced with full-stream `grep -Fx`, and the existing lightweight Fast Contracts job now executes a Linux archive-listing regression. A change to the consolidated packaging workflow triggers the existing native platform QA gates on the next main push; no new workflow or RC build.

Acceptance still requires real Windows in-place update and K500 USB hardware proof. The published v1.1.5 installer and its hashes remain immutable.
