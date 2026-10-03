# K500 System Controls Capture Map — 2026-10-04

This document records the focused physical USB captures used to close several
previously read-only System controls. It is evidence, not a license to infer
neighbouring fields from command-family similarity.

## Capture identities

| Capture | Bytes | SHA-256 |
| --- | ---: | --- |
| `RECALL_MODE_01_With_USE_INIT_VOL_Ticked.pcapng` | 6504 | `19840ab1db50bcaf852200e0f6e3e4ad1abe822bbd66675ffb6cd2a81b59bd55` |
| `RECALL_MODE_01_With_USE_INIT_VOL_Unticked.pcapng` | 5512 | `11e70490a5ea4d00749fdafdd65539cdf309f5c669e51ad307619631f2aeb5d3` |
| `Rename_Preset_Name_On_Device_KONSER_NYANYI_to_KONSER_SOLO.pcapng` | 6380 | `40fe010bc9c363309c095bd18f62e3237576553e404e6d5cdaf4812fdb2f6b85` |
| `Adj_Manner_VR_OFF_Unticked_Ticked_Unticked_Ticked.pcapng` | 1544 | `b13a17014c2ab8211f65473144cb61cb5e44d57125aeea44a6b16c5de66846cf` |
| `USB_Record_Vol_4_6Max_1Min.pcapng` | 3032 | `bacecde452ac438dfabf173f1a0705adc4afdfa170b9c3448c21b8f33c22e3a2` |
| `Effect_Init_Level_25_50_84Max_30.pcapng` | 18408 | `717fc0828e0f8365d7c7e215bef9ed5bfe731b54d7e27e69b4956b2474354366` |
| `Mic_Init_Vol_25_50_84Max_35.pcapng` | 18408 | `25d387713e00de737db5072a247f0eac316773ce9c87c49b3e59def8d925e99d` |
| `Music_Init_Vol_25_84Max_40.pcapng` | 22128 | `7c2e59701b0d437445595b2a3b2a1ebee56302e3e8d2a59db13c0ee3f2a97ecf` |

All promoted vectors below are checksum-valid packet deltas from these captures.

## Music Init Volume

Music Init is the second writable scalar of Top Music `CMD 0x02`.

Representative USB vector at Music Init = 84:

```text
AA 0D 00 02 19 54 54 02 09 09 09 08 08 07 15 00 E7
                  ^^
```

The UI/native domain is `0..84`. The engine must seed
`K500MusicBlockState.musicInitVol` from authoritative readback and serialize
that state on every later Top Music write. Do not replay a stale scalar cache
after the user edits Music Init.

ACK family remains Top Music `RSP 0xFD`.

## Mic Init Volume

Mic Init is the second writable scalar of Top Mic `CMD 0x05`.

Representative USB vector at Mic Init = 26:

```text
AA 0E 00 05 1E 1A 54 0B 04 00 64 64 32 02 01 0C 00 49
                  ^^
```

The UI/native domain is `0..84`. Mic Max remains the following scalar and is
**not** promoted by this capture. It stays device-seeded/read-only until its own
write delta is captured.

ACK family remains Top Mic `RSP 0xFA`.

## Effect Init Level

Effect Init has a dedicated `CMD 0x0A`; it is not a Top Effect `CMD 0x09`
edit.

Exact USB examples:

```text
Init 26  AA 03 00 0A 1A 23 B6
Init 84  AA 03 00 0A 54 23 7C
```

The second scalar remained `0x23` (=35) throughout the supplied sweep and
matches current Top Effect in the capture. SonKuPik therefore seeds that
neighbour from hydrated `topEffectVol` instead of hard-coding 35.

Device ACK is `RSP 0xF5`.

After this mapping, Top Effect `CMD 0x09` must also carry the current
hydrated/edited Effect Init state. Replaying a pre-edit readback byte there would
silently undo a preceding `CMD 0x0A` change.

## USB Record Volume

The captured USB Record family is `CMD 0x3E`, selector `0x03`, fixed tail
`0x54`. UI values `1..6` encode as raw `0..5`.

```text
UI 4  AA 04 00 3E 03 03 54 64
UI 6  AA 04 00 3E 03 05 54 62
UI 1  AA 04 00 3E 03 00 54 67
```

Device ACK is `RSP 0xC1`.

This capture does **not** prove the UDisk Record selector. UDisk Record remains
read-only even though it is adjacent in active memory.

## Adj Manner / VR OFF

Exact toggle vectors:

```text
OFF / unticked  AA 03 00 07 00 00 F6
ON  / ticked    AA 03 00 07 01 00 F5
```

Device ACK is `RSP 0xF8`.

No connect-time or 939-byte readback location was proven for this state. The
application therefore treats a valid `RSP 0xF8` as current-session truth only
and returns the UI to **DEVICE STATE UNKNOWN** after disconnect/reconnect.

## Persistent Equipment Mode rename

The native application did not use a dedicated Rename opcode. The supplied
`KONSER NYANYI -> KONSER SOLO` capture used the existing permanent Store
transaction:

```text
CMD 0x41 begin
CMD 0x42 chunk x11
CMD 0x43 commit
```

The new 16-byte space-padded name is located at slot-image offset:

```text
0x0280 .. 0x028F
"KONSER SOLO     "
```

Implementation contract:

1. Require USB Store availability and a known ACTIVE slot.
2. Perform a fresh 939-byte device readback.
3. Take exactly the first `0x0290` bytes as current native slot image.
4. Replace only bytes `0x0280..0x028F` with a 1..16 printable-ASCII,
   space-padded name.
5. Run the already-proven single-slot `0x41/0x42/0x43` Store path.
6. Recall the same slot, handshake, and perform a full 939-byte readback before
   LIVE resumes.

This intentionally renames only the active slot. It must never copy the active
DSP image into another selected slot merely to rename that other slot.

## Use Init Volume recall semantics

The paired Mode 01 Recall captures prove the behavioral meaning of Use Init
Volume, not just its `CMD 0x12` setter.

With Use Init Volume **ON**, the post-Recall active masters become:

```text
Top Music  = Music Init  = 40
Top Mic    = Mic Init    = 35
Top Effect = Effect Init = 30
```

With Use Init Volume **OFF**, the corresponding active masters remained 25 in
the paired capture.

The reconstructed 939-byte snapshots differ only at the three active-master
bytes:

```text
activeMemory[0x0000]  Music master   ON 40 / OFF 25
activeMemory[0x0001]  Mic master     ON 35 / OFF 25
activeMemory[0x0002]  Effect master  ON 30 / OFF 25
```

The stored init values remain unchanged at their established active-memory/file
mappings. Recall therefore performs device-owned application of Init values;
SonKuPik must not emulate that copy locally. It continues to Recall, handshake,
read all 939 bytes, then hydrate whatever the K500 actually applied.

## Evidence boundary still open

These neighbouring controls remain intentionally unpromoted:

- **Mic Max Volume** — READ known, WRITE not captured.
- **UDisk Record Volume** — READ known, WRITE selector not captured.
- **Adj Manner / VR OFF readback** — setter/ACK known, reconnect truth unknown.
- Dance/Mic Trigger Threshold and Hold Time — no byte-verified mapping yet.

Do not infer any of them from selector adjacency or command-family similarity.
