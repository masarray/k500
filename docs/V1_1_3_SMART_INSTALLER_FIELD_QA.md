# SonKuPik K500 v1.1.3 — Windows / physical K500 release-candidate QA

**Public stable v1.1.2 remains untouched.** This checklist tests a GitHub
`v1.1.3-rc.N` **prerelease**, not the latest production download.

## A. GitHub Actions — no terminal commands

1. Open the repository [Actions](https://github.com/masarray/k500/actions).
2. Confirm the latest `K500 CI` run on `main` is **green**.
3. Select **Windows Updater Release Candidate**, then **Run workflow**.
4. Select branch `main`, a **new unused positive** `rc_number` (first new RC:
   `1` if that tag does not exist), and enable **Publish candidate as GitHub
   prerelease**. Click the green **Run workflow** button once.
5. Open the new run. All steps must be green, including exact-head check,
   real-app Smart install/reinstall, updater deep acceptance, checksums and
   three-artifact provenance. Never rerun an already published RC tag.
6. Open `Releases`, choose `v1.1.3-rc.N` **Pre-release** and download the
   `Windows-Smart-Installer.exe`. Do not install by double-clicking a ZIP's
   inner file or use the stable `/download/windows` link for RC testing.

## B. Windows GUI, scope, and data — use a non-production PC or Windows VM

Before running RC: close K500, export/back up your LOCAL `.k500` presets
and record any important customized settings. Do not manually delete
`Program Files`, `LocalAppData`, registry keys or personal presets.

- **Clean per-user:** run Smart Installer; select **current user**. It should
  install without administrator UAC, launch with version **1.1.3**, and
  show Windows Installed Apps once.
- **Same-scope upgrade:** with a real older SonKuPik installation in an
  isolated test environment, run Smart Installer again. It must retain scope,
  keep LOCAL presets/settings/cache, and not create duplicate shortcuts.
- **Machine mode:** choose **all users** on a clean snapshot. UAC must be
  normal and user-controlled. Verify Program Files installation and no
  second per-user copy.
- **UAC cancel:** cancel the Windows permission dialog on a disposable
  machine-mode test. Verify the original app and user data still work.
- **Conflicting registrations or stale installer:** use only dedicated
  snapshots/VM fixtures; installer must stop clearly without deleting
  either copy. **Never deliberately corrupt your everyday PC's registry.**
- **Unplug internet before opening:** the app must start without popup
  network errors and LOCAL/bundled/cached presets must remain available.
- **Disconnect/reconnect during a manual update check:** retry **Cek Update**
  from About once online. Automatic network failure must not affect K500
  control, presets or startup.

For legacy version matrix (1.0.x, 1.1.0, 1.1.1, 1.1.2), run each real
install in an isolated Windows snapshot and record its original scope,
upgrade result, health check, uninstall registration, whether shortcuts are
duplicated, and whether data remains unchanged. If installer blocks a
cross-scope migration, count that as **safe refusal**, not a successful
migration. Note the blocked scenario for a later transactional repair.

## C. Physical USB/HID — only after the Windows GUI is stable

With your K500 device connected by USB, test in this order:
1. **Connect** and wait for full device **Readback**.
2. **Recall** an existing hardware slot and confirm authoritative values.
3. Change a small non-critical control and verify live readback.
4. **Store** a known-good test preset to a slot you explicitly permit to change.
5. **Mass Upload** only on deliberately backed-up test slots; confirm exact
   target mapping, descending 10→1 behavior and post-upload Slot 01 readback.
6. Repeat Connect/Readback and a harmless Recall while completely offline.
7. Test network restoration and manual **Cek Update** with USB still working.
8. Do not start an installer during active Store/Mass Upload. Verify in-app
   update consent and device-busy safety only under a controlled test.

Use separate recordings/screenshots for GUI, updater dialog and failures.
A machine with no physical K500 can pass Windows install QA, **not**
USB/HID field acceptance.

## D. Report results back in this conversation

Provide the **RC tag** (for example `v1.1.3-rc.1`), your Windows edition,
previous SonKuPik version/scope (if upgrading), whether the Smart wizard
completed, whether presets/QSettings survived, results for
Connect/Readback/Recall/Store/Mass Upload, and any screenshots/logs.
State **Accepted for stable** only if the Windows upgrade matrix and
physical USB/hardware behavior meet the release requirement.

Until that acceptance, do not run **Windows Updater Stable Promotion**,
change public landing downloads, or publish a stable tag.
