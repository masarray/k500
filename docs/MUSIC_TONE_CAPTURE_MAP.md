# K500 Music Tone Native Capture Map

Status: WRITE mappings captured 2026-09-18; authoritative reconnect READ mappings closed 2026-10-04 with physical USB HID captures.

## Evidence files

| Capture | Size | SHA-256 | Role |
| --- | ---: | --- | --- |
| `MusicNoiseGate_min50_OFF.pcapng` | 17,168 bytes | `5a6ca678b704e56b53dec3e7d92d24c33934a4d61975a2f80040eba3a62e9008` | original Noise Gate WRITE sweep |
| `Bass_12_9_4_0_min5_min10_min12.pcapng` | 28,824 bytes | `f458a84ea8bad87eddf6201ac9e442c9e769f8ea98c27ce033b91fdac58fe366` | original Bass WRITE sweep |
| `Music_NoiseGate_OFFbottom_min75_min60_min50Top_min60_min80_OFF_min90.pcapng` | 20,144 bytes | `e1a882074e02d8781a9181b98dc8a3754edc4c7d6d8fbac7fd8f27945c819144` | 2026-10-04 WRITE requalification |
| `Music_NoiseGate_min90_setlowest_OFF_reconnect.pcapng` | 10,968 bytes | `3131d85d46c9f812e81eb8a631dc07e01e5aa7400468d9fc69256461e483d223` | -90/OFF reconnect delta |
| `Reconnect_MusicNoiseGate_minus50dB.pcapng` | 10,968 bytes | `803dc46077f3c275ae9bcda67e71a7557b102a45c2b01b503f507d6606ece5e7` | -50 reconnect truth |
| `Music_Bass_0_plus4_plus12_0_minus4_minus12_0.pcapng` | 71,232 bytes | `9a2f5231b23aab6580149cc9c7d013c17c76c159f5ea5fd953c728811da0695d` | 2026-10-04 WRITE requalification |
| `Connect_MusicBass_0_12_Reconnect_12_0_reconnect_0_min12.pcapng` | 26,592 bytes | `df9d49787979cc6aac006dc273ff51a9b5cc9093db2a0ba09452502ce69a7d7e` | 0/+12/0/-12 reconnect truth |

Every reconnect claim below is derived from reconstructed full `0x03AB` / 939-byte active-memory images, not from filenames alone.

## Music Noise Gate

Native app retransmits the full Top-Music `CMD 0x02` block. The Noise Gate field is the byte immediately after Music Key.

Historical endpoint vectors:

```text
OFF     AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 00 13 24
-90 dB  AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 01 13 23
-50 dB  AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 29 13 FB
```

Authoritative UI/value contract:

- raw `0x00` = OFF;
- raw `0x01` = -90 dB;
- ...;
- raw `0x29` (41) = -50 dB.

For raw 1..41:

```text
dB = raw - 91
raw = dB + 91
```

OFF is a dedicated sentinel and must not be treated as an ordinary -91 dB threshold.

### Authoritative CONNECT/readback truth

The paired reconnects isolate exactly one active-memory byte:

```text
activeMemory[0x0005] = Music Noise Gate

OFF     -> 0
-90 dB  -> 1
-50 dB  -> 41
```

The -90 -> OFF reconnect changes only `0x0005: 01 -> 00`. The independent OFF vs -50 snapshots likewise isolate only `0x0005: 00 -> 29`.

This is a **direct active-memory index**. Runtime code must not pass `0x0005` through the `.k500`/file offset translator.

### Top-Music tail safety boundary

The new capture also exposes an older preservation ambiguity that must not be guessed away:

- the historical Noise Gate vectors above carry a trailing Top-Music byte `0x13`;
- the 2026-10-04 native sweep in the current device state carries trailing byte `0x00`;
- `activeMemory[0x0007]` is independently proven to be **Music HP Type**, so it must not be newly documented as authoritative Top-Music-tail truth;
- a single current state is insufficient to promote another candidate offset such as `0x001B`.

Therefore this batch changes the Noise Gate fallback/readback donor to captured `0x0005` but intentionally leaves the unrelated trailing-byte source **evidence-gated**. A dedicated donor-isolation capture is still required before that preservation path may be remapped.

## Music Bass

Native app uses `CMD 0x0C`, selector `0x02`:

```text
AA 06 00 0C 02 00 RAW_LO RAW_HI 09 CHECKSUM
```

Representative captured values:

| UI Bass | Raw decimal | Raw LE | Exact captured frame |
| ---: | ---: | --- | --- |
| +9.0 dB | 210 | D2 00 | AA 06 00 0C 02 00 D2 00 09 11 |
| +4.0 dB | 160 | A0 00 | AA 06 00 0C 02 00 A0 00 09 43 |
| 0.0 dB | 120 | 78 00 | AA 06 00 0C 02 00 78 00 09 6B |
| -5.0 dB | 70 | 46 00 | AA 06 00 0C 02 00 46 00 09 9D |
| -10.0 dB | 20 | 14 00 | AA 06 00 0C 02 00 14 00 09 CF |
| -12.0 dB | 0 | 00 00 | AA 06 00 0C 02 00 00 00 09 E3 |

The complete encoding is linear in 0.1 dB units:

```text
raw = round((dB + 12.0) * 10)
dB  = raw / 10.0 - 12.0
```

Native range is -12.0..+12.0 dB, raw 0..240.

### Authoritative CONNECT/readback truth

The 2026-10-04 reconnect sequence reconstructs four complete 939-byte snapshots:

```text
0 dB    -> activeMemory[0x00DF] = 120
+12 dB  -> activeMemory[0x00DF] = 240
0 dB    -> activeMemory[0x00DF] = 120
-12 dB  -> activeMemory[0x00DF] = 0
```

Each adjacent known-state reconnect changes only `activeMemory[0x00DF]`. The same affine transform used by `CMD 0x0C` therefore decodes the active-memory byte:

```text
dB = (raw - 120) / 10.0
```

As with Noise Gate, `0x00DF` is a direct active-memory index and is not a `.k500` file offset.

## Application contract

1. Hydrate Music Noise Gate from direct `activeMemory[0x0005]` only when raw is 0..41.
2. Hydrate Music Bass from direct `activeMemory[0x00DF]` only when raw is 0..240.
3. Hydration is authoritative and emits zero live-edit/device-write events.
4. Canonical state may promote these semantic values as snapshot-derived truth.
5. An unrelated Top-Music edit must preserve the captured Noise Gate value from `0x0005`.
6. Do not infer the unresolved Top-Music trailing-byte donor from adjacency or one-state correlation.


## Cross-mode Top-Music tail isolation — 2026-10-04

The same cross-mode capture
`Reconnect_ModeA_B_C_TopMusicTail_MicLPStateDonor.pcapng`
(SHA-256 `e3b39a78019f2948e722553e51379c464720c553153ef12773998ba8df592bff`)
contains six stable 939-byte readbacks spanning Equipment Modes 1, 2 and 3.

Each mode changes many unrelated active-memory bytes, but every native Music Key
edit carries the same trailing Top-Music byte:

```text
Key +1  AA 0D 00 02 19 19 54 02 09 09 09 08 08 08 15 00 21
Key  0  AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 15 00 22
                                                     ^^ gate = 0x15
                                                        ^^ unresolved tail = 0x00
```

The captured gate byte `0x15` matches authoritative
`activeMemory[0x0005]`, independently re-confirming the Noise Gate donor.

The unresolved trailing byte remains `0x00` across all three modes. Because
196 active-memory bytes are also zero in all three representative snapshots,
this capture cannot uniquely identify a direct donor for that field. It does,
however, eliminate every candidate byte that changes between Modes 1/2/3.

A focused high-value next test is to vary FBX `0 -> 4 -> 0` and trigger a Music
Key write after each state. Direct `activeMemory[0x001B]` is a known FBX byte
and equals zero in the current three-mode capture, so this test can decisively
confirm or eliminate that historically suspicious nearby candidate without
guessing.


## FBX candidate eliminated for Top-Music tail — 2026-10-04

Physical capture:

| Capture | SHA-256 |
| --- | --- |
| `1 Top-Music tail — tes kandidat paling mencurigakan.pcapng` | `b66efea0e8ff244287cac9fe696bc36bea5c63602cd2eeeee2a0b79baba86dcb` |

Sequence:

```text
initial Music Key edit                 tail 00
FBX 0 -> 1 -> 2 -> 3 -> 4 (CMD 0x05)
Music Key edit                         tail 02
FBX 4 -> 3 -> 2 -> 1 -> 0 (CMD 0x05)
Music Key edit                         tail 02
```

Therefore the Top-Music tail does **not** directly encode current FBX level:
when FBX returned to 0 the tail remained `0x02`.

The accompanying native-UI state shows Music HP Type = Butterworth 12 dB
(enum `0x02`), which is consistent with the existing compatibility donor
`activeMemory[0x0007]`, but this is supporting correlation rather than a
controlled donor proof. Do not promote it yet.

### Minimal final donor capture

Vary **Music HP Type itself**, then trigger Music Key after each value while
leaving other controls untouched:

```text
Music HP Type Bypass      -> Music Key 0 -> +1 -> 0
Music HP Type Bessel 12   -> Music Key 0 -> +1 -> 0
Music HP Type Butter 18   -> Music Key 0 -> +1 -> 0
restore Butter 12         -> Music Key 0 -> +1 -> 0
```

If the Top-Music tail follows `00 -> 01 -> 04 -> 02`, direct
`activeMemory[0x0007]` is proven as the preservation donor. If it does not,
that candidate is eliminated without disturbing any other mapping.
