# v1.1.3 — Smart Installer / Updater Release-Candidate Acceptance

> **OWNER ACCEPTED RC.6 (2026-10-13).** Stable publication still requires the
> locked promotion workflow to pass; public v1.1.2 remains unchanged until then.

UPDATER_V1_1_3_ACCEPTANCE=accepted
UPDATER_V1_1_3_ACCEPTED_TAG=v1.1.3-rc.6
UPDATER_V1_1_3_ACCEPTED_COMMIT=2cfd0060d5c4d192d35347ce1ae34b250d98d7ff
UPDATER_V1_1_3_MACHINE_SHA256=282b0d5111879786c8db6c21e3542e81d43ec6cc475d130d12356950c494cde6
UPDATER_V1_1_3_USER_SHA256=6f9af6dcf5b8fcf4540ee417f322873dd2ecdd33cd9c7686f21a44b71ed9cc14
UPDATER_V1_1_3_SMART_SHA256=760531e9ce9bf741db5d39f589c599101e2b950b2bad04c81e12e52191cea680

## RC.6 accepted by K500 owner — 2026-10-13

**Explicit owner acceptance:** "RC.6 lulus, setuju stable." This response
followed a direct field-check question asking whether the **RC.6 Smart
Installer** was successfully installed on a real Windows PC with existing
settings/presets preserved, and whether physical K500 USB Connect, Readback,
Recall, Save and Mass Upload tests passed. The owner's affirmative reply is
recorded as the required **field sign-off on RC.6**; it is not a new CI result
or a claim that the assistant personally witnessed the machine.

- Immutable prerelease: [v1.1.3-rc.6](https://github.com/masarray/k500/releases/tag/v1.1.3-rc.6),
  source commit `2cfd0060d5c4d192d35347ce1ae34b250d98d7ff`.
- [Exact-main CI #38384812228](https://github.com/masarray/k500/actions/runs/38384812228):
  successful Fast Contracts, native Windows Build/Regression and installer smoke.
- [RC qualification #38389976497](https://github.com/masarray/k500/actions/runs/38389976497):
  successful Machine, PerUser and Smart real-app test installations, health
  checks, deep updater migration/rollback, checksum and prerelease publishing.
- The exact Machine/PerUser/Smart installer SHA-256 hashes are recorded above
  in the stable-promotion workflow's required order and verified against the
  published RC.6 release metadata.
- Earlier RC.5 real Windows migration and pre-RC.6 K500 hardware screenshots
  remain historical supporting evidence, not substitutes for the owner's
  explicit RC.6 acceptance.
- **Limit of attestation granularity:** prior installed scope, UAC event log,
  precise storage paths and per-slot USB results were not individually
  enumerated in the owner's reply. Do not claim separate test coverage of
  every migration edge case or measured device DSP transfer coefficients.
  The owner nevertheless explicitly accepted the tested RC.6 for stable.

**Promotion authority:** these six tokens pin owner-accepted RC.6 **bytes**.
Only use the existing `Windows Updater Stable Promotion` workflow from
`main`, after this acceptance-only PR is merged and verified. The promotion
must verify all tag/commit/hash/manifest gates and copy the three accepted RC.6
installer binaries without rebuilding, overwriting or moving earlier tags.
Stable publication is not complete until the resulting GitHub release,
`latest` pointer, filenames and SHA-256 values are independently checked.

## Revoked public Smart candidate — v1.1.3-rc.1

**DO NOT PROMOTE v1.1.3-rc.1.** Although its CI passed, the public Smart
configuration inherited previous Machine privileges and could install to
Program Files. CI forced `/CURRENTUSER` and `/DIR`, obscuring the interactive
default-path regression. The immutable RC.1 binaries/checksums remain
historical evidence, **not eligible for acceptance**.

The corrected native Smart Installer must provide a **consent-based direct
machine-to-user migration** when it discovers the exact canonical Program
Files machine install. It delegates to the app-owned SHA-256 verified helper:
machine backup + HKLM snapshot first, per-user install + health-check second,
one normal UAC for the protected old uninstaller third. It must refuse
ambiguous, stale, or dual registration before touching either copy. It must
not claim cleanup until the old app, HKLM uninstall registration and canonical
Program Files folder are absent. If residual files cannot be safely attributed
to the old installer, report incomplete cleanup; never delete blindly.

Any acceptable **v1.1.3-rc.2 or higher** must install in
`%LOCALAPPDATA%\\Programs\\SonKuPik K500` **without admin UAC, all-users
choice, or scope flags**. It must reject a registered old machine install
without writing another copy, and separately offer a user-approved
**transactional** machine-to-user migration that verifies the LocalAppData
copy and cleans the old Program Files uninstall registration/files/shortcuts.
Removing a protected machine installation requires a **single authorized
elevation**; there is no safe way to bypass Windows permissions.


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
   from those exact bytes. All pass installed-app health checks; the Smart
   must use the **native default LocalAppData path with no /DIR or
   /CURRENTUSER**, reinstall without UAC, and preserve HKCU/state.
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
