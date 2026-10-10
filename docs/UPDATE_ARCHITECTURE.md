# K500 Windows updater — engineering status

## Offline-first resilience after v1.1.2 (future patch only)

**K500 runs offline.** A failed background GitHub update check must not
show an error popup, restart the app, block USB/HID control, or claim
a new version is installed. A user-triggered **Cek Update** may show a
friendly connection-unavailable explanation; it never installs anything
unless the user explicitly confirms the verified update.

`OFFLINE_NETWORK_BOUNDED_FALLBACK_V1` limits small GitHub API requests
to 12 seconds, leaves large installer transfers to their own handling,
and runs a network-free native self-test covering silent automatic failure,
manual diagnostic failure, and clearing stale error state.
`OFFLINE_PRESET_CACHE_FALLBACK_V1` bounds official preset catalog checks
(12 seconds) and small preset downloads (30 seconds). When the network
fails, all previously validated cached, bundled and local presets remain
available; a neutral fallback status replaces a red connectivity error.
Malformed or tampered catalog/preset data still report genuine
validation errors and never overwrite valid cache data.

These changes are in source only until a separately qualified future patch
release. Existing public v1.1.2 installer bytes remain unchanged.

## Release-1.1.2 updater troubleshooting and 1.1.3 UX follow-up

In v1.1.0 and v1.1.2 the application **automatically discovers** new
stable releases after the GUI starts (~2.5 seconds) and, during a session,
every six hours. A persisted six-hour successful-discovery throttle applies;
a version explicitly skipped in the update dialog stays skipped. Discovery
opens a consent dialog; installing a new Setup is **not** an unattended write.
Network errors on background discovery do not interrupt K500 control.

For a v1.1.0 machine that does not display an update prompt, download the
[latest stable machine Smart Installer](https://github.com/masarray/k500/releases/latest)
and install in-place with the same scope. Do not delete Program Files or
LocalAppData by hand; preserve Documents, QSettings and user preset caches.
If separate registered machine and per-user installs coexist, the updater
intentionally fails closed rather than destroying either copy.

**Post-v1.1.2 source change, not shipped in stable v1.1.2:**
`STARTUP_FRESH_RELEASE_DISCOVERY_V1` also corrects the persisted
successful-check timestamp: on every new application process the 2.5-second
startup timer must contact GitHub Latest regardless of a successful request
made by an earlier process. The six-hour throttle still applies *within*
one process, and manual checks always bypass it. Release availability still
shows the existing consent popup; the update coordinator still verifies
checksum/manifest, preserves K500 device transactions, runs Inno and restarts
only after a healthy install. This change requires an independent patch RC;
it cannot retroactively rewrite the shipped v1.1.0 or v1.1.2 executable.

**Post-v1.1.2 source change, not shipped in stable v1.1.2:** a compact
**Cek Update** action in About calls `AppUpdater.checkForUpdates(true)`
to bypass the six-hour throttle and displays version/status/error in the
existing premium UpdateDialog. This is a manual *check*, not implicit
installation, and the source must be qualified in a future patch RC before
the current v1.1.2 owner could receive it.

Installer model remains **two distinct update scopes**: machine/HKLM
(requires elevation) and per-user/HKCU (no elevation), using the same
Inno AppId. Only one primary Smart Installer should be recommended in
public-facing UI; the per-user package is the internal scope-matched
update backend. They must not be collapsed or old assets renamed without
1.0.x-to-current migration and recovery acceptance.

SmartScreen reputation and UAC are separate Windows trust/privilege
boundaries. Inno Setup cannot guarantee no SmartScreen warning while
packages remain unsigned. Never disable security controls or blindly
uninstall installations in either scope merely to suppress a warning.
See [installer/updater issue #182](https://github.com/masarray/k500/issues/182).


## Current stable behavior

The published v1.0.3 binary uses an in-app GitHub stable-release check,
manifest + SHA-256 verification, and Inno Setup with a machine-wide Program
Files installation. A new source commit does **not** become a public update
until a new version is assigned and a corresponding release is published.

## P1: reliable external install handoff (merged; not yet in public v1.0.3)

`SonKuPik-K500-Updater.exe` is a standalone **Win32, no-Qt-runtime** helper
built from `packaging/windows/update_helper.cpp`, packaged alongside the
application. The installed application stages a copy outside the installation
directory and checks the copy's SHA-256 against the installed helper.

1. On an installed (not portable) copy, K500 discovers the stable release,
   verifies release metadata, downloads Setup, and checks its SHA-256.
2. The backend's `m_deviceTransactionBusy` gate starts **closed** and is
   mirrored from the real `K500PresetManager.busy` property. If a transaction
   starts during download, it waits until the coordinator becomes idle.
3. K500 launches the standalone helper **as the original user** and quits.
   The helper waits for the exact parent PID to exit. It never terminates
   the K500 process and cannot interrupt a permanent write.
4. The helper re-hashes Setup with a read-only handle that denies writers,
   then asks Windows for UAC **only for the existing machine-wide installer**.
   The helper itself stays unelevated.
5. Inno installs with `/AUToupdate=1 /HELPERUPDATE=1`. The second flag
   suppresses the legacy automatic [Run] relaunch; older updater binaries
   without the flag retain their established restart behavior.
6. The helper waits for the installer process's exit code. After exit code 0,
   it invokes `SonKuPik-K500.exe --update-health-check=<version>` to verify
   the new version and embedded UI fonts **without device I/O**, then restarts
   the app in the original user's session.
7. Coordinator errors are recorded in `updates/v<version>/update-handoff.log`; Inno Setup writes separately to `update-handoff.log.inno.log`. If UAC is
   declined before installation, the helper reopens the existing application.
   If installation fails or times out, it reports the failure without
   terminating the installer, deleting user files, or claiming success.

No preset protocol, official preset, local preset, or Windows security setting
is changed by P1. The helper is not a privileged service or a UAC bypass.

## P2: separate per-user installer (merged; not yet in public v1.0.3)

The legacy `SonKuPik-K500-v<version>-Windows-Setup.exe` filename remains
**machine-wide** for older installed v1.0.3 updaters. The new
`SonKuPik-K500-v<version>-Windows-Setup-PerUser.exe` package uses
`PrivilegesRequired=lowest` and `{userpf}\\SonKuPik K500` (the current
user's LocalAppData Programs directory).

The two installers use the original Inno AppId but separate uninstall registry
roots (HKLM machine / HKCU current user). The per-user installer rejects an
existing registered machine-wide copy rather than creating duplicate shortcuts.
Neither package silently moves a registered machine-wide install to per-user.

The application checks the actual Inno uninstall registration **and its exact
installed path**, not a writable marker or just a Program Files path prefix.
If registration is missing/ambiguous or the path changes, automatic updates
fail closed; extracted portable ZIPs do not become installations. The
installed scope chooses the matching asset, checksum and manifest entry:
machine continues to request UAC only for its installer; per-user launches
the matching lowest-privilege Setup without UAC.

Both packages and the portable ZIP have independent release SHA-256 entries
and manifest artifact identities. Per-user installers must not be published
before the full P2 asset-routing and per-user upgrade qualification pass.

## P3: explicit migration, recovery, and resumable delivery

P3 completes the application-owned lifecycle without silently moving users.

- A machine-wide install may explicitly choose the per-user package. The helper
  installs and health-checks the per-user copy **before** requesting one UAC
  elevation to remove the old Program Files copy. If any pre-uninstall step
  fails, the per-user copy is removed and the machine install is reopened.
- Normal updates create a complete application-directory recovery snapshot
  outside the install tree before Setup starts. Installer non-zero exit or a
  failed new-app health check restores the previous files, checks the previous
  version with the hardware-free health command, and reopens it. Program Files
  restore elevates only the staged helper; per-user restore remains unelevated.
- An installer timeout is intentionally not killed or raced by rollback because
  it may still be writing files. The recovery snapshot is retained and its path
  is reported instead of making an unsafe success/rollback claim.
- Interrupted downloads keep a `.part` file. Retry sends an HTTP Range request;
  resumed responses must be 206 with the exact Content-Range start. Servers
  that return a normal 200 cause a safe restart from byte zero. Completion is
  still gated by release byte count plus manifest/SHA256SUMS SHA-256 agreement.
- An opposite-scope uninstall registration is removable only when its recorded
  app **and** uninstaller are both absent and the user explicitly chooses the
  repair action. Active duplicate installations fail closed and are never
  silently deleted.
- Documents presets, QSettings and the LocalAppData official-preset cache stay
  outside installer ownership. Windows CI plants persistence sentinels across
  an explicit machine-to-user migration and verifies they survive unchanged.

The per-user installer accepts an existing HKLM registration only with the
combined internal `/MIGRATEFROMMACHINE=1 /HELPERUPDATE=1 /AUToupdate=1`
handoff. A direct per-user install still refuses an existing machine install.

## P4: v1.1.0 release-candidate qualification

P1-P3 are merged on `main` with exact-head Windows qualification. The source version is now `1.1.0`, but **public stable remains v1.0.3** until an exact release candidate is separately accepted.

P4 owns the release boundary rather than weakening the stable channel:

- publish only immutable manual `v1.1.0-rc.N` GitHub prereleases built from one exact `main` commit;
- keep normal application launches on `/releases/latest`; public v1.0.3 therefore never sees prereleases;
- an installed v1.1 candidate may be launched explicitly with `--update-candidate=v1.1.0-rc.N`. This process-local QA channel fetches only that tag, requires a prerelease + `updater-rc` manifest, and does not write candidate skip/throttle state into stable QSettings;
- public v1.0.3 → RC compatibility is tested by manually applying the exact RC machine installer over a real v1.0.3 machine install, then using the candidate channel for same-version migration/re-apply tests;
- acceptance records the immutable RC tag, exact source commit, and SHA-256 of machine installer, per-user installer, and portable ZIP;
- v1.1 stable promotion **does not rebuild**. It downloads those accepted prerelease assets, resolves the tag back to the accepted commit, verifies RC manifest + three hashes, revalidates package health/scope, and republishes the same bytes under stable filenames with a stable v3 manifest;
- the legacy `windows-stable-release.yml` builder is fail-closed for v1.1+; `windows-updater-promote.yml` is the only v1.1 stable path;
- never overwrite public v1.0.3, an RC tag, or a stable tag.

## Deliberate scope boundaries and remaining work

- **P1/P2/P3:** merged and source-qualified; merge does not equal a published release.
- **P4:** release-candidate packaging and desktop acceptance are required before v1.1.0 stable promotion.
- **Portable:** remains intentionally independent. It is never silently changed into an installed copy; a future portable-update workflow must be explicit.
- **Device support:** updater work does not broaden the existing Windows x64 + USB HID hardware-qualified scope or Bluetooth qualification.
- **Release management:** stable v1.1+ publication must refuse `pending` or incomplete RC provenance, must promote byte-identical accepted artifacts, and must never overwrite an existing tag.

Official Windows binaries remain unsigned open-source releases. SHA-256 plus
release metadata detect inconsistent downloads but do not alone authenticate
the publisher against a fully compromised release account.
