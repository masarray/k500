# K500 Windows updater — engineering status

## Current stable behavior

The published v1.0.3 binary uses an in-app GitHub stable-release check,
manifest + SHA-256 verification, and Inno Setup with a machine-wide Program
Files installation. A new source commit does **not** become a public update
until a new version is assigned and a corresponding release is published.

## P1: reliable external install handoff (this PR; not yet released)

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

## P2: separate per-user installer (this stacked PR; not yet released)

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

## Deliberate scope boundaries and remaining work

- **P1 qualification:** exact-head Windows CI, Inno compile, helper self-test,
  deployed app health checks, and physical Windows upgrade test from an older
  installed version. A build passing is not proof of a successful upgrade.
- **P2:** introduce per-user `LocalAppData\\Programs` installer mode, without
  UAC for routine updates; distinguish machine-wide and per-user installation
  and never silently migrate the current Program Files installation.
- **P3:** explicit one-time migration, improved failure recovery / rollback,
  resumable downloads, distinct RC/stable build identity, and portable-update
  handling. Do not promise rollback while it is not implemented.
- **Release management:** bump CMake version before publishing; do not overwrite
  existing public stable v1.0.3 from development builds or bypass hardware
  acceptance gates.

Official Windows binaries remain unsigned open-source releases. SHA-256 plus
release metadata detect inconsistent downloads but do not alone authenticate
the publisher against a fully compromised release account.
