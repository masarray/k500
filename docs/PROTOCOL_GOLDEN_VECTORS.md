# K500 Protocol Golden Vectors

These vectors freeze the **currently accepted native Qt baseline**. They are enforced by `K500Protocol::selfTest()` and are intentionally conservative: only behavior already present in the Qt implementation is locked here. New write commands belong in later phases and must arrive with their own donor/capture evidence.

## Frame rules

- Bluetooth/shared frame: `AA len8 body checksum`.
- USB HID conversion: `AA len16LE body checksum`.
- Checksum is chosen so the sum from the length byte through checksum is `0 mod 256`.
- Bluetooth CMD `0x40` final mode byte: `0x63`.
- USB CMD `0x40` final mode byte: `0x00`.

## Connection and transport

```text
Heartbeat BT       AA 01 1C E3
Heartbeat USB      AA 01 00 1C E3
Handshake          AA 01 3F C0
Read 0x0000/0x3A BT  AA 06 40 00 00 3A 00 63 1D
Read 0x0000/0x3A USB AA 06 40 00 00 3A 00 00 80
Final full-memory block BT  (offset 0x03A0, len 0x000B)
Final full-memory block USB (offset 0x03A0, len 0x000B)
```

The active-memory snapshot is exactly `0x03AB` bytes, so 16 full 58-byte blocks are followed by one 11-byte final block.

## Mute and media transport

```text
Mute OFF      AA 03 15 00 00 E8
Mute ON       AA 03 15 01 00 E7
Rewind        AA 03 06 00 05 F2
Forward       AA 03 06 01 05 F1
Play/Pause    AA 03 06 02 05 F0
```

## PEQ baseline

Reference edit: band index 2, 355 Hz, Q 1.0, -11.1 dB Bell.

```text
Music EQ      AA 09 03 02 02 63 01 0A 80 6F 60 33
Mic A EQ      AA 09 03 00 02 63 01 0A 80 6F 00 95
Sub EQ        AA 09 03 08 02 63 01 0A 80 6F 00 8D
```

These vectors lock the section IDs and the special Music target byte `0x60` without pretending later P1 hardware qualification is already complete.

## Crossover baseline

Music HPF reference: 95 Hz, Butterworth 12 dB/oct, current device state byte `0x32`:

```text
AA 06 11 02 02 5F 00 32 54
```

Additional selector checks are retained in code for the already-known donor selector table, but capture-derived vs inferred selector provenance must remain documented in the parity matrix/hardware checklist before a row becomes `LOCKED ✅`.

## Top Music mirrored-scalar safety

`CMD 0x02` is a block write. Rarely edited fields must come from the device scalar cache, not from stale UI defaults. The self-test therefore supplies deliberately different cache values and verifies that the generated block preserves them.

This test protects against the historical class of bug where moving one Music control overwrote neighboring K500 fields.

## Change policy

A golden vector may only change when all of the following are included in the same PR:

1. donor implementation or packet-capture evidence explaining the change;
2. updated golden vector;
3. regression test update;
4. hardware acceptance result when the command writes to the real device;
5. parity-matrix status update.

Do not weaken a test merely to make a changed implementation pass.
