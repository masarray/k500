# K500 Protocol Golden Vectors

These vectors freeze the accepted native Qt protocol behavior. They are enforced by `K500Protocol::selfTest()`. P0 vectors protect connection/readback and the existing Music path; P1 adds only commands already verified in the donor/captures.

## Frame rules

- Bluetooth/shared frame: `AA len8 body checksum`.
- USB HID conversion: `AA len16LE body checksum`.
- Checksum is chosen so the sum from the length byte through checksum is `0 mod 256`.
- Bluetooth CMD `0x40` final mode byte: `0x63`.
- USB CMD `0x40` final mode byte: `0x00`.

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

The active-memory snapshot is exactly `0x03AB` bytes: 16 full 58-byte blocks followed by one 11-byte final block.

## Mute and media transport

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

## Crossover CMD 0x11

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

`CMD 0x02` is a block write. Rarely edited fields come from device scalar cache, never stale UI defaults. The self-test deliberately supplies different cache values and verifies preservation.

## P1 Top Mic CMD 0x05

Default/reference state from donor mapping:

```text
AA 0E 05 23 19 54 0B 07 07 60 60 26 03 0A 02 00 4F
```

Layout after command byte:

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

These tail bytes are part of the captured command and must not be regenerated heuristically.

## P1 output blocks CMD 0x0E

Shared body structure:

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

The 35-byte image is seeded from current K500 readback. Qt patches only donor-verified fields; every unknown/reserved byte must remain unchanged.

Golden reference frames:

```text
Main
AA 25 0E 00 63 00 5F 00 5B 00 57 00 53 00 4F 00 2F 12 07 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 6E

Surround (includes L=3 ms, R=4 ms delay)
AA 25 0E 02 63 00 61 00 57 00 55 00 50 00 4B 00 1E 64 01 01 03 00 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 35

Center
AA 25 0E 04 63 00 00 00 58 00 56 00 54 00 52 00 2E 0A 05 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 D3

Sub
AA 25 0E 05 5D 00 00 00 46 00 5A 00 3C 00 32 00 28 08 04 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 26
```

The self-test also seeds all unknown bytes with `0x5A` and verifies an untouched position remains `0x5A` after a Main edit. This is the destructive-write regression guard.

## Explicit non-support boundary

P1 does **not** invent commands for fields for which the donor has no verified live mapping. Mic gate and detailed Reverb/Echo level/timing parameters remain hardware read-only until packet evidence exists.

## Change policy

A golden vector may only change when all of the following are included in the same PR:

1. donor implementation or packet-capture evidence explaining the change;
2. updated golden vector;
3. regression test update;
4. hardware acceptance result when promoting the command to `LOCKED ✅`;
5. parity-matrix status update.

Do not weaken a test merely to make a changed implementation pass.
