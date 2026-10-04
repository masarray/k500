# K500 Output Delay Capture Map

Status: 2026-10-04 physical USB capture mapping.

This document is the packet-level source of truth for Main, Surround, Center and
Subwoofer output delay controls. It supersedes the older Main/Center/Sub
read-only evidence gate and corrects the earlier Surround transport field order.

## Capture identities

| Capture | Bytes | SHA-256 |
| --- | ---: | --- |
| `Main_R_Delay_0ms0meter_20ms6meter8_50ms17meter_0ms.pcapng` | 16052 | `8ba717de4b3bdf9fe9bb1acbe5b48264fb0ec855a8d3816b9b2196b7417ccdb3` |
| `Main_L_Delay_0ms0meter_20ms6meter8_50ms17meter.pcapng` | 11960 | `22c14407b0b9e6afc526ada9403f9e92746fe19cfe989fa305c3d784b4bdc733` |
| `Subwoofer_OutputDelay_0ms0meter_20ms6meter8_30ms10m2_40ms13m6_50ms17meter.pcapng` | 9728 | `47917582faf0d4882790808a52870806517e2a73afe3803183ac72d99d5e198d` |
| `Center_OutputDelay_0ms0meter_20ms6meter8_30ms10m2_40ms13m6_50ms17meter.pcapng` | 605508 | `f546ed2d6c19a18f6cc794be618d71cf4442810635e098d61485a13ce1f40e61` |
| `Surround_L_Delay_20ms6meter8_50ms17meter_0ms0meter.pcapng` | 463760 | `f076d747036bd90f257835b7c25652a203d1f56ab391d29a739de2164e3980a2` |
| `Surround_R_Delay_14ms4meter8_50ms17meter_0ms0meter.pcapng` | 12704 | `7529ef354261aa339f19fc6cc0985046db98f9ee480ec1c0be8ba5b31d91d5f8` |

## Shared command family

All four output sections use the existing full-image Output command:

```text
USB: AA 25 00 0E <section> [35-byte data image] checksum
ACK: RSP 0xF1
```

Section IDs remain:

```text
Main      0x00
Surround  0x02
Center    0x04
Subwoofer 0x05
```

The delay value is a uint16 little-endian integer in **milliseconds**. The
manufacturer UI range observed in this batch is **0..50 ms**.

The manufacturer's distance label is display-only:

```text
distance_m ≈ delay_ms × 0.34
20 ms -> 6.8 m
30 ms -> 10.2 m
40 ms -> 13.6 m
50 ms -> 17.0 m
```

No distance value is transmitted separately.

## Clarified Surround semantic reference

The manufacturer UI/reference screenshot supplied with this batch shows the
actual device state before further edits as:

```text
Surround L Delay = 14 ms ≈ 4.8 m
Surround R Delay = 20 ms ≈ 6.8 m
```

This is a **semantic/readback reference**, not an application default. SonKuPik
must hydrate these values from hardware. It must never hardcode 14/20.

The two PCAP sweeps still prove the native transport exception independently:
the capture identified as **Surround L** moves the second timing word
(`data[18..19]`) while the first timing word stays fixed; the capture
identified as **Surround R** moves the first timing word (`data[16..17]`)
while the second stays fixed. Therefore the final semantic mapping remains:

```text
L = data[18..19]
R = data[16..17]
```

For the screenshot/reference state above, the serialized pair is therefore
`14 00 0E 00` in wire order (**R=20 first, L=14 second**).

## Delay field positions

Positions below are relative to the 35-byte Output data image, not the complete
USB frame.

| Section | UI control | Wire field | Encoding |
| --- | --- | --- | --- |
| Main | L Delay | data[16..17] | uint16 LE ms |
| Main | R Delay | data[18..19] | uint16 LE ms |
| Surround | **R Delay** | **data[16..17]** | uint16 LE ms |
| Surround | **L Delay** | **data[18..19]** | uint16 LE ms |
| Center | Output Delay | data[16..17] | uint16 LE ms |
| Subwoofer | Output Delay | data[16..17] | uint16 LE ms |

### Important Surround exception

Surround is physically captured in the opposite wire order from Main.

Do **not** normalize this in the transport builder:

```text
Main:      data16=L, data18=R
Surround:  data16=R, data18=L
```

The semantic state remains normal (`lDelayMs`, `rDelayMs`). Only the native
wire serialization order is reversed for Surround.

## Exact reference vectors

### Main

Main L=20 ms, R=0 ms:

```text
AA 25 00 0E 00 63 63 63 63 64 32 64 32 61 32 26 32 2F 12 07 01
14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 CD
```

Main L=50 ms, R=20 ms:

```text
AA 25 00 0E 00 63 63 63 63 64 32 64 32 61 32 26 32 2F 12 07 01
32 00 14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 9B
```

### Surround

The first timing pair below is **R then L**.

Surround R=14 ms, L=20 ms:

```text
AA 25 00 0E 02 5D 63 5D 63 46 32 4E 32 64 32 32 32 2A 04 0C 03
0E 00 14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FA
```

Surround R=50 ms, L=20 ms:

```text
AA 25 00 0E 02 5D 63 5D 63 46 32 4E 32 64 32 32 32 2A 04 0C 03
32 00 14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 D6
```

### Center

Center Output Delay=20 ms:

```text
AA 25 00 0E 04 58 63 63 63 62 32 30 32 4A 32 22 32 29 04 0A 03
14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 34
```

Center Output Delay=50 ms:

```text
AA 25 00 0E 04 58 63 63 63 62 32 30 32 4A 32 22 32 29 04 0A 03
32 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 16
```

### Subwoofer

Subwoofer Output Delay=20 ms:

```text
AA 25 00 0E 05 63 4B 4B 4B 00 32 60 32 00 32 00 32 2A 06 19 03
14 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FC
```

Subwoofer Output Delay=50 ms:

```text
AA 25 00 0E 05 63 4B 4B 4B 00 32 60 32 00 32 00 32 2A 06 19 03
32 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 DE
```

## Active-memory / preset scalar mapping

The command capture proves the write positions. Existing active-memory scalar
layout supplies the read/persistence seed:

```text
Main L Delay       file scalar 0x0034, uint16 LE
Main R Delay       file scalar 0x0036, uint16 LE
Surround L Delay   file scalar 0x00D8, uint16 LE
Surround R Delay   file scalar 0x00DA, uint16 LE
Center Delay       file scalar 0x005C, uint16 LE
Subwoofer Delay    file scalar 0x0070, uint16 LE
```

For Main/Center/Sub these offsets are the timing words occupying data[16..19]
or data[16..17] in their contiguous scalar image. Surround keeps its established
separate 0x00D8/0x00DA read/preset words and maps them into the reversed native
wire order at serialization time.

Reconnect hardware acceptance must still verify that the displayed readback
matches the native application for all six semantic controls.

## Safety contract

- preserve all unknown/reserved bytes from current hardware readback;
- patch only the captured delay word(s);
- clamp runtime and preset edits to 0..50 ms;
- one semantic L/R name must always mean the UI channel, never raw wire position;
- Surround serialization must remain R-at-data16 / L-at-data18;
- no output-delay edit may alter volume, mixer, compressor or crossover state.
