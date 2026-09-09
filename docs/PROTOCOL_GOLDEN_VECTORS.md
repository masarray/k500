# K500 Protocol Golden Vectors

These vectors freeze the accepted native SonKuPik K500 protocol behavior. They are implementation contracts, not examples to be regenerated heuristically.

**v1.0 support scope:** Windows x64 + USB HID. Bluetooth vectors remain documented because the transport is implemented, but Bluetooth itself is experimental until independently hardware-qualified.

## Frame rules

- Bluetooth/shared frame: `AA len8 body checksum`.
- USB HID conversion: `AA len16LE body checksum`.
- Checksum is chosen so the sum from the length byte through checksum is `0 mod 256`.
- Bluetooth `CMD 0x40` final mode byte: `0x63`.
- USB `CMD 0x40` final mode byte: `0x00`.

## Connection and transport

```text
Heartbeat BT          AA 01 1C E3
Heartbeat USB         AA 01 00 1C E3
Handshake             AA 01 3F C0
Read 0x0000/0x3A BT   AA 06 40 00 00 3A 00 63 1D
Read 0x0000/0x3A USB  AA 06 40 00 00 3A 00 00 80
Read final BT          AA 06 40 A0 03 0B 00 63 A9
Read final USB         AA 06 40 A0 03 0B 00 00 0C
```

The active-memory snapshot is exactly `0x03AB` / **939 bytes**: sixteen 58-byte blocks followed by one 11-byte final block.

## Mute and media

```text
Mute OFF      AA 03 15 00 00 E8
Mute ON       AA 03 15 01 00 E7
Rewind        AA 03 06 00 05 F2
Forward       AA 03 06 01 05 F1
Play/Pause    AA 03 06 02 05 F0
```

## PEQ

Reference edit: band index 2, 355 Hz, Q 1.0, -11.1 dB Bell.

```text
Music EQ      AA 09 03 02 02 63 01 0A 80 6F 60 33
Mic A EQ      AA 09 03 00 02 63 01 0A 80 6F 00 95
Sub EQ        AA 09 03 08 02 63 01 0A 80 6F 00 8D
```

## Crossover `CMD 0x11`

Reference 1000 Hz, Butterworth 12 unless noted:

```text
Music HPF 95 Hz, device state 0x32  AA 06 11 02 02 5F 00 32 54
Mic HPF                              AA 06 11 00 02 E8 03 00 FC
Main LPF                             AA 06 11 05 02 E8 03 00 F7
Reverb HPF                           AA 06 11 06 02 E8 03 00 F6
Surround LPF                         AA 06 11 09 02 E8 03 00 F3
Echo HPF                             AA 06 11 0A 02 E8 03 00 F2
Center LPF                           AA 06 11 0D 02 E8 03 00 EF
Sub LPF                              AA 06 11 0F 02 E8 03 00 ED
```

Music uses current scalar `0x1B` as the final state byte. Verified non-Music crossover writes use `0x00`.

## Top Music mirrored-scalar safety

`CMD 0x02` is a block write. Rarely edited fields are seeded from device scalar cache, never stale UI defaults. Regression tests deliberately provide different cached values and verify preservation.

## P1 Top Mic CMD 0x05

Reference state:

```text
AA 0E 05 23 19 54 0B 07 07 60 60 26 03 0A 02 00 4F
```

Body layout after command byte:

```text
[topMicVol] [micInit mirrored] [micMax mirrored] [gate mirrored]
[fbxA] [fbxB] [micA] [micB] [TH+50] [ratio] [attack] [release*10] [00]
```

The final `00` is explicitly **not** EQ Link.

## P1 Top Effect CMD 0x09

Reference master effect 49, init 25:

```text
AA 03 09 31 19 AA
```

The init byte is mirrored from current device scalar `0x15`.

## P1 Mic EQ Link

```text
OFF  AA 04 3C 00 00 C4 FC
ON   AA 04 3C 01 01 9E 20
```

These tail bytes are captured command data and must not be regenerated heuristically.

## P1 output blocks CMD 0x0E

Shared structure:

```text
AA 25 0E [section] [35-byte data image] checksum
```

Section IDs:

```text
Main      00
Surround  02
Center    04
Sub       05
```

The 35-byte image is seeded from current K500 readback. SonKuPik K500 patches only donor-verified fields; every unknown/reserved byte stays device truth.

Golden reference frames:

```text
Main
AA 25 0E 00 63 00 5F 00 5B 00 57 00 53 00 4F 00 2F 12 07 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 6E

Surround (L=3 ms, R=4 ms delay)
AA 25 0E 02 63 00 61 00 57 00 55 00 50 00 4B 00 1E 64 01 01 03 00 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 35

Center
AA 25 0E 04 63 00 00 00 58 00 56 00 54 00 52 00 2E 0A 05 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 D3

Sub
AA 25 0E 05 5D 00 00 00 46 00 5A 00 3C 00 32 00 28 08 04 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 26
```

The regression self-test seeds unknown bytes with sentinel values and verifies untouched positions survive a block edit. This is a destructive-write safety guard.

## Recall / Use Init Volume

The route byte is a **device destination mask**, not the current physical PC cable. Donor/native captures use `0x03` for Recall and Use Init.

Shared/BT-style builders:

```text
Recall slot 1, mask 03       AA 03 01 00 03 F9
Recall refresh handshake     AA 03 3F 00 03 BB
Use Init OFF, mask 03        AA 03 12 00 03 E8
Use Init ON,  mask 03        AA 03 12 01 03 E7
```

USB framing inserts the 16-bit length high byte. Captured-equivalent Recall slot 4 to USB destination:

```text
AA 03 00 01 03 01 F8
```

Recall transaction:

```text
CMD 0x01 -> settle 80 ms -> CMD 0x3F -> require RSP 0xC0
-> full 939-byte active-memory readback -> hydrate -> LIVE
```

Use Init Volume requires `RSP 0xED`.

## Permanent Store

Native slot image length is `0x0290` = **656 bytes**. Chunk length is `0x003C` = **60 bytes**, therefore each slot uses ten 60-byte chunks plus one final 56-byte chunk.

Single-slot Store:

```text
CMD 0x41 begin
CMD 0x42 chunk x11, each requires RSP 0xBD
CMD 0x43 commit, requires RSP 0xBC
```

The native single-slot capture does not require `0xBE` begin ACK; it waits 80 ms after `CMD 0x41` before chunks. **Mass Upload does require `RSP 0xBE` for each slot begin.**

Zero-image reference vectors:

```text
Store begin, 656-byte zero image
AA 08 41 90 02 00 00 00 00 00 25

First 60-byte zero chunk, offset 0000
AA 45 42 00 00 3C 00 [60x00] 00 00 00 00 3D

Final 56-byte zero chunk, offset 0258
AA 41 42 58 02 38 00 [56x00] 00 00 00 00 EB

Commit slot 1, zero image
AA 07 43 00 00 38 00 00 00 7E
```

Mass-upload chain vector with final slot-image bytes `12 34` and chain input `12 34 56`:

```text
AA 08 41 90 02 BA 00 12 34 56 CF
```

Slot-1 commit:

```text
AA 07 43 00 00 38 00 12 34 38
```

Next chain becomes `[12, 34, 38]`: final image byte 0, final image byte 1, previous commit checksum.

## Integrated source-of-truth rules

### Current-device Save

Immediately before permanent current-device Save, the application obtains fresh device truth and uses the proven native active-memory/slot-image path. It does not serialize stale UI defaults.

### PC preset Upload

A validated `.k500` file is converted through `K500PresetCodec::buildDeviceSlotImage()` into a 656-byte native slot image. A permanent slot image is **not** a raw `file.left(0x0290)` slice.

The PC image is then passed to `K500PresetManager`; a pre-Store hardware readback must not replace the selected PC preset image. After commit, the destination is recalled and hardware is re-read before LIVE resumes.

### Mass Upload

Mass Upload accepts a fully validated batch of converted 656-byte images, sorts selected destinations highest-to-lowest, follows the native inter-slot chain, and recalls Slot 01 after a full bank before authoritative readback.

This file/preset integration is part of the v1 stable baseline; P3/P4 are no longer future dependencies.

## Explicit non-support boundary

No verified live/persistent write is invented for:

- Mic gate detail where donor/native command semantics remain unproven;
- detailed Reverb/Echo timing/level fields without a verified command;
- persistent LCD/Equipment Mode Name rename.

Such fields remain read-only/unsupported until packet evidence exists.

## Change policy

A golden vector may change only when the same focused PR includes:

1. donor implementation or packet-capture evidence explaining the change;
2. the updated vector;
3. regression-test update;
4. physical hardware evidence when the support claim changes;
5. parity-matrix/documentation update.

Never weaken a vector or preservation test merely to make a changed implementation pass.
