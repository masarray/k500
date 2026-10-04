# K500 Mic LP Filter-Type Capture Map

Status: physically captured 2026-10-04 with the manufacturer K500 application over USB HID.

## Evidence

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| `Connect_Mic_LPType_bypass_bessel12db_reconnect_butter12db_reconnect_bessel18db_RC_butter18db_RC_butter24db.pcapng` | 30,808 bytes | `6d3f7d233653674c60554c6e1dd88261a64a7937c68c13ec343ded12140c4a29` |

The capture begins with Mic LP Type at Bypass and then exercises Bessel 12, Butter 12, Bessel 18, Butter 18, and Butter 24 with reconnects between known states.

## Authoritative CONNECT/readback truth

Six reconstructed 939-byte snapshots isolate one and only one changing active-memory byte:

```text
activeMemory[0x0014] = Mic LP Type

Bypass     -> 0
Bessel 12  -> 1
Butter 12  -> 2
Bessel 18  -> 3
Butter 18  -> 4
Butter 24  -> 5
```

Across every adjacent reconnect in this capture, `0x0014` is the only active-memory byte that changes.

This is a **direct active-memory index**. It must not be translated with `fileU8()` or other `.k500` file-offset helpers.

The observed values align with the already-established shared filter enum:

```text
0 Bypass
1 Bessel 12
2 Butter 12
3 Bessel 18
4 Butter 18
5 Bessel 24
6 Butter 24
7 LR 24 / Link Riley 24
```

This file physically exercises 0..5. Codes 6..7 remain shared-enum compatibility for Mic LP until independently exercised on Mic LP.

## Captured WRITE frames and selector

At 16 kHz LP frequency, the native application emitted:

```text
Bessel 12  AA 06 00 11 01 01 80 3E 09 20
Butter 12  AA 06 00 11 01 02 80 3E 09 1F
Bessel 18  AA 06 00 11 01 03 80 3E 09 1E
Butter 18  AA 06 00 11 01 04 80 3E 09 1D
Butter 24  AA 06 00 11 01 05 80 3E 09 1C
```

Thus selector `0x01` is Mic LP and the type byte follows the shared enum.

### Important trailing-byte boundary

The final data byte is `0x09` in this 16 kHz capture. Older verified non-Music `CMD 0x11` vectors at 1 kHz carry `0x00`.

That contradiction is useful evidence: the final byte is not safe to generalize as a constant `0x00` or `0x09`. Its donor/meaning must be isolated before Mic LP WRITE is promoted as fully preservation-safe.

This capture therefore promotes **Mic LP READ truth only**. It records the observed WRITE selector/type bytes, but does not authorize hard-coding the trailing `0x09`.

## Explicit non-claims

- **Mic HP Type readback is not mapped by this capture.** A neighboring byte may look plausible, but no Mic HP dropdown transition was performed here.
- Main/Surround/Center/Sub filter-type readback is not inferred from Mic.
- The Mic LP trailing `CMD 0x11` byte is dynamic/unresolved.
- No new persistent/file offset is inferred from the live offset.

## Application contract

1. On CONNECT/reconciliation, decode Mic LP Type from direct `activeMemory[0x0014]`.
2. Update both Mic A and Mic B LP presentation without emitting an edit.
3. Preserve the existing Mic HP presentation/assumption until dedicated HP reconnect evidence exists.
4. Canonical state may mark Mic LP Type as captured truth while Mic HP remains assumed.
5. Do not change the existing Mic WRITE trailing-byte strategy from this capture alone.
