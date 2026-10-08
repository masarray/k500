# v1.1.2 Updater Release-Candidate Acceptance

> Release-specific fail-closed acceptance gate. Historical v1.1.0 and v1.1.1 acceptance records remain immutable.

UPDATER_V1_1_2_ACCEPTANCE=pending
UPDATER_V1_1_2_ACCEPTED_TAG=
UPDATER_V1_1_2_ACCEPTED_COMMIT=
UPDATER_V1_1_2_MACHINE_SHA256=
UPDATER_V1_1_2_USER_SHA256=

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
