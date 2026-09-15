# Native Echo CMD 0x0D capture map

Evidence source: HHD Device Monitoring Studio 8.47 USB HID delta captures from the native KTV application. The KTV application writes Echo as one complete native USB block; changing any exposed Echo parameter retransmits the full block.

## Native USB frame

```text
AA 17 00 0D [22-byte Echo image] CHECKSUM
```

`0x17` is the body length (`CMD 0x0D` + 22 data bytes). The checksum is the two's-complement checksum used by the other native K500 USB blocks.

Representative baseline image seen across the captures:

```text
index:  00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21
data :  01 5A 02 64 02 40 64 3C 3C 26 02 68 10 2C 01 64 00 C8 00 00 00 00
```

## Verified field map

| Echo UI control | CMD 0x0D data | Native representation | Capture evidence |
| --- | --- | --- | --- |
| Effect Level | `data[1]` | `u8`, direct percent | `100 -> 99..90` changes only this byte |
| Repeat | `data[2]` | `u8`, direct integer | `2 -> 3..10` changes only this byte |
| Direct | `data[6]` | `u8`, direct percent | `100 -> 99..90` changes only this byte |
| Right Delay | `data[7]` | signed percent encoded as `percent + 50` | `10% -> raw 60`, `20% -> raw 70` |
| Right Ch Predelay | `data[8]` | signed percent encoded as `percent + 50` | `10% -> raw 60`, `-10% -> raw 40` |
| Highpass | `data[9..10]` | `u16 LE`, Hz | `550 -> 551..560` |
| Lowpass | `data[11..12]` | `u16 LE`, Hz | `4200 -> 4201..4210` |
| Left Delay | `data[13..14]` | `u16 LE`, milliseconds | `300 -> 301..310` |
| Left Ch Predelay | `data[15..16]` | `u16 LE`, milliseconds | `100 -> 99..90` |

The following bytes were not changed by any supplied delta capture and therefore remain preservation-only: `data[0]`, `data[3]`, `data[4]`, `data[5]`, and `data[17..21]`. They must be seeded from device readback and must not be synthesized by a live edit.

## Exact examples

DIRECT 99%:

```text
AA 17 00 0D 01 5A 02 64 02 40 63 3C 3C 26 02 68 10 2C 01 64 00 C8 00 00 00 00 05
```

Combined regression vector using only independently verified fields (Level 90, Repeat 10, Direct 90, Right Delay +20%, Right Predelay -10%, HPF 560 Hz, LPF 4210 Hz, Left Delay 310 ms, Left Predelay 90 ms), with all unknown bytes preserved from the donor image:

```text
AA 17 00 0D 01 5A 0A 64 02 40 5A 46 28 30 02 72 10 36 01 5A 00 C8 00 00 00 00 FC
```

## Capture files

- `EchoDirect_100_99_98_97_90.dmslog8`
- `EchoEffLevel_100_99_98_97_90.dmslog8`
- `EchoRepeat_2_10.dmslog8`
- `EchoHighPass_550_560Hz.dmslog8`
- `EchoLowPass_4200_4210Hz.dmslog8`
- `EchoLeftDelay_300_310ms.dmslog8`
- `EchoRightDelay_10_20percent.dmslog8`
- `EchoLeftChPreDelay_100_90ms.dmslog8`
- `EchoRighPreDelay_10_minus10percent.dmslog8`

The HHD recordings can contain older frames as a cumulative prefix. Field conclusions above are based on the newly appended, checksum-valid `CMD 0x0D` delta frames, not on filename assumptions alone.

## Evidence gate

These captures prove the frequency values for Echo HPF/LPF but do **not** prove filter-type selector bytes. Filter-type writes therefore remain gated until a dedicated native capture is available. Generic `CMD 0x11` Echo frequency writes must not be used for these captured frequency controls; the native application retransmits the full `CMD 0x0D` image instead.
