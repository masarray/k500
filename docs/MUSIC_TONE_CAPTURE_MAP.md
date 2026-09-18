# K500 Music Tone Native Capture Map

Status: write-side mappings captured from Wireshark/USBPcap exports on 2026-09-18.
These captures contain only K500 64-byte HID payload traffic after Wireshark display filtering/export.

## Evidence files

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| MusicNoiseGate_min50_OFF.pcapng | 17,168 bytes | 5a6ca678b704e56b53dec3e7d92d24c33934a4d61975a2f80040eba3a62e9008 |
| Bass_12_9_4_0_min5_min10_min12.pcapng | 28,824 bytes | f458a84ea8bad87eddf6201ac9e442c9e769f8ea98c27ce033b91fdac58fe366 |

## Music Noise Gate

Native app retransmits the full top-Music CMD 0x02 block. The only changing field in the supplied sweep is the byte after Music Key:

```text
AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 XX 13 CS
                                             ^^
```

Captured endpoint examples:

```text
OFF    raw 00: AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 00 13 24
-90 dB raw 01: AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 01 13 23
-50 dB raw 29: AA 0D 00 02 19 19 54 02 09 09 09 08 08 07 29 13 FB
```

Authoritative UI/value contract from the physical native app:

- raw `0x00` = OFF
- raw `0x01` = -90 dB
- ...
- raw `0x29` (41) = -50 dB

For raw 1..41:

```text
dB = raw - 91
raw = dB + 91
```

OFF is a dedicated sentinel and must not be treated as -91 dB.

The write-side field is now proven. Connect/readback ownership is still pending a dedicated known-state reconnect diff before SonKuPik should claim authoritative hydration.

## Music Bass

Native app uses CMD 0x0C with selector 0x02:

```text
AA 06 00 0C 02 00 RAW_LO RAW_HI 09 CHECKSUM
```

The capture sequence was +12 -> +9 -> +4 -> 0 -> -5 -> -10 -> -12. Long dwell periods identify the exact target raw values:

| UI Bass | Raw decimal | Raw LE | Exact captured frame |
| ---: | ---: | --- | --- |
| +9.0 dB | 210 | D2 00 | AA 06 00 0C 02 00 D2 00 09 11 |
| +4.0 dB | 160 | A0 00 | AA 06 00 0C 02 00 A0 00 09 43 |
| 0.0 dB | 120 | 78 00 | AA 06 00 0C 02 00 78 00 09 6B |
| -5.0 dB | 70 | 46 00 | AA 06 00 0C 02 00 46 00 09 9D |
| -10.0 dB | 20 | 14 00 | AA 06 00 0C 02 00 14 00 09 CF |
| -12.0 dB | 0 | 00 00 | AA 06 00 0C 02 00 00 00 09 E3 |

The complete captured sweep is linear in 0.1 dB units:

```text
raw = round((dB + 12.0) * 10)
dB  = raw / 10.0 - 12.0
```

Therefore the native range is -12.0..+12.0 dB and encoded raw range is 0..240.
The supplied file starts while moving away from +12, so raw 240 is formula-consistent but not emitted as a stationary target frame in this particular capture. All remaining named targets independently fit the same linear transform exactly.

As with Noise Gate, write-side mapping is proven; connect/readback mapping still needs a known-state reconnect diff before authoritative hydration is implemented.

## Next evidence needed

To close read-side mapping, use two short connect captures for each control:

### Noise Gate readback
1. Set native Music Noise Gate = OFF, disconnect, start/continue capture, reconnect and wait for full 939-byte readback.
2. Set Music Noise Gate = -50 dB, disconnect/reconnect again and wait for full readback.

### Bass readback
1. Set native Music Bass = 0.0 dB, reconnect and wait for full readback.
2. Set Music Bass = +12.0 dB (or -12.0 dB), reconnect and wait for full readback.

The stable active-memory delta becomes the authoritative connect-time decoder. Do not guess an offset from write frames alone.
