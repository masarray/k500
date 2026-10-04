# v1.1 Updater Release-Candidate Acceptance

> RC5 acceptance is recorded below from the immutable v1.1.0-rc.5 artifacts built from exact main commit `01c545ed27d9a4316fb255765a25ae41f0e1457a`. Public stable remains **v1.0.3** only until the byte-identical promotion workflow publishes v1.1.0.

UPDATER_V1_1_ACCEPTANCE=accepted
UPDATER_V1_1_ACCEPTED_TAG=v1.1.0-rc.5
UPDATER_V1_1_ACCEPTED_COMMIT=01c545ed27d9a4316fb255765a25ae41f0e1457a
UPDATER_V1_1_MACHINE_SHA256=47b74627d141bd9b9676fd8e9891e36f2f407b918660fbf955d9de26c59e9d0f
UPDATER_V1_1_USER_SHA256=ba5952249d32ae808d295470f24424209a30428fe255223d202beca40a01ce30

This record is the fail-closed publication gate for the P1–P4 Windows updater lifecycle. It covers desktop installation/update behavior only; it does not broaden the existing Windows x64 + USB HID K500 hardware support claim.

## RC3/RC4 validation evidence and RC5 final-candidate closure

RC3 was published as `v1.1.0-rc.3` from exact commit `b5ff360bcebe8afc18a01ac7e303c0fde67cb76b` and completed its exact-head qualification workflow successfully.

Operator desktop testing on the exact RC3 line confirmed:

- final preset names/content render correctly in the application;
- a Program Files installation correctly uses the explicit machine-to-user migration path and requests UAC only for removal of the old machine-wide installation;
- an already per-user LocalAppData installation updates without Administrator elevation;
- post-update relaunch is fast and returns the application to the foreground.

The RC-only deep updater lifecycle qualification in `tools/ci/updater_deep_acceptance.ps1` covers the persistence/cleanup behavior that is difficult to inspect manually. It runs from `windows-updater-rc.yml` against the exact per-user candidate installer, plants sentinels in `Documents\SonKuPik K500\Presets`, the LocalAppData official-preset cache, and QSettings, performs explicit machine-to-user migration, verifies all three sentinels survive unchanged, verifies the old Program Files application/HKLM registration are removed, and verifies the new HKCU per-user registration exists. It also exercises verified same-version update, rollback after installer/health-check failure, and narrow stale-registration repair. Normal PR CI retains only the smaller path-aware installer smoke so development does not pay this release-only cost.

RC3 is **not** the final stable candidate because current `main` subsequently synchronized the ten official preset filenames through PR #108. The public distribution policy is also being simplified to **Smart Installer only**: RC/stable publication retains the machine Setup plus the scope-matched per-user package required by the updater, while the Portable ZIP is removed from the v1.1 release contract.

RC4 was subsequently published from exact commit `3fa49b3627ffd0631e9e3915413775db65725852`, but it predates the final capture-backed K500 protocol/readback closure, output-delay/crossover completion, Adj Manner hardware-ownership semantics, and the final voluntary QRIS support integration. Those changes are part of the application binary and therefore RC4 cannot be promoted as v1.1.0 stable.

The next immutable candidate is **RC5** from one exact post-merge `main` commit. RC3/RC4 updater desktop results remain valid regression evidence for the updater lifecycle itself, but public stable provenance must bind to the newer RC5 bytes because stable promotion is byte-identical and may not rebuild after acceptance.

## RC5 acceptance record

Release-owner acceptance was recorded on 2026-10-04 after the exact RC5 workflow completed successfully on commit `01c545ed27d9a4316fb255765a25ae41f0e1457a`, including clean build, regression suite, machine-wide installer validation, per-user installer validation, and deep updater lifecycle acceptance.

Accepted immutable artifacts:

- Machine installer SHA-256: `47b74627d141bd9b9676fd8e9891e36f2f407b918660fbf955d9de26c59e9d0f`
- Per-user installer SHA-256: `ba5952249d32ae808d295470f24424209a30428fe255223d202beca40a01ce30`

The release owner explicitly authorized public release from this accepted RC5 line. Stable publication must be byte-identical promotion only.

## Functional desktop status

Follow-up operator testing after PR #96 confirms the auto-managed/self-update lifecycle is functionally working, including the no-admin per-user update path and post-update relaunch visibility. This closes the updater-runtime/UX acceptance concern.

The machine-readable acceptance token is now `accepted` for immutable `v1.1.0-rc.5`. The exact source commit and both installer SHA-256 values are recorded above. Stable promotion must reuse these exact bytes; no rebuild is authorized.

## Why RC testing is staged

Public v1.0.3 intentionally follows GitHub `/releases/latest` and ignores prereleases. Its already-published bytes cannot discover a `v1.1.0-rc.N` prerelease without exposing that RC as public latest, which this project will not do.

Therefore acceptance uses a two-stage compatibility path instead of pretending v1.0.3 can see a prerelease:

1. Install the public v1.0.3 machine-wide build as the real starting state.
2. Apply the exact v1.1.0 RC machine installer manually over that installation. This validates the legacy Inno upgrade boundary using the same installer bytes that can later be promoted stable.
3. Launch the installed v1.1.0 candidate with `--update-candidate=v1.1.0-rc.N`. This process-local QA flag fetches exactly that immutable GitHub prerelease by tag, accepts only the RC manifest/channel, and is never persisted.
4. Use the normal in-app updater UI to exercise same-version machine-to-user migration and candidate re-apply/update behavior against the exact prerelease machine/per-user packages.
5. After acceptance, stable promotion reuses the accepted machine/per-user installer bytes byte-for-byte. It only renames the files to stable names and generates stable v3 metadata/checksums; no Portable ZIP is published for v1.1.0.

## Candidate identity

- Target application version: `1.1.0`.
- RC tags: immutable `v1.1.0-rc.N` GitHub prereleases.
- Candidate must be built from one exact `main` commit after all public-release protocol/UI/support merges; the next eligible immutable candidate is `v1.1.0-rc.5`.
- Candidate updater opt-in is process-local and explicit; normal launches remain on `/releases/latest`.
- Windows artifacts remain unsigned open-source builds.
- Public v1.0.3 assets and tag are immutable and must never be overwritten.

## Automated qualification required on the exact RC commit

- standalone updater coordinator self-test;
- stable and candidate manifest/checksum parser self-tests;
- machine-wide and per-user installer build + runtime health checks;
- machine/per-user scope registration checks;
- explicit machine-to-user migration fixture;
- installer non-zero and failed-health rollback fixtures;
- interrupted/resumable download contract and final size/SHA-256 verification;
- stale-registration repair guards;
- Documents preset, QSettings, and official-cache persistence sentinels;
- immutable RC manifest + SHA256SUMS provenance.

## Windows desktop acceptance before stable promotion

- [ ] Install the public v1.0.3 machine-wide package and confirm its existing data/preferences.
- [ ] Manually apply the exact accepted-candidate machine installer over v1.0.3 and confirm the resulting v1.1.0 candidate is healthy.
- [ ] Launch that installed candidate with `--update-candidate=<accepted RC tag>` and confirm candidate metadata is discovered only for that exact prerelease tag.
- [ ] Choose **Update tanpa Admin** and confirm the per-user copy is downloaded, hash-verified, installed, and health-checked before the one-time old-install UAC removal.
- [ ] Confirm the migrated per-user copy can re-apply the exact candidate through the in-app updater without Administrator permission.
- [ ] Cancel the old-install UAC prompt in a controlled migration attempt and confirm the original machine installation remains usable.
- [ ] Exercise a failed/interrupted download and confirm retry preserves the selected install scope and resumes safely.
- [ ] Exercise rollback/recovery failure fixtures or equivalent controlled acceptance and inspect updater logs.
- [ ] Confirm Documents presets, QSettings preferences, and official preset cache survive upgrade/migration.
- [ ] Confirm successful migration leaves only one active SonKuPik installation/uninstall registration.

## Promotion rule

Stable v1.1.0 publishing is blocked while `UPDATER_V1_1_ACCEPTANCE=pending`.

After the exact GitHub prerelease is accepted, a separate reviewed PR must:

- change `UPDATER_V1_1_ACCEPTANCE` to `accepted`;
- set `UPDATER_V1_1_ACCEPTED_TAG` to the immutable `v1.1.0-rc.N` tag;
- set `UPDATER_V1_1_ACCEPTED_COMMIT` to the exact 40-character source commit bound to that tag/RC manifest;
- record the lowercase SHA-256 of the accepted machine installer and per-user installer.

The v1.1 stable promotion workflow must independently resolve the tag to the accepted commit, download the two installer assets, verify both recorded hashes plus RC manifest provenance, validate the packages, and publish those exact bytes under stable filenames. A rebuild is not an accepted promotion mechanism.