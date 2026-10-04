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

## Music Tone + Mic LP readback — 2026-10-04

- [ ] Reconnect with Music Noise Gate OFF hydrates OFF from direct `activeMemory[0x0005]=0`.
- [ ] Reconnect at -90 dB and -50 dB hydrates -90/-50 from raw 1/41 without any startup replay write.
- [ ] Editing an unrelated Top-Music field preserves the hardware Noise Gate value; no stale `0x001B` fallback is replayed as the gate.
- [ ] The unresolved Top-Music trailing byte is not presented as capture-mapped truth and is not remapped from one-state correlation.
- [ ] Reconnect Music Bass at -12/0/+12 dB hydrates -12/0/+12 from direct `activeMemory[0x00DF]=0/120/240`.
- [ ] Reconnect Mic LP at Bypass/Bessel12/Butter12/Bessel18/Butter18/Butter24 hydrates enum 0/1/2/3/4/5 from direct `activeMemory[0x0014]`.
- [ ] Mic A and Mic B LP presentation follow the shared Mic LP device field while hydration emits zero `stateEdited` events.
- [ ] Mic HP presentation is not falsely promoted from a neighboring byte; it remains evidence-gated.
- [ ] Mic LP type edits preserve current Music Input1 Gain raw in the final CMD `0x11` byte: -3 dB -> `0x09`, 0 dB -> `0x0C`.
- [ ] Change Input2/BT gain while Input1 is unchanged and confirm the Mic CMD `0x11` tail does not follow those controls.
- [ ] After a live Input1 edit, a later Mic HP/LP edit must not roll Input1 back to its connect-time value.

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

## Capture-mapped System controls — 2026-10-04

- [ ] Music Init 0/25/84 writes change only the intended CMD 0x02 scalar and later Top Music edits do not roll it back.
- [ ] Mic Init 0/25/84 writes change only the intended CMD 0x05 scalar and later Top Mic edits do not roll it back.
- [ ] Effect Init 0/25/84 uses CMD 0x0A / RSP 0xF5 and later Top Effect edits preserve the new Init value.
- [ ] USB Record 1/4/6 uses CMD 0x3E selector 0x03 and reconnect readback returns the same UI value.
- [ ] Mic Max and UDisk Record use their capture-mapped live writes and reconnect to the same device values.
- [ ] Adj Manner VR OFF ON/OFF receives RSP 0xF8.
- [ ] After reconnect, Adj Manner VR OFF returns to DEVICE STATE UNKNOWN rather than claiming a stale local value.
- [ ] With Use Init ON, Recall Mode 01 applies stored Music/Mic/Effect Init values to the three active masters.
- [ ] With Use Init OFF, Recall does not locally synthesize those master values; the UI follows the 939-byte K500 readback.

## Final daily-use operational controls — 2026-10-04

### Mic Max

- [ ] Mic Max 84, 50, 30 and 0 produce the captured Top Mic CMD 0x05 layout.
- [ ] Lowering Mic Max below current Master Mic clamps Master Mic in the same device write.
- [ ] Raising Mic Max does not raise Master Mic automatically.
- [ ] Programmatic Master Mic edits above Mic Max are clamped before serialization and canonical DesiredState.
- [ ] Reconnect/readback restores the same Mic Max and Master Mic values.

### Recording volumes

- [ ] UDisk Record UI 1/4/6 uses the captured CMD 0x3E `<raw> 00 00` form.
- [ ] USB Record UI 1/4/6 keeps the distinct captured CMD 0x3E `03 <raw> 54` form.
- [ ] Editing UDisk Record does not alter USB Record.
- [ ] Editing USB Record does not alter UDisk Record.
- [ ] Full reconnect/readback restores both hardware values.

### Dance Mic trigger

- [ ] Threshold -60/-50/0 dB serializes as raw 0/10/60 in CMD 0x22.
- [ ] Hold Time 1/6/30 s serializes as raw seconds in CMD 0x22.
- [ ] Editing Threshold preserves the current hardware Hold Time in the same pair frame.
- [ ] Editing Hold Time preserves the current hardware Threshold in the same pair frame.
- [ ] RSP 0xDD is observed for accepted native writes.
- [ ] If the paired 0x0093/0x0094 seed does not decode inside captured ranges, both controls stay disabled and no guessed CMD 0x22 is sent.
- [ ] Reconnect on normal hardware produces a valid paired seed and the UI matches the native application.

### BT identity

- [ ] BT Name `ARI` sends captured CMD 0x4E SET and receives RSP 0xB1.
- [ ] BT Reset sends captured CMD 0x4E RESET and receives RSP 0xB1.
- [ ] Rename accepts only 1..8 printable ASCII characters.
- [ ] BT identity actions are available only on the USB transport qualified by the capture.
- [ ] After ACK, a full 939-byte readback completes before normal LIVE operation resumes.
- [ ] UI displays the BT Name returned by hardware readback, not an optimistic local string.
- [ ] Reconnect confirms the BT Name/Reset result.
- [ ] BLE Name remains unchanged and read-only.

### Credential scope

- [ ] Lock/password and Admin/User credential controls remain unavailable.
- [ ] No credential command is guessed or emitted.
- [ ] These device-managed controls are not release-blocking for the daily-use operational scope.

## Output Delay — final physical capture batch

- [ ] Main L Delay 0/20/50 ms writes only data[16..17] of Main CMD 0x0E.
- [ ] Main R Delay 0/20/50 ms writes only data[18..19] of Main CMD 0x0E.
- [ ] Main L/R readback after reconnect matches the native application.
- [ ] With the documented native reference state, reconnect hydrates Surround **L=14 ms / 4.8 m** and **R=20 ms / 6.8 m** without swapping them.
- [ ] Surround **L** Delay writes native data[18..19], not data[16..17].
- [ ] Surround **R** Delay writes native data[16..17], not data[18..19].
- [ ] Surround L/R labels remain semantically correct in SonKuPik despite the reversed wire order.
- [ ] Center Output Delay 0/20/30/40/50 ms writes only data[16..17].
- [ ] Subwoofer Output Delay 0/20/30/40/50 ms writes only data[16..17].
- [ ] All six controls clamp to the native 0..50 ms range.
- [ ] Device ACK is RSP 0xF1 for accepted Output block writes.
- [ ] Volume, mixer, compressor, HPF/LPF and every reserved byte remain unchanged while editing delay.
- [ ] Reconnect restores Main L/R, Surround L/R, Center and Sub delay from hardware truth.
- [ ] Native distance display equivalence is sensible: 20 ms ≈ 6.8 m and 50 ms ≈ 17.0 m; distance is not transmitted separately.

## Persistent Equipment Mode rename

Use a sacrificial active slot and a <=16-character printable-ASCII test name.

- [ ] Rename is enabled only for the ACTIVE slot and only on USB Store transport.
- [ ] A fresh 939-byte readback occurs before the Store transaction.
- [ ] Only slot-image bytes 0x0280..0x028F differ before Store.
- [ ] CMD 0x41 -> eleven CMD 0x42 chunks -> CMD 0x43 completes with expected ACKs.
- [ ] The same slot is recalled and a full 939-byte readback completes before LIVE resumes.
- [ ] Device slot table displays the renamed value from hardware readback.
- [ ] Reconnect and power-cycle confirm persistence.
- [ ] DSP/audio settings in the renamed slot remain unchanged.
- [ ] Non-target slots remain unchanged.

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
resources/presets/01_KONSER_NYANYI.k500
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
