# Security Policy

SONKUPIK STUDIO controls real audio-processing hardware and includes permanent device-write operations. Treat protocol integrity and destructive preset writes as safety-sensitive project surfaces.

## Supported versions

Until stable `v1.0`, only the latest published release candidate and current `main` are maintained.

## Reporting a vulnerability

Please do **not** post secrets, private device captures containing unrelated personal data, or credentials in a public issue.

For ordinary reproducible crashes, parser failures, connection failures or device-write regressions, open a GitHub issue and attach a P5 Support Report JSON when possible. The report intentionally excludes active-memory contents, preset bytes and preset file paths.

For a vulnerability that should not be public before a fix exists, use GitHub's private vulnerability reporting / Security Advisory flow when enabled for the repository.

Include:

- affected SONKUPIK STUDIO version and commit;
- Windows version;
- USB HID or Bluetooth SPP transport;
- minimum reproducible steps;
- whether permanent device storage was touched;
- expected vs actual behavior;
- relevant protocol trace/support report with unnecessary private data removed.

## Hardware-write safety policy

A change must not:

- invent or guess an unverified K500 command/offset;
- replace device-owned unknown bytes with editor defaults;
- enable LIVE before authoritative hydration finishes;
- bypass `K500DeviceManager` to own a second transport handle;
- weaken P0–P4 regression checks merely to make new code pass;
- report a destructive Store/Upload transaction as successful after an uncertain transport failure.

Unknown protocol behavior should remain read-only or unsupported until donor/capture evidence exists.

## Release status

P5 release candidates are software-verified but physical K500 acceptance remains a separate gate. See `docs/P5_RELEASE_READINESS.md` and `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md`.
