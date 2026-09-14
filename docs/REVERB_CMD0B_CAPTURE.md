# Native Reverb CMD `0x0B` capture

Status: byte-verified from HHD Device Monitoring Studio captures against the native KTV application.

## USB frame

Reverb edits use one complete block, not the generic crossover `CMD 0x11` family:

```text
AA 10 00 0B [15 data bytes] CHECKSUM
```

The 15-byte Reverb data image is:

| Data offset | Meaning | Encoding | Capture evidence |
| ---: | --- | --- | --- |
| `0` | Level | `u8`, percent | 99 → `63`, 98 → `62`, … 95 → `5F` |
| `1` | Unknown / preserve | `u8` | observed `01` |
| `2` | Direct | `u8`, percent | 99 → `63`, 98 → `62`, … 95 → `5F` |
| `3` | Unknown / preserve | `u8` | observed `32` |
| `4` | Unknown / preserve | `u8` | observed `32` |
| `5` | Unknown / preserve | `u8` | observed `55` |
| `6..7` | HPF | `u16 LE`, Hz | 221 → `DD 00`, … 225 → `E1 00` |
| `8..9` | LPF | `u16 LE`, Hz | 15801 → `B9 3D`; 16000 → `80 3E` |
| `10..11` | Decay | `u16 LE`, ms | 1681 → `91 06`, … 1685 → `95 06` |
| `12..13` | Predelay | `u16 LE`, ms | 43 → `2B 00`, … 50 → `32 00` |
| `14` | Unknown / preserve | `u8` | observed `00` |

Unknown bytes are never synthesized during LIVE edits. `K500Controller` seeds the complete block from current device readback and `K500Protocol::reverbBlock()` patches only the byte-verified fields above.

## Golden examples

```text
Level 99:
AA 10 00 0B 63 01 64 32 32 55 DC 00 B8 3D 90 06 2A 00 00 D3

Direct 99:
AA 10 00 0B 5F 01 63 32 32 55 DC 00 B8 3D 95 06 32 00 00 CB

HPF 221 Hz:
AA 10 00 0B 5F 01 5F 32 32 55 DD 00 B8 3D 95 06 32 00 00 CE

LPF 16000 Hz:
AA 10 00 0B 5F 01 5F 32 32 55 E1 00 80 3E 95 06 32 00 00 01
```

The source capture filename for the final LPF transition contains `1600Hz`; the packet itself is unambiguous: `80 3E` little-endian is decimal `16000`, so the protocol evidence is recorded as 16000 Hz.

## Evidence gate

This capture verifies Level, Direct, HPF frequency, LPF frequency, Decay and Predelay. It does **not** verify Reverb HPF/LPF filter-type dropdown writes. Reverb filter-type edits therefore remain non-destructive/unsupported until a dedicated native delta capture proves their command bytes.
