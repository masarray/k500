# v1.1 Updater Release-Candidate Acceptance

> Public stable remains **v1.0.3** while release-provenance qualification is pending. The auto-managed/self-update lifecycle itself has completed follow-up desktop testing successfully after PR #96.

UPDATER_V1_1_ACCEPTANCE=pending
UPDATER_V1_1_ACCEPTED_TAG=
UPDATER_V1_1_ACCEPTED_COMMIT=
UPDATER_V1_1_MACHINE_SHA256=
UPDATER_V1_1_USER_SHA256=
UPDATER_V1_1_PORTABLE_SHA256=

This record is the fail-closed publication gate for the P1–P4 Windows updater lifecycle. It covers desktop installation/update behavior only; it does not broaden the existing Windows x64 + USB HID K500 hardware support claim.

## Current immutable candidate under acceptance

The final source-aligned candidate is now published as GitHub prerelease `v1.1.0-rc.3` from exact commit `b5ff360bcebe8afc18a01ac7e303c0fde67cb76b`.

Recorded candidate artifacts:

- machine Setup SHA-256: `71388686a644f8a5e051d1ad01d9f691b0a3f799c5fe4c46f78fb34801d3b2e8`;
- per-user Setup SHA-256: `6863b6cde993cbb605795caf1f8a9fb5314e5902f219cdb9bbc5822a0cecd568`;
- portable ZIP SHA-256: `4f7da0f9bb15d4b5c00f155711995062b8c97bdd74fb955535fab9b68a1e62b3`;
- qualification workflow run: `36847039459`.

The RC workflow completed successfully on the exact candidate commit, including exact-main prerequisite gates, build/regression tests, direct-app health checks, portable validation, machine-wide installer validation, per-user installer validation, provenance generation, artifact upload, and immutable prerelease publication. The release asset digests, generated `SHA256SUMS.txt`, RC manifest artifact records, and independently recomputed SHA-256 values agree.

No updater runtime, update-helper, or installer implementation file changed between the functionally accepted `v1.1.0-rc.2` updater baseline and this RC3 candidate. RC3 aligns the final preset library/catalog truth and release guards with current source. RC2 functional desktop evidence therefore remains relevant regression evidence, but it does not by itself change the exact-candidate publication token below to `accepted`.

The machine-readable acceptance fields intentionally remain pending until exact RC3 desktop acceptance is explicitly recorded.

## Functional desktop status

Follow-up operator testing after PR #96 confirms the auto-managed/self-update lifecycle is functionally working, including the no-admin per-user update path and post-update relaunch visibility. This closes the updater-runtime/UX acceptance concern.

The machine-readable acceptance token intentionally remains `pending` until one immutable accepted RC tag, its exact source commit, and SHA-256 values for the machine installer, per-user installer, and portable ZIP are recorded below. That remaining work is release provenance/promotion bookkeeping, not an unresolved updater-runtime defect.

## Why RC testing is staged

Public v1.0.3 intentionally follows GitHub `/releases/latest` and ignores prereleases. Its already-published bytes cannot discover a `v1.1.0-rc.N` prerelease without exposing that RC as public latest, which this project will not do.

Therefore acceptance uses a two-stage compatibility path instead of pretending v1.0.3 can see a prerelease:

1. Install the public v1.0.3 machine-wide build as the real starting state.
2. Apply the exact v1.1.0 RC machine installer manually over that installation. This validates the legacy Inno upgrade boundary using the same installer bytes that can later be promoted stable.
3. Launch the installed v1.1.0 candidate with `--update-candidate=v1.1.0-rc.N`. This process-local QA flag fetches exactly that immutable GitHub prerelease by tag, accepts only the RC manifest/channel, and is never persisted.
4. Use the normal in-app updater UI to exercise same-version machine-to-user migration and candidate re-apply/update behavior against the exact prerelease machine/per-user packages.
5. After acceptance, stable promotion reuses the accepted installer/portable bytes byte-for-byte. It only renames the files to stable names and generates stable v3 metadata/checksums, so v1.0.3 later downloads the same accepted machine installer bytes through its normal stable channel.

## Candidate identity

- Target application version: `1.1.0`.
- RC tags: immutable `v1.1.0-rc.N` GitHub prereleases.
- Candidate must be built from one exact `main` commit after P4 merges.
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
- portable ZIP runtime smoke tests;
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
- [ ] Confirm portable ZIP remains independent and is never silently converted to installed mode.

## Promotion rule

Stable v1.1.0 publishing is blocked while `UPDATER_V1_1_ACCEPTANCE=pending`.

After the exact GitHub prerelease is accepted, a separate reviewed PR must:

- change `UPDATER_V1_1_ACCEPTANCE` to `accepted`;
- set `UPDATER_V1_1_ACCEPTED_TAG` to the immutable `v1.1.0-rc.N` tag;
- set `UPDATER_V1_1_ACCEPTED_COMMIT` to the exact 40-character source commit bound to that tag/RC manifest;
- record the lowercase SHA-256 of the accepted machine installer, per-user installer, and portable ZIP.

The v1.1 stable promotion workflow must independently resolve the tag to the accepted commit, download the prerelease assets, verify all three recorded hashes plus RC manifest provenance, validate the packaged binaries, and publish those exact bytes under stable filenames. A rebuild is not an accepted promotion mechanism.