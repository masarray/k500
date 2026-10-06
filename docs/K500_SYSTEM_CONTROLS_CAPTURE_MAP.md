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

Adj Manner is an **ownership switch** for the front-panel screwdriver trim
potentiometers, not a tone preset rewrite.

### 2026-10-06 write + connect/readback closure

The latest native-app capture set supersedes the older CMD-0x07 tail assumption
and also proves that C0 data[19] is not a stable VR-OFF authority.

| Capture | Bytes | SHA-256 |
| --- | ---: | --- |
| `VR_OFF_tick_untick.pcapng` | 4,272 | `97c880d5caec84aaa760d552f852b64f3305b8c01a4be00fc28acf4b321ca3ba` |
| `Connect_VR_OFF_ticked_position.pcapng` | 5,016 | `9a66ba4b54c767a594b0c4eea351b15b145a662de89056a07452fd98815e9e87` |
| `Connect_VR_OFF_unticked_position.pcapng` | 5,016 | `64f5baa97db41cacc526e0c8d78be8b5b631462222f8c5b9fc31a85d97831839` |
| `nativeKTV_VROFF_OFF_alias_Unticked.pcapng` | 1,048 | `5cb8e2ee8a6f35de66aa5cbaeb95ec9cbd30bb1c87343a762d552fbbeab604db` |
| `k500_VROFF_BounchyON.pcapng` | 16,920 | `ebaf3be207ac66529acf53f355a17d6df8fd70dc4b76a1d1f5bc50108edb5bc9` |

The native toggle sweep contains fourteen alternating CMD 0x07 writes. Every
write uses route/control byte `0x03`, and every write receives `RSP 0xF8`:

```text
OFF / unticked  AA 03 00 07 00 03 F3
ON  / ticked    AA 03 00 07 01 03 F2
ACK             RSP 0xF8
```

The paired connect files reconstruct complete checksum-valid 939-byte active
memory images. They differ at exactly one semantic byte:

```text
activeMemory[0x008C]

VR OFF unticked / OFF = 0x00
VR OFF ticked   / ON  = 0x01
```

The standard captured active-memory block that contains this byte starts at
`0x0074`, length `0x003A`; VR OFF is relative index `0x18`. This mapping is
authoritative when that memory is actually read during connect/reconnect/Recall.

The newer native runtime capture closes a separate boundary: after
`AA 03 00 07 00 03 F3`, native receives `RSP 0xF8` and does **not** issue an
immediate CMD `0x40` verification read. The SonKuPik regression capture shows
that adding such a read can return the previous `0x008C` value and bounce the
UI back ON. Runtime authority is therefore:

```text
CMD 0x07 -> valid RSP 0xF8 -> commit requested state for the current session
```

while `activeMemory[0x008C]` is reserved for real connect/reconnect/Recall
hydration. No synthetic patch is written into the cached active-memory snapshot.

Because `0x008C` is in the low scalar region, the corresponding file/preset
scalar remains:

```text
file[0x0094]
0x00 = unticked
0x01 = ticked
```

### C0 boundary

Do **not** decode VR OFF from C0 data[19]. Older reconnect evidence carried
`0x43/0x42`, while the new controlled connect pair carries `0x0E/0x0D` with
the opposite bit-0 polarity relative to tick state. That field therefore has
other semantics mixed into it and is not a safe authority.

Online VR-OFF truth is exclusively:

```text
activeMemory[0x008C]
```

### User-visible ownership semantics

- **VR OFF unticked / OFF**: fascia trim-pots are active. Software must not
  send edits for the trim-owned controls; the native application renders those
  controls darker/green and read-only.
- **VR OFF ticked / ON**: fascia trim-pots are disabled and software regains
  ownership of those controls.
- Changing Adj Manner itself does **not** rewrite the parameter values; the
  paired active-memory images are otherwise byte-identical.

The supplied manufacturer screenshots identify the trim-owned groups as:

- Music: Bass, Mid, Mid Freq, Treble.
- Mic: Bass, Mid, Mid Freq, Treble.
- Reverb: Level, Decay, Predelay.
- Echo: Effect Level, Left Delay, Repeat.

Reverb Direct/HPF/LPF and Echo Direct/Right Delay/Right Predelay/HPF/LPF are
not shown with the manual-VR treatment and remain software controls.

### Important manual-value telemetry boundary

This mapping proves the **ownership state**, not the fourteen analog VR
readings. The screenshots prove those manual readings are a separate value
domain. SonKuPik may lock the trim-owned editors and display hardware-owner
treatment, but must not invent live analog values or pretend normal digital
scalars are the screwdriver positions.

### Dance Mic collision correction

The reconnect delta also disproves the previous structural assumption that
file `0x0094` was Dance Mic Hold. It is definitively Adj Manner VR OFF.
Consequently the old speculative Dance pair at `0x0093/0x0094` remains retired
until Threshold/Hold read-side seeds are independently proven.

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
- Dance/Mic Trigger Threshold and Hold Time — no byte-verified mapping yet.

Do not infer any of them from selector adjacency or command-family similarity.


> Output Delay for Main/Surround/Center/Subwoofer is captured separately in
> `docs/K500_OUTPUT_DELAY_CAPTURE_MAP.md`, including the Surround R-first/L-second wire-order exception.

# Final operational capture batch — 2026-10-04

This batch closes the remaining System controls needed for ordinary daily use.
Credential/administrative controls are explicitly outside product scope.

## Capture identities

| Capture | Bytes | SHA-256 |
| --- | ---: | --- |
| `KTV_BTName_RESET.pcapng` | 1296 | `c5c0357721b036ba4b3f9726118f14b34aeb2d02945f7c3a67114abbd8992532` |
| `KTV_BTName_BLANK_to_ARI.pcapng` | 2040 | `ad4b1b1e8eb7a1e953550ff92945f4550648456880591d10a9eb18430171b15d` |
| `DanceMicTime_6s_30s_1s.pcapng` | 10720 | `e6d1ad19f9f9e8d5fd926658bda83100a6212721c1398cfa40e0b59cd43b66b6` |
| `DanceMicThres_min50dB_0dB_min60dB.pcapng` | 16424 | `bd3c68de7cefdccc2204aefd49923052688bfe59b86c5332d0bdb23684ce3a50` |
| `UDiskRecordVol_4-6maxVal-3-1minVal.pcapng` | 2536 | `1fc34360225ad9dfbfe0cb8ee421484e6f1a97061f2b62e0b157934a4fa4cd5e` |
| `MicMaxVol_84Max-50-30-0.pcapng` | 13944 | `48884d622d0e427a38605136517e66c739093f7bf12c775e4606caab13792eae` |

## Mic Max Volume

Mic Max is the third writable scalar in Top Mic `CMD 0x05`. Native range is
`0..84`. Lowering Mic Max below Top Mic clamps Top Mic in the SAME block.

Exact USB vectors:

```text
Max 50  AA 0E 00 05 23 23 32 0B 00 00 60 60 27 03 0A 02 00 74
Max 30  AA 0E 00 05 1E 23 1E 0B 00 00 60 60 27 03 0A 02 00 8D
Max  0  AA 0E 00 05 00 23 00 0B 00 00 60 60 27 03 0A 02 00 C9
```

ACK is `RSP 0xFA`.

Contract: `TopMic = min(TopMic, MicMax)`. Raising Mic Max does not raise Top Mic.

## UDisk Record Volume

UDisk Record also uses `CMD 0x3E`, but its captured payload is distinct from
USB Record. UI `1..6` maps to raw `0..5`.

```text
UI 6  AA 04 00 3E 05 00 00 B9
UI 1  AA 04 00 3E 00 00 00 BE
```

ACK is `RSP 0xC1`.

Do not collapse the two record controls into a guessed common selector:
USB Record is `3E 03 <raw> 54`, while UDisk Record is
`3E <raw> 00 00`.

## Dance Mic Trigger — Threshold + Hold

Both controls are a single full-pair `CMD 0x22` write:

```text
AA 07 00 22 01 <thresholdRaw> <holdSec> 0B 00 00 checksum
```

Threshold:
- native UI range: `-60..0 dB`
- encoding: `thresholdRaw = dB + 60`

Hold:
- native UI range: `1..30 s`
- encoding: raw seconds

Representative vectors:

```text
-50 dB / 6 s  AA 07 00 22 01 0A 06 0B 00 00 BB
  0 dB / 6 s  AA 07 00 22 01 3C 06 0B 00 00 89
-60 dB / 1 s  AA 07 00 22 01 00 01 0B 00 00 CA
-60 dB /30 s  AA 07 00 22 01 00 1E 0B 00 00 AD
```

ACK is `RSP 0xDD`.

Because changing either control transmits BOTH values, SonKuPik must have a valid
paired seed before writing. The later Adj Manner reconnect pair **invalidates**
the former structural `0x0093/0x0094` seed: file `0x0094` is now physically
proven to be Adj Manner VR OFF (active `0x008C`). Therefore Dance Mic remains
fail-closed for live editing until an independent reconnect delta maps both
Threshold and Hold. The captured `CMD 0x22` write format remains valid; only
the unsafe read-side seed was retired.

## BT Name rename/reset

BT Name uses dedicated `CMD 0x4E`; it is not part of preset Store.

```text
SET "ARI"  AA 0B 00 4E 01 41 52 49 00 00 00 00 00 03 C7
RESET      AA 0B 00 4E 00 00 00 00 00 00 00 00 00 03 A4
```

Captured structure:

```text
CMD 0x4E
operation 0x01 = SET
operation 0x00 = RESET
name field = exactly 8 bytes, NUL padded
route mask = 0x03
ACK = RSP 0xB1
```

The promoted UI accepts only 1..8 printable ASCII characters for SET. BT Name
operations are USB-only because the supplied evidence is USB capture. After ACK,
SonKuPik performs a full 939-byte refresh and displays the hardware BT name from
the established active-memory readback. The readback identity field itself is
0x13 bytes at activeMemory[0x0385], so generated/reset identities such as
`KTV_BT_00AB12` must be shown in full even though CMD 0x4E SET accepts only
1..8 user-entered ASCII characters. UI display length and write payload length
are therefore separate contracts. BLE rename/reset is NOT inferred from this BT
command and remains read-only.

## Daily-use scope closure

The following uncommon credential controls are intentionally unsupported:

- Lock Key state/password/Modify;
- Admin/User mode credentials/password/Modify.

They are not considered release-blocking reverse-engineering TODOs. The product
must keep them device-managed and must never invent credential frames.
