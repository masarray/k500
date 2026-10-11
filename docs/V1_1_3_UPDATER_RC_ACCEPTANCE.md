# v1.1.3 — Smart Installer / Updater Release-Candidate Acceptance

> **PENDING.** Public stable remains immutable **v1.1.2**. No release manager
> may mark accepted before Windows and physical K500 owner testing.

UPDATER_V1_1_3_ACCEPTANCE=pending
UPDATER_V1_1_3_ACCEPTED_TAG=pending
UPDATER_V1_1_3_ACCEPTED_COMMIT=pending
UPDATER_V1_1_3_MACHINE_SHA256=pending
UPDATER_V1_1_3_USER_SHA256=pending
UPDATER_V1_1_3_SMART_SHA256=pending

## Exact source scope

- v1.1.2 public installer bytes and historical acceptance record are frozen.
- PR #183/#184: startup updater discovery, manual update check, offline-first network safety.
- PR #185: read-only original-user HKCU/LocalAppData collision safety preflight.
- PR #186: native Inno scope-aware single-file Smart Installer candidate.
- The **three** candidate artifacts share one immutable source commit and installed
  application; Machine and Per-User legacy filenames remain stable for existing updaters:
  `SonKuPik-K500-v1.1.3-rc.N-Windows-Setup.exe`,
  `SonKuPik-K500-v1.1.3-rc.N-Windows-Setup-PerUser.exe`,
  `SonKuPik-K500-v1.1.3-rc.N-Windows-Smart-Installer.exe`.
- No DSP, preset, protocol, USB/HID, Bluetooth or accepted GUI redesign implied.

## RC workflow requirements

1. Main exact commit passes all jobs of **K500 CI**.
2. Manual **Windows Updater Release Candidate** action is dispatched from `main`
   with an unused positive `rc_number`. Publishing as GitHub *prerelease*
   must not change `/releases/latest`.
3. RC builds the real app once; machine/per-user/Smart installers are built
   from those exact bytes. All pass installed-app health checks; the native
   Smart user-mode reinstall preserves user state and registered scope.
4. App-owned updater helper and deep migration/rollback/sentinel suite pass.
5. RC manifest and `SHA256SUMS.txt` list **three** named installers.
6. RC workflow uploads the exact immutable binaries and provenance.
7. No stable promotion or overwrite of any existing RC tag.

## REQUIRED real Windows + K500 owner field acceptance

See [v1.1.3 Windows GUI and K500 manual QA](V1_1_3_SMART_INSTALLER_FIELD_QA.md).
Record: RC tag, full source SHA, exact 3 SHA-256 values, Windows installer
upgrade matrix outcomes, offline reconnect, K500 USB/HID behavior,
data-persistence checks, and explicit owner acceptance. A green CI alone
**never qualifies physical hardware** or legacy OS migrations.

**Manual-install limitation:** Smart Installer has Inno's native
scope-preserving installation, read-only collision detection and normal
Windows uninstaller. The app-owned helper has verified rollback for
in-app updates, but the standalone Smart Installer does not yet guarantee
transactional rollback after all possible manual installation failures.
Do not state that user→machine migration, dual-registration repair or manual
rollback has been field-qualified without separate evidence. Fail closed
when uncertain; never mass-delete installed directories.

## Stable promotion — locked until owner acceptance

After RC field acceptance, create an independent PR updating *this* file's
six tokens to accepted tag, 40-character RC commit, and exact 64-character
Machine, Per-User, Smart hashes. Merge only after review. Then run the manual
**Windows Updater Stable Promotion** workflow. It must resolve RC tag→commit,
verify all three RC SHA-256s and manifest entries, and **copy, never rebuild**
the accepted bytes under stable filenames. Public `v1.1.3` must not exist
before that run; never change or replace published `v1.1.2`.

Only after stable promotion is verified should the landing
`/download/windows` route switch from the machine asset to the new Smart
asset and its fallback public version text be updated. Preserve the
Machine/Per-User assets as authenticated update backends.
