# K500 Hardware Acceptance & Regression Checklist

Use this runbook for any change that can affect real K500 behavior. The public v1.0 support baseline is **Windows 10/11 x64 + USB HID**. Bluetooth SPP is implemented but remains experimental until independently qualified.

Stable v1.0 is already published; this checklist now protects future changes from silently regressing that accepted baseline.

## Test record

Record for every physical session:

- SonKuPik K500 version:
- exact commit SHA:
- K500 firmware/version:
- Windows version:
- transport: USB HID / Bluetooth SPP:
- test date and tester:
- donor/capture reference when protocol behavior is involved:
- Support Report / trace / capture reference:
- affected source/destination slots for preset operations:

A check mark without a reproducible version/commit and transport is not sufficient evidence for a hardware-facing promotion.

## Baseline connect / reconnect

1. Close manufacturer software and any process that can own the K500 transport.
2. Connect K500.
3. Select the intended transport and press CONNECT.
4. Confirm ONLINE/LIVE appears only after handshake and full device readback.
5. Confirm current K500 values hydrate the editor before any control is touched.
6. Change one verified control and confirm expected hardware behavior.
7. Disconnect/reconnect.
8. Confirm the K500 is authoritative again; stale editor values must not replay into hardware.

Expected USB truth path:

```text
heartbeat -> handshake -> CMD 0x40 blocks -> 939-byte hydration -> LIVE ON
```

## Destructive-write safety rule

For a new or modified write path:

1. establish before-state from device/file truth;
2. change exactly the intended target;
3. verify the target changed;
4. verify neighboring/non-target values did not change;
5. verify failure/timeout behavior;
6. reconnect and verify resulting state;
7. power-cycle when persistence is part of the claim.

Unexpected neighboring changes are an acceptance failure even if the intended target appears to work.

## LIVE control regression matrix

Test USB for every stable release that materially changes these paths. Use the Bluetooth column only for Bluetooth qualification work.

| Control family | USB stable regression | Bluetooth qualification | Reconnect/readback | Non-target preserved |
|---|---:|---:|---:|---:|
| Master Music / inputs / key | [ ] | [ ] | [ ] | [ ] |
| Master Mic / Mic A/B | [ ] | [ ] | [ ] | [ ] |
| Mic dynamics / FBX | [ ] | [ ] | [ ] | [ ] |
| Master Effect | [ ] | [ ] | [ ] | [ ] |
| Music / Mic PEQ | [ ] | [ ] | [ ] | [ ] |
| Main / Surround / Center / Sub PEQ | [ ] | [ ] | [ ] | [ ] |
| Reverb / Echo PEQ | [ ] | [ ] | [ ] | [ ] |
| Verified HPF/LPF selectors | [ ] | [ ] | [ ] | [ ] |
| Main output block | [ ] | [ ] | [ ] | [ ] |
| Surround output block / delay | [ ] | [ ] | [ ] | [ ] |
| Center / Sub output block | [ ] | [ ] | [ ] | [ ] |
| Mic EQ Link | [ ] | [ ] | [ ] | [ ] |
| Mute / media | [ ] | [ ] | N/A | [ ] |

Fields that remain read-only in the parity matrix are not acceptance failures; they are intentional protocol boundaries.

## Section-navigation crash regression

The v1 baseline uses fixed EQ graph/model lifetimes. If a change touches QML section/workspace lifecycle, run repeated transitions including:

```text
Mic A -> Reverb -> Mic B -> Reverb -> Echo -> Main -> Surround -> Center -> Sub -> System
```

Repeat the sequence rapidly and with a connected K500. There must be no freeze, close, heap corruption, stale-band selection, or unintended hardware write.

## Equipment Mode Recall

- [ ] LIVE is paused during Recall.
- [ ] Selected slot command completes.
- [ ] `CMD 0x3F` refresh handshake completes and `RSP 0xC0` is received.
- [ ] Full 939-byte readback completes.
- [ ] UI reflects hardware slot/readback before LIVE resumes.
- [ ] No stale queued write is emitted afterward.
- [ ] Repeat across multiple slots.

## Use Init Volume

- [ ] ON receives expected `RSP 0xED` behavior.
- [ ] OFF receives expected `RSP 0xED` behavior.
- [ ] Failure/timeout does not leave UI claiming an unverified state.
- [ ] Reconnect confirms expected device behavior.

## Current-device permanent Save

- [ ] USB HID is used; destructive Store remains gated appropriately.
- [ ] Fresh device truth is obtained before Save.
- [ ] Store Begin completes.
- [ ] Eleven chunks complete in order.
- [ ] Store Commit completes.
- [ ] Recalled saved slot contains intended values.
- [ ] Reconnect confirms state.
- [ ] Power-cycle confirms persistence when persistence behavior changed.
- [ ] Other slots/non-target data remain unchanged.

## PC preset single Upload

Use a known `.k500` and record SHA-256.

- [ ] File passes exact size/checksum validation.
- [ ] Staging the file does not alter current K500 audio/editor/slot state.
- [ ] Preview is explicit rather than implied by selection.
- [ ] Destination slot is unambiguous.
- [ ] Converted PC slot image is used; a device readback does not replace it before Store.
- [ ] Store Begin/Chunk/Commit completes.
- [ ] Destination slot is recalled after commit.
- [ ] Full 939-byte readback occurs before LIVE resumes.
- [ ] Reconnect confirms the same uploaded values.
- [ ] Power-cycle confirms persistence when required by the change under test.
- [ ] Source `.k500` remains unchanged unless an explicit file edit was saved.
- [ ] Non-target device slots remain unchanged.

## Unified Official + Local preset library

- [ ] Mass Upload collection contains bundled **SONKUPIK** presets without requiring network or a Local folder.
- [ ] Selecting a Local folder adds valid **LOCAL** entries to the same collection.
- [ ] Official and Local entries remain visibly distinguishable.
- [ ] Official sync never overwrites Local files.
- [ ] Failed network sync leaves bundled/last-known-good official entries usable.
- [ ] Invalid remote `.k500` cannot replace a valid official cache entry.
- [ ] A newly valid official preset can appear after Sync without reinstalling the app.

### Mode 01 golden donor

Before any stable release or official Mode 01 change, verify:

```text
resources/presets/01_ALL_GENRE.k500
internal name: CONCERT HIFI V4
SHA-256: 9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

A valid checksum alone is not sufficient evidence of donor identity.

## Multi-file Mass Upload

### Transfer-list staging

- [ ] Unified PC collection is not artificially capped at 10.
- [ ] Add / Add All / Remove / Clear do not touch hardware.
- [ ] Right list accepts at most 10 destination entries.
- [ ] Right row 1 maps to Slot 01 ... row 10 to Slot 10.
- [ ] Official and Local presets can be mixed in one batch.
- [ ] Entire batch validates before the first Store command.
- [ ] One invalid member aborts before hardware writes begin.

### Native hardware sequence

- [ ] Full bank starts at Slot 10 and ends at Slot 01.
- [ ] Partial batch also executes highest selected destination first.
- [ ] Each Mass Upload begin receives the expected begin ACK before chunks.
- [ ] Each slot sends ten 60-byte chunks + one final 56-byte chunk.
- [ ] Every chunk/commit is acknowledged according to the golden protocol.
- [ ] Native inter-slot chain remains correct.
- [ ] No slot is reported complete before commit ACK.
- [ ] After final Slot 01 commit, Slot 01 is recalled.
- [ ] `RSP 0xC0` + full 939-byte refresh complete before LIVE.

### Correctness

- [ ] Recall uploaded Slot 01 and confirm music/output behavior is normal.
- [ ] Recall other uploaded slots and compare against intended source presets.
- [ ] Device Mode names come from hardware readback, not PC staging labels.
- [ ] Reconnect confirms device truth.
- [ ] Power-cycle confirms persistence when required.
- [ ] Slots outside the batch remain unchanged.

## Failure / recovery injection

- [ ] Unplug USB while LIVE: app becomes offline/error, does not crash, sends no further writes.
- [ ] Reconnect triggers full device hydration again.
- [ ] Malformed/short readback is never promoted to LIVE.
- [ ] Unsupported UI path sends no guessed frame.
- [ ] Unplug USB during Recall: transaction fails closed.
- [ ] Unplug USB during Save: transaction fails closed.
- [ ] Unplug USB during single Upload: transaction fails closed.
- [ ] Unplug USB during Mass Upload: transaction fails closed and never claims complete success.

## Package acceptance

Test both artifacts from the same exact commit:

- [ ] Inno Setup installer launches and application starts.
- [ ] Portable ZIP extracts and application starts.
- [ ] USB connect works from installed build.
- [ ] USB connect works from portable build.
- [ ] Section navigation remains crash-free in both packages.
- [ ] Support Report saves and contains version/OS/transport/status/log metadata.
- [ ] Support Report contains no full active-memory bytes, preset payloads, or local preset paths.
- [ ] `SHA256SUMS.txt` matches the downloaded artifacts.
- [ ] `release-manifest.json` matches exact version/commit and support scope.

Bluetooth package testing is required only when qualifying or changing Bluetooth behavior; it does not inherit USB acceptance automatically.

## Acceptance record

Attach the trace/support-report/capture reference to the relevant PR, issue, or release qualification record. Promote a hardware-facing claim only when the evidence matches the exact code and transport being promoted.
