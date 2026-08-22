# K500 Hardware Acceptance Checklist

Use this checklist before promoting any hardware-facing row in `PORTING_PARITY_MATRIX.md` to `LOCKED ✅` or publishing a stable `v1.0`.

## Test environment

Record for every session:

- K500 firmware/version:
- Connection: USB HID / Bluetooth SPP:
- Windows version:
- SONKUPIK STUDIO version:
- SONKUPIK STUDIO commit SHA:
- Donor/web version or capture reference used for comparison:
- Tester/date:
- Support-report filename / trace reference:

## P0 baseline reconnect test

1. Start with manufacturer software closed.
2. Connect K500.
3. Select the intended transport.
4. Press CONNECT.
5. Confirm CONNECT/SYNC becomes ONLINE/LIVE only after full readback.
6. Confirm current K500 values appear in Qt before touching any control.
7. Disconnect and reconnect.
8. Confirm the device is source of truth again; stale editor values must not overwrite it.

Expected trace order:

```text
heartbeat -> handshake -> CMD 0x40 blocks 0x0000..0x03AA -> 939-byte hydration -> LIVE ON
```

## Destructive-write safety rule

For every new block command:

1. Capture/read all neighboring fields before editing.
2. Change exactly one target control.
3. Verify the intended field changes on hardware.
4. Verify every non-target field remains unchanged.
5. Disconnect/reconnect and verify persistence semantics are exactly as expected.

If a neighboring field changes unexpectedly, the feature fails acceptance even when the target control appears to work.

## LIVE control matrix

Mark each transport independently.

| Control family | USB | Bluetooth | Reconnect readback | Non-target fields preserved |
|---|---:|---:|---:|---:|
| Master Music | [ ] | [ ] | [ ] | [ ] |
| Music input trims/key | [ ] | [ ] | [ ] | [ ] |
| Master Mic | [ ] | [ ] | [ ] | [ ] |
| Mic A/B | [ ] | [ ] | [ ] | [ ] |
| Mic dynamics / FBX | [ ] | [ ] | [ ] | [ ] |
| Master Effect | [ ] | [ ] | [ ] | [ ] |
| Music PEQ | [ ] | [ ] | [ ] | [ ] |
| Mic A/B PEQ | [ ] | [ ] | [ ] | [ ] |
| Main PEQ | [ ] | [ ] | [ ] | [ ] |
| Surround PEQ | [ ] | [ ] | [ ] | [ ] |
| Center PEQ | [ ] | [ ] | [ ] | [ ] |
| Sub PEQ | [ ] | [ ] | [ ] | [ ] |
| Reverb PEQ | [ ] | [ ] | [ ] | [ ] |
| Echo PEQ | [ ] | [ ] | [ ] | [ ] |
| All verified HPF/LPF selectors | [ ] | [ ] | [ ] | [ ] |
| Main output block | [ ] | [ ] | [ ] | [ ] |
| Surround output block/delay | [ ] | [ ] | [ ] | [ ] |
| Center output block | [ ] | [ ] | [ ] | [ ] |
| Sub output block | [ ] | [ ] | [ ] | [ ] |
| Mic EQ Link | [ ] | [ ] | [ ] | [ ] |
| Mute/media | [ ] | [ ] | N/A | [ ] |

## Device preset management gate — P2

### Recall

- [ ] LIVE edits are paused during recall.
- [ ] Recall command/handshake completes.
- [ ] Full 939-byte readback runs again.
- [ ] UI reflects recalled slot before LIVE resumes.
- [ ] No stale queued write is emitted after recall.
- [ ] Repeat on at least three different slots.

### Use Init Volume

- [ ] Toggle ON receives expected device ACK and UI remains ON.
- [ ] Toggle OFF receives expected device ACK and UI remains OFF.
- [ ] Failure/timeout rolls back UI state.
- [ ] Reconnect confirms the device-side behavior expected from the donor.

### Current-device permanent Save

- [ ] USB HID is used; operation is rejected over Bluetooth.
- [ ] Fresh device readback occurs before Store.
- [ ] Store Begin accepted.
- [ ] Every chunk accepted in order.
- [ ] Store Commit accepted.
- [ ] Application returns to a valid LIVE state.
- [ ] Recall of the saved slot returns the intended values.
- [ ] Power-cycle confirms the intended slot persists.
- [ ] Other slots remain unchanged.

## PC preset permanent Upload — P4

Use a known `.k500` fixture and record its SHA-256 before testing.

- [ ] File loads only when exact size/checksum are valid.
- [ ] Upload is enabled only with USB store availability and idle transaction manager.
- [ ] Selected destination slot is clearly known before starting.
- [ ] PC preset image is used directly; no pre-Store device readback replaces it.
- [ ] Store Begin/Chunk/Commit completes without timeout.
- [ ] Recall destination slot and confirm values match the source preset.
- [ ] Disconnect/reconnect and confirm the same values.
- [ ] Power-cycle K500 and confirm persistence.
- [ ] Source `.k500` remains unchanged by Upload.
- [ ] Non-target device slots remain unchanged.

## Multi-file Mass Upload — P4.2

Start with 2–3 distinct known presets before attempting all 10 slots.

- [ ] File selection is validated before any device write.
- [ ] Invalid checksum in one member aborts the entire batch before Store begins.
- [ ] Filename sort / sequential selected-slot mapping matches the UI expectation.
- [ ] Native device write order is descending by destination slot.
- [ ] Each Store Begin receives `RSP 0xBE`.
- [ ] Every chunk receives `RSP 0xBD`.
- [ ] Every commit receives `RSP 0xBC`.
- [ ] Three-byte Store chain remains continuous across entries.
- [ ] Batch completion refresh/recall behavior matches donor semantics.
- [ ] Recall each uploaded slot and compare against its source file.
- [ ] Disconnect/reconnect and repeat comparison.
- [ ] Power-cycle K500 and repeat comparison.
- [ ] Slots outside the batch remain unchanged.

## Failure/recovery tests

- [ ] Unplug USB while LIVE: app becomes offline/error, no crash, no further writes.
- [ ] Drop Bluetooth while LIVE: queue clears, LIVE OFF, no crash.
- [ ] Reconnect: full device readback occurs again.
- [ ] Malformed/short readback: connection is rejected; partial state is not promoted to LIVE.
- [ ] Unsupported UI path: no guessed frame is sent.
- [ ] Unplug USB during Recall: transaction fails closed and requires reconnect.
- [ ] Unplug USB during current-device Save: transaction fails closed and requires reconnect.
- [ ] Unplug USB during PC Upload: transaction fails closed and requires reconnect.
- [ ] Unplug USB during Mass Upload: transaction fails closed; never claim partial batch success.

## P5 release-candidate package acceptance

Test both artifacts generated from the exact same commit:

- [ ] Installer launches and passes basic UI smoke test.
- [ ] Portable single EXE launches and passes basic UI smoke test.
- [ ] USB connect works from installer package.
- [ ] USB connect works from portable package.
- [ ] Bluetooth connect works from installer package.
- [ ] Bluetooth connect works from portable package.
- [ ] Support Report JSON saves successfully.
- [ ] Support Report contains version, OS, transport/status and bounded log.
- [ ] Support Report does **not** contain active-memory bytes, preset bytes or preset file paths.
- [ ] `SHA256SUMS.txt` matches downloaded artifacts.
- [ ] `release-manifest.json` commit/version match the tested build.
- [ ] Release manifest still says hardware acceptance `pending` until this checklist is fully evidenced.

## Acceptance record

Attach trace/support-report/capture references to the PR or release note. A checked box without a reproducible commit SHA and transport is not sufficient evidence for `LOCKED ✅` status.

When all mandatory hardware rows pass, update `PORTING_PARITY_MATRIX.md` in a separate evidence-backed PR. Do not silently promote hardware status as part of unrelated code cleanup.
