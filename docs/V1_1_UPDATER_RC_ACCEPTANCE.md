# v1.1 Updater Release-Candidate Acceptance

> Public stable remains **v1.0.3** while this qualification is pending.
>
UPDATER_V1_1_ACCEPTANCE=pending

This record is the fail-closed publication gate for the P1-P3 Windows updater lifecycle. It covers desktop installation/update behavior only; it does not broaden or replace the existing Windows x64 + USB HID K500 hardware support claim.

## Candidate identity

- Target application version: `1.1.0`.
- RC tags: immutable `v1.1.0-rc.N` prereleases.
- Candidate must be built from one exact `main` commit after P4 merges.
- Windows artifacts remain unsigned open-source builds.
- Public `v1.0.3` assets and tag are immutable and must never be overwritten.

## Automated qualification required on the exact RC commit

- standalone updater coordinator self-test;
- machine-wide and per-user installer build + runtime health checks;
- machine/per-user scope registration checks;
- explicit machine-to-user migration fixture;
- installer non-zero and failed-health rollback fixtures;
- interrupted/resumable download contract and final size/SHA-256 verification;
- stale-registration repair guards;
- Documents preset, QSettings, and official-cache persistence sentinels;
- portable ZIP runtime smoke tests;
- release manifest and SHA256SUMS validation.

## Windows desktop acceptance before stable promotion

- [ ] Install public v1.0.3 machine-wide, then exercise the v1.1.0 machine update path.
- [ ] From public v1.0.3 machine-wide, choose **Update tanpa Admin** and confirm the per-user copy is health-checked before the one-time old-install UAC removal.
- [ ] Confirm a subsequent per-user update path does not request Administrator permission.
- [ ] Cancel the old-install UAC prompt and confirm the original installation remains usable.
- [ ] Exercise a failed/interrupted download and confirm retry preserves the selected install scope.
- [ ] Exercise rollback/recovery failure fixtures or equivalent controlled acceptance and inspect updater logs.
- [ ] Confirm Documents presets, QSettings preferences, and official preset cache survive update/migration.
- [ ] Confirm successful migration leaves only one active SonKuPik installation/uninstall registration.
- [ ] Confirm portable ZIP remains independent and is never silently converted to installed mode.

## Promotion rule

Stable v1.1.0 publishing is blocked while the token above is `pending`. After the exact RC is accepted, change only this record and any final release notes in a separate reviewed PR so the token reads:

the value `accepted`.

The stable workflow must parse the machine-readable token as an exact anchored line before publishing any v1.1+ stable tag.