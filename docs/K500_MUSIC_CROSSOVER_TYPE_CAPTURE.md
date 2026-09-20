# K500 Music HP/LP Type Capture Map

Status: physically captured on 2026-09-20 with the manufacturer K500 application over USB HID.

## Evidence files

| Capture | SHA-256 |
| --- | --- |
| `Connect_LPType_butter12_bypass_disconnect_connect_Lbypass_butter12.pcapng` | `5461867b30c2822e27c8d25c57f37a8b71ec29907617b2aa3cba5ea332e7bf2a` |
| `Connect_HPtype_bypass_bessel12db_butter12db_bessel18db_butter18db_bessel24db_butter24db_linkRiley24.pcapng` | `e54267b88a65dd474a3f8d872af0d8e1636ada2c2862289e14715f092148c3e3` |

These captures apply specifically to the **Music Section** crossover HP/LP type controls.

## Authoritative CONNECT readback

Reconnect deltas isolate two direct active-memory bytes:

```text
activeMemory[0x0007] = Music HP Type
activeMemory[0x0008] = Music LP Type
```

Equivalent .k500 file scalar locations are `0x000F` and `0x0010`, but the
runtime implementation should use the direct active-memory bytes above.

Observed reconnect states:

```text
Music LP Butter 12 -> activeMemory[0x0008] = 0x02
Music LP Bypass    -> activeMemory[0x0008] = 0x00

Music HP Bypass    -> activeMemory[0x0007] = 0x00
```

## Native HP enum

The HP sweep proves the complete shared filter enum through Music `CMD 0x11`,
selector `0x02`:

| Raw | Music HP Type |
| ---: | --- |
| 0 | Bypass |
| 1 | HP Bessel 12 |
| 2 | HP Butter 12 |
| 3 | HP Bessel 18 |
| 4 | HP Butter 18 |
| 5 | HP Bessel 24 |
| 6 | HP Butter 24 |
| 7 | HP LR 24 / Link Riley 24 |

Captured HP write sequence:

```text
Bessel 12      AA 06 00 11 02 01 3C 00 00 AA
Butter 12      AA 06 00 11 02 02 3C 00 00 A9
Bessel 18      AA 06 00 11 02 03 3C 00 00 A8
Butter 18      AA 06 00 11 02 04 3C 00 00 A7
Bessel 24      AA 06 00 11 02 05 3C 00 00 A6
Butter 24      AA 06 00 11 02 06 3C 00 00 A5
Link Riley 24  AA 06 00 11 02 07 3C 00 00 A4
```

## Native LP evidence

The LP capture directly proves:

```text
raw 0 = Bypass
raw 2 = LP Butter 12
```

Music LP writes use `CMD 0x11`, selector `0x03`:

```text
LP Bypass     AA 06 00 11 03 00 20 4E 00 78
LP Butter 12  AA 06 00 11 03 02 20 4E 00 76
```

The implementation reuses the same 0..7 filter enum already shared by
`crossoverFilterCode()` for HP and LP. Only the HP sequence is fully captured;
LP codes other than 0 and 2 remain protocol-family inference until independently
swept. This distinction is documented rather than hidden.

## Application contract

1. On hardware CONNECT/readback, hydrate Music HP Type from `activeMemory[0x0007]`.
2. Hydrate Music LP Type from `activeMemory[0x0008]`.
3. Update both the top-level Music properties and Music EQ model without emitting live edits.
4. Canonical state marks Music HP/LP Type as captured device truth.
5. Other sections remain assumption-gated until their own crossover type readback captures exist.
6. Existing Music `CMD 0x11` WRITE behavior is preserved.
