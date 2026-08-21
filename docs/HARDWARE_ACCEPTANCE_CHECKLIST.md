# K500 Hardware Acceptance Checklist

Use this checklist before promoting any hardware-facing row in `PORTING_PARITY_MATRIX.md` to `LOCKED ✅`.

## Test environment

Record for every session:

- K500 firmware/version:
- Connection: USB HID / Bluetooth SPP:
- Windows version:
- SONKUPIK STUDIO commit SHA:
- Donor/web version or capture reference used for comparison:
- Tester/date:

## P0 baseline reconnect test

1. Start with manufacturer software closed.
2. Connect K500.
3. Select the intended transport.
4. Press CONNECT.
5. Confirm CONNECT/SYNC becomes ONLINE/LIVE only after full readback.
6. Confirm current K500 values appear in the Qt UI before touching any control.
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
| All HPF/LPF | [ ] | [ ] | [ ] | [ ] |
| Main output block | [ ] | [ ] | [ ] | [ ] |
| Surround output block/delay | [ ] | [ ] | [ ] | [ ] |
| Center output block | [ ] | [ ] | [ ] | [ ] |
| Sub output block | [ ] | [ ] | [ ] | [ ] |
| Mic EQ Link | [ ] | [ ] | [ ] | [ ] |
| Mute/media | [ ] | [ ] | N/A | [ ] |

## Device preset management gate (P2)

### Recall

- [ ] LIVE edits are paused during recall.
- [ ] Recall command/handshake completes.
- [ ] Full 939-byte readback runs again.
- [ ] UI reflects recalled slot before LIVE resumes.
- [ ] No stale queued write is emitted after recall.

### Permanent Store

- [ ] Store Begin accepted.
- [ ] Every chunk accepted in order.
- [ ] Store Commit accepted.
- [ ] Application resyncs after completion/failure.
- [ ] Power-cycle confirms intended slot persists.
- [ ] Other slots remain unchanged.

Mass Upload may not be accepted until single-slot Store passes the full checklist repeatedly.

## Failure/recovery tests

- [ ] Unplug USB while LIVE: app becomes offline, no crash, no further writes.
- [ ] Drop Bluetooth while LIVE: queue clears, LIVE OFF, no crash.
- [ ] Reconnect: full device readback occurs again.
- [ ] Malformed/short readback: connection is rejected; partial state is not promoted to LIVE.
- [ ] Unsupported UI path: no guessed frame is sent.

## Acceptance record

Attach trace/log/capture references to the PR or release note. A checked box without a reproducible commit SHA and transport is not sufficient evidence for `LOCKED ✅` status.
