# v1.1.1 Updater Release-Candidate Acceptance

> This file is the fail-closed acceptance gate for the v1.1.1 patch release. The historical v1.1.0 acceptance record remains immutable in `docs/V1_1_UPDATER_RC_ACCEPTANCE.md`.

UPDATER_V1_1_1_ACCEPTANCE=pending
UPDATER_V1_1_1_ACCEPTED_TAG=pending
UPDATER_V1_1_1_ACCEPTED_COMMIT=pending
UPDATER_V1_1_1_MACHINE_SHA256=pending
UPDATER_V1_1_1_USER_SHA256=pending

## Candidate scope

Target application version: **1.1.1**.

The candidate must be built from one exact post-v1.1.0 `main` commit and include the already-merged, current-main application state. In particular, the release must preserve:

- the hardware-accepted Mode 01 `KONSER NYANYI` flagship preset;
- the hardware-accepted Mode 02 `MC HOST RADIO` flagship preset;
- premium/calm System toggle rows for Use Init Volume and VR / Trim Pot Off;
- native-parity VR OFF CMD 0x07 / RSP 0xF8 current-session behavior;
- authoritative connect/reconnect hydration from `activeMemory[0x008C]`;
- offline-edit handoff that preserves accepted toggle state across disconnect without auto-writing on reconnect;
- strict preset semantic/range validation and consolidated CI guards.

The public support scope does not expand: **Windows 10/11 x64 + USB HID** remains qualified; Bluetooth SPP remains experimental.

## Release-owner authorization

On 2026-10-08 the release owner authorized preparation and publication of v1.1.1 after testing the post-v1.1.0 System-toggle/VR OFF/disconnect behavior and accepting the current preset/UI state.

That authorization does **not** bypass provenance. Stable publication remains blocked until the exact RC:

1. is built from current `main`;
2. passes the RC workflow, machine/per-user installer runtime validation, consolidated regression suite, and deep updater lifecycle acceptance;
3. is published under one immutable `v1.1.1-rc.N` tag;
4. has its exact commit and both installer SHA-256 values recorded below by a reviewed acceptance commit.

## Acceptance update after RC qualification

When the exact RC passes all gates, replace the five machine-readable values at the top with:

- `UPDATER_V1_1_1_ACCEPTANCE=accepted`
- the immutable RC tag;
- the exact 40-character RC source commit;
- lowercase SHA-256 of the machine installer;
- lowercase SHA-256 of the per-user installer.

Stable promotion must download and verify those accepted RC assets and publish the **same installer bytes** under v1.1.1 stable filenames. Rebuilding after acceptance is not permitted.
