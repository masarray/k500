# v1.1.2 Updater Release-Candidate Acceptance

> Release-specific fail-closed acceptance gate. Historical v1.1.0 and v1.1.1 acceptance records remain immutable.

UPDATER_V1_1_2_ACCEPTANCE=accepted
UPDATER_V1_1_2_ACCEPTED_TAG=v1.1.2-rc.2
UPDATER_V1_1_2_ACCEPTED_COMMIT=8e3be5d4e567463e5bae4aa911537417aeba11b1
UPDATER_V1_1_2_MACHINE_SHA256=decd7d8e54837d664fa586a7425b502faeec27dee280d9dc48f6f6989a23b8ab
UPDATER_V1_1_2_USER_SHA256=f9a1f29a9b209f47ffeb4f289b0d28b38a9df10764927dcef304397c65b3171b

## Candidate scope and owner-requested changes

Target Windows application version: **1.1.2** (candidate only until accepted).
Public stable remains **v1.1.1** until exact, verified v1.1.2 promotion.

1. Windows native title bar displays the executable's actual version.
2. Official UAUDIO companions 11–20 are embedded as a second *PC library*
   bank, derived exactly from 01–10 donors (only sourceRaw, name, checksum
   edited); hardware mode slots remain 01–10.
3. Mass Upload source list supports Ctrl/Shift multiselect and bounded batch Add.
4. EQ BYPASS active warning is ruby-red, with unchanged bypass protocol/semantics.

The prior official preset bank 01–10 must remain byte-for-byte identical.
All native K500 protocol, live device control, 10→1 mass transfer semantics,
and the existing updater lifecycle must remain unchanged.

## Immutable RC qualification

- Run the existing single PR/main CI workflow against exact current `main`
  source commit; all fast, Qt/MSVC, deployed-app, and applicable installer
  smoke tests must pass.
- Use the **manual** `windows-updater-rc.yml` with a previously unused
  `rc_number` to create `v1.1.2-rc.N`.
- Prove both Windows installers install/uninstall correctly, preserve user
  data and existing official cache, and pass the deep updater acceptance tests.
- The RC must be a GitHub *prerelease*, never the public latest route.
- Record the exact tag, full 40-char commit and both SHA-256 digests above.
- An actual Windows GUI check must confirm three new features and the title.
  Real-device K500 upload acceptance is required for public hardware claims.

## Final owner acceptance — 2026-10-10

The release owner confirmed RC.2 passed their Windows GUI and physical
K500 USB tests, including Connect/Readback, Recall, Store and Mass Upload,
and instructed release of public stable v1.1.2.

Evidence:
- [Main CI #38044817516](https://github.com/masarray/k500/actions/runs/38044817516): success on accepted source commit.
- [RC.2 workflow #38045317595](https://github.com/masarray/k500/actions/runs/38045317595): success including both installer scopes, install/uninstall,
  release regression, updater lifecycle, manifest and SHA validation.
- [Immutable RC.2 release](https://github.com/masarray/k500/releases/tag/v1.1.2-rc.2): SHA-256 for both installers agrees with the exact tokens above.

The stable workflow must reuse exactly these two verified installers,
without rebuilding them. This owner acceptance is not automated proof
that GitHub runners exercised physical K500 hardware.

## Promotion gate

`UPDATER_V1_1_2_ACCEPTANCE=accepted` is set only after full RC
qualification and release-owner acceptance. A separate gate-update PR must
record exact accepted identity before invoking the existing manual
`windows-updater-promote.yml`. Promotion downloads and checks the accepted
RC assets and publishes **identical binary bytes** under the stable tag.
Rebuilding them after acceptance, overwriting v1.1.1, or promoting while
the token is pending is prohibited.

## Website

Keep all landing fallback labels at **v1.1.1** until stable v1.1.2 really
exists. The existing `/api/release` must resolve current public stable. Update
the fallback HTML, docs and public download checks only after promotion.
