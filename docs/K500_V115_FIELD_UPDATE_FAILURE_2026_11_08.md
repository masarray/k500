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

## Follow-up P0 — preflight and relaunch safety (2026-11-08)

Two remaining rollback race conditions were found in the actual updater source:

- **Snapshot released before application relaunch:** successful in-place update removed the entire `recovery-previous` tree after only a short command-line version/font health check and *before* `startApplication`. A Windows CreateProcess rejection left no automatic previous-version recovery. The updater now attempts the new application launch while the snapshot remains intact. If CreateProcess fails, it restores the prior EXE and exact Inno uninstall-registration snapshot and relaunches the prior version. It archives failed-target unknown user bytes using the existing non-destructive rename. A **CI-only one-shot fault injection** in the updater helper exercises this gap after a healthy installer and health-check; release binaries do not include the fault injection.
- **Wasteful late backup detection:** the already-existing `recovery-previous` gate ran only after network metadata/Setup download, immediately before the GUI closed. The shared recovery-path guard now runs at the beginning of the selected-package transaction (before any network request) **and** at the final app handoff. It refuses to automatically delete either a real directory or unexpected file/link occupying the recovery path. A temporary-directory-only updater self-test verifies occupancy without ever touching real app data.
- **Other K500 GUI running:** the independently staged helper now probes the exact Inno `AppMutex` after the parent app exits and the installer hash is authenticated. If another K500 instance is still present (or Windows cannot safely inspect that mutex), setup is not started, no snapshot is created and no installed files are replaced. The existing installer smoke checks this with a real Windows named mutex held by a second process.
- **Truthful result logging:** snapshot removal outcomes are now recorded accurately. A verified install may have been relaunched while cleanup is still deferred; the log must never claim the snapshot was removed when Windows reports a file-system error.

**Scope limits:** successful `CreateProcess` does not prove the complete QML window, DSP USB handshake or user interaction is healthy. Do not equate the Windows smoke result with physical acceptance. Machine-to-user migration after removal of the old machine registration preserves its snapshot if new GUI relaunch fails; it does not silently request a second elevation or delete the verified per-user copy. Existing issue #241 remains the field acceptance gate. Public v1.1.5 assets remain immutable.
