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
