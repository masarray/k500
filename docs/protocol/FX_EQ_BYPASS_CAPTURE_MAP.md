# EQ Enable / Bypass Shared Image — Native Capture Map

Markers: `FX_EQ_BYPASS_CAPTURED_V1`, `EQ_ENABLE_ACTIVE_LOW_BYPASS_V1`

Native K500 uses one shared three-byte EQ-enable image with `CMD 0x0F`:

```text
USB: AA 06 00 0F 40 M0 M1 M2 02 CHECKSUM
BT : AA 06    0F 40 M0 M1 M2 02 CHECKSUM
```

The important semantic is **active-low bypass**:

```text
relevant bit SET   = EQ ACTIVE / NOT BYPASSED
relevant bit CLEAR = EQ BYPASSED
```

This corrects the earlier implementation error that treated a set bit as
"bypass enabled".

## Physical reconnect proof — Music EQ

Evidence:

| Capture | SHA-256 |
| --- | --- |
| MusicEQ_Bypass_NotThicked.pcapng | `6188ab05ba7eda8dd6596b89be0de8e9b8099559062ff07da24a358dd03aec43` |
| MusicEQ_Bypass_Thicked.pcapng | `e7ca7c4074957e12756fd89b3179ba874e317a436821f9f8a47f2e5203da4b33` |

The reconstructed 939-byte active-memory snapshots are identical except for
one byte:

```text
Music EQ ACTIVE / native EQ Bypass unchecked:
activeMemory[0x027D] = FD

Music EQ BYPASSED / native EQ Bypass checked:
activeMemory[0x027D] = 7D

delta = 0x80
```

Therefore the Music mask is `0x80` and its polarity is proven:

```text
FD & 80 = 80  -> EQ active
7D & 80 = 00  -> EQ bypassed
```

## Shared image masks

Existing native sequential captures identify the section masks:

| Section | Byte | Mask |
| --- | ---: | ---: |
| Mic A/B shared | M0 / activeMemory[0x027D] | 0x60 |
| Music | M0 / activeMemory[0x027D] | 0x80 |
| Main | M1 / activeMemory[0x027E] | 0x01 |
| Surround | M1 / activeMemory[0x027E] | 0x04 |
| Center | M1 / activeMemory[0x027E] | 0x10 |
| Sub | M1 / activeMemory[0x027E] | 0x40 |
| Reverb | M2 / activeMemory[0x027F] | 0x01 |
| Echo | M2 / activeMemory[0x027F] | 0x02 |

A device state with all exposed EQ sections active was observed as:

```text
FD FF 03
```

which has every relevant enable bit set. Interpreting these bits as bypass bits
was the regression that made every SonKuPik section display `EQ BYPASS`.

## Read contract

```text
bypass(section) = (imageByte & sectionMask) != sectionMask
```

## Write contract

The application must preserve all unrelated bits and modify only the selected
section's enable mask:

```text
set bypass ON  -> clear section mask
set bypass OFF -> set section mask
```

Example for Music:

```text
ACTIVE image   FD FF 03
BYPASS Music   7D FF 03
ACTIVE Music   FD FF 03
```

Do not invert this semantic again. The image is best understood as an
**EQ-enable image**, with bypass represented by clearing enable bits.
