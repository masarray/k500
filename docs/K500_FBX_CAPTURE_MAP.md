# K500 FBX Native Capture Map

Status: physically captured on 2026-09-20 with the manufacturer K500 application over USB HID.

## Evidence files

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| Connect_FBX_0_1_2_3_4.pcapng | 6752 bytes | b7fe3ccfc81c1f0a125c575ca6f05b43f7c932051799b750962c88cc172afe22 |
| Connect_FBX_4_3_2_1_0.pcapng | 7000 bytes | 5d8cf04d6b533c27a3ae9fc09765b814ec26df99216c40eb755b3478243f2487 |

The two captures are deliberately symmetric: connect at level 0 then write 1→2→3→4, and connect at level 4 then write 3→2→1→0.

## Authoritative READ mapping

The first 58-byte RSP 0xBF readback block starts at active-memory offset 0x0000.

The only FBX endpoint delta in that block is live active-memory byte `0x001B`:

```text
Connect with FBX 0: activeMemory[0x001B] = 0x00
Connect with FBX 4: activeMemory[0x001B] = 0x04
```

Therefore:

```text
FBX level = activeMemory[0x001B]
range = 0..4
```

Important: this is a **live active-memory offset**. It is not file offset `0x001B`.
The file-offset helper used by StudioEngine/Controller subtracts the K500 file header,
so using `fileU8(memory, 0x001B)` reads the wrong live byte. FBX must use the direct
active-memory byte at `0x001B`.

## Native WRITE mapping

The manufacturer app uses Top Mic `CMD 0x05`. Across all five levels, exactly one
FBX data byte changes and the following byte remains `0x00`:

```text
Level 0  AA 0E 00 05 19 19 54 0B 00 00 60 60 27 03 0A 02 00 66
Level 1  AA 0E 00 05 19 19 54 0B 01 00 60 60 27 03 0A 02 00 65
Level 2  AA 0E 00 05 19 19 54 0B 02 00 60 60 27 03 0A 02 00 64
Level 3  AA 0E 00 05 19 19 54 0B 03 00 60 60 27 03 0A 02 00 63
Level 4  AA 0E 00 05 19 19 54 0B 04 00 60 60 27 03 0A 02 00 62
```

Every write receives checksum-valid RSP `0xFA`.

Contract:

- FBX is a direct integer scalar `0..4`.
- The byte immediately after FBX in CMD 0x05 is captured as fixed `0x00`.
- Do not mirror active-memory neighbour `0x001C` into that command byte.
- Unrelated Top Mic fields still preserve their existing device-seeded behavior.

## CMD 0x40 trailing-byte observation

These captures also show the manufacturer app's final byte in each USB `CMD 0x40`
read request matching the FBX level that was already present when the session connected:

```text
FBX 0: AA 06 00 40 00 00 3A 00 00 80
FBX 4: AA 06 00 40 00 00 3A 00 04 7C
```

The same 0/4 correlation appears on every read block in each session.

This is **observational evidence only**, not yet a replay requirement. The captures do
not prove whether that byte is required by the device, merely mirrored by the native
application, or derived from another session state. SonKuPik's established readback
wire behavior is therefore left unchanged in this patch. Do not reinterpret the byte
or derive it from C0 without an independent capture/test proving necessity.

## Implementation requirements

1. Hydrate FBX from direct active-memory offset `0x001B`.
2. Clamp FBX to native range `0..4`.
3. Emit the direct FBX byte in CMD `0x05`.
4. Emit fixed `0x00` in the following captured command byte.
5. Preserve all unrelated Top Mic device-seeded values.
6. Protect exact level 0..4 frames with hardware-free protocol self-tests.
