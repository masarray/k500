# K500 Mic LP Filter-Type Capture Map

Status: physically captured 2026-10-04 with the manufacturer K500 application over USB HID.

## Evidence

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| `Connect_Mic_LPType_bypass_bessel12db_reconnect_butter12db_reconnect_bessel18db_RC_butter18db_RC_butter24db.pcapng` | 30,808 bytes | `6d3f7d233653674c60554c6e1dd88261a64a7937c68c13ec343ded12140c4a29` |

The capture begins with Mic LP Type at Bypass and then exercises the first five raw enum states with reconnects. The operator note labelled the final raw-5 state as Butter 24, but later independent physical evidence resolves the shared enum as raw 5 = Bessel 24 and raw 6 = Butter 24. The raw bytes from this first sweep remain valid; only that final human-readable label is superseded.

## Authoritative CONNECT/readback truth

Six reconstructed 939-byte snapshots isolate one and only one changing active-memory byte:

```text
activeMemory[0x0014] = Mic LP Type

Bypass     -> 0
Bessel 12  -> 1
Butter 12  -> 2
Bessel 18  -> 3
Butter 18  -> 4
Bessel 24  -> 5   (raw-5 state; corrected by later independent evidence)
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

This first file physically exercises raw 0..5. A later independent Mic LP donor-isolation capture physically exercises raw 6 = Butter 24 and raw 7 = LR 24, closing the high-order end of the shared enum.

## Captured WRITE frames and selector

At 16 kHz LP frequency, the native application emitted:

```text
Bessel 12  AA 06 00 11 01 01 80 3E 09 20
Butter 12  AA 06 00 11 01 02 80 3E 09 1F
Bessel 18  AA 06 00 11 01 03 80 3E 09 1E
Butter 18  AA 06 00 11 01 04 80 3E 09 1D
Bessel 24  AA 06 00 11 01 05 80 3E 09 1C
```

Thus selector `0x01` is Mic LP and the type byte follows the shared enum. The original session note calling the raw-`0x05` frame "Butter 24" is superseded by the later raw-`0x06` Butter 24 / raw-`0x07` LR 24 physical toggle and by the independent State-A output capture.

### Important trailing-byte boundary

The final data byte is `0x09` in this 16 kHz capture. Historical generic
non-Music vectors used `0x00`, but repository history shows that value came
from the original software builder rather than an independent physical Mic
capture.

The later donor-isolation capture below resolves the field: Mic `CMD 0x11`
mirrors the current Music Input1 Gain raw byte (`dB + 12`). Therefore `0x09`
is correct specifically when Input1 Gain is -3 dB; it is not a constant.

## Explicit non-claims

- The original Mic-LP sweep alone did not map Mic HP. The later State-A reconnect closes Mic HP at direct activeMemory[0x0013].
- Main/Surround/Center/Sub filter-type readback is established separately by the same State-A reconnect and is not inferred from Mic.
- The Mic LP trailing `CMD 0x11` byte is dynamic and capture-resolved to Music Input1 Gain raw.
- No new persistent/file offset is inferred from the live offset.

## Application contract

1. On CONNECT/reconciliation, decode Mic HP Type from direct `activeMemory[0x0013]` and Mic LP Type from direct `activeMemory[0x0014]`.
2. Update both Mic A and Mic B shared crossover presentation without emitting an edit.
3. Decode type raws through the shared 0..7 filter enum; in particular raw 5 = Bessel 24 and raw 6 = Butter 24.
4. Canonical state may treat both Mic HP/LP Type as captured reconnect truth.
5. Mic CMD 0x11 WRITE must preserve the current Music Input1 Gain raw byte; never hard-code `0x00` or `0x09`.


## Cross-mode donor-isolation capture — 2026-10-04

Additional evidence:

| Capture | SHA-256 |
| --- | --- |
| `Reconnect_ModeA_B_C_TopMusicTail_MicLPStateDonor.pcapng` | `e3b39a78019f2948e722553e51379c464720c553153ef12773998ba8df592bff` |

Sequence observed:

- Mode 1 CONNECT/readback.
- Music Key 0 -> +1 -> 0.
- Mic LP LR24 -> Butter24 -> LR24.
- Recall Mode 2, full readback, reconnect, repeat the edits.
- Recall Mode 3, full readback, reconnect, repeat the edits, final reconnect.

Six complete checksum-valid 939-byte snapshots were reconstructed. Stability is excellent:

```text
Mode 2 post-recall vs Mode 2 reconnect = 0 differing bytes
Mode 3 post-recall vs Mode 3 reconnect = 0 differing bytes
Mode 3 pre-final vs final reconnect     = 0 differing bytes
```

The three representative mode states are genuinely different:

```text
Mode 1 -> Mode 2 = 98 differing active-memory bytes
Mode 2 -> Mode 3 = 88 differing active-memory bytes
```

Despite those state changes, every physical Mic LP type write retained final byte
`0x09`, while the filter type itself explicitly confirms the shared high-order enum:

```text
Butter24  AA 06 00 11 01 06 80 3E 09 1B
LR24      AA 06 00 11 01 07 80 3E 09 1A
```

This later capture is the decisive correction for the first sweep's final display label: Mic LP uses raw `0x06` for Butter 24, not raw `0x05`.

The same pair repeats in Modes 1, 2 and 3.

### What this proves

The repository's historical non-Music `0x00` final byte originated in the
initial generic software builder; it was not independently established by a
physical Mic capture. The physical Mic LP evidence currently available therefore
agrees on `0x09` for type edits at 16 kHz across multiple Equipment Modes.

This is strong evidence for the native Mic-LP type-edit byte, but it does not yet
prove whether `0x09` is a fixed protocol constant or a preserved device field
that happens to remain equal across these modes.

If the byte is a direct single-byte donor from active memory, cross-mode
correlation reduces the candidate set to only:

```text
0x0016 0x0017 0x0018 0x00E8
0x011A 0x01DD 0x020F 0x0278
```

Do not promote any one candidate from equality alone.

### Minimal next capture

Before changing the generic Mic crossover writer, vary one candidate source and
then trigger a Mic LP type write. Highest-value first pass:

1. Music Input1 Gain -3 dB -> 0 dB, then LR24 -> Butter24 -> LR24.
2. Restore Input1 Gain.
3. Music Input2 Gain -3 dB -> 0 dB, repeat Mic LP toggle.
4. Restore Input2 Gain.
5. Bluetooth Gain -3 dB -> 0 dB, repeat Mic LP toggle.
6. Restore Bluetooth Gain.

If the final Mic `CMD 0x11` byte follows `0x09 -> 0x0C`, the donor is
identified. If it remains `0x09`, those three easy scalar candidates are
eliminated and the remaining EQ-byte candidates can be tested deliberately.


## Mic CMD 0x11 tail donor isolated — 2026-10-04

Additional physical capture:

| Capture | SHA-256 |
| --- | --- |
| `2 Mic LP tail 0x09 — eliminasi tiga kandidat termudah.pcapng` | `e3d58b92d9ddfa922a09dd2628b4c982e1036f30a4033c65dddca5b9a41a3cd3` |

The experiment varied the three easiest cross-mode candidates independently and
triggered the same Mic LP type toggle after each change.

### Music Input1 Gain

Input1 was moved from -3 dB to 0 dB, so its Top-Music raw value changed
`0x09 -> 0x0C`. Mic LP then emitted:

```text
Butter24  AA 06 00 11 01 06 80 3E 0C 18
LR24      AA 06 00 11 01 07 80 3E 0C 17
```

After Input1 returned to -3 dB, the Mic tail returned to `0x09`.

### Negative controls

With Input1 restored to raw `0x09`:

- Input2 Gain was changed independently to raw `0x0C`; Mic LP tail remained `0x09`.
- Bluetooth Gain was changed independently to raw `0x0C`; Mic LP tail remained `0x09`.

Therefore the donor is isolated, not merely correlated:

```text
Mic CMD 0x11 final data byte = Music Input1 Gain raw
                             = round(Input1GainDb + 12)
                             = activeMemory[0x0016]
```

This is the same scalar stored as file/preset offset `0x001E`; the live
Retrieve-All image translates it to direct active offset `0x0016`.

### Preservation rule

A Mic HP/LP write must serialize the **current canonical Music Input1 Gain**,
not a connect-time constant. This matters after a live Input1 edit: replaying
the old snapshot byte could silently roll Input1 back when the user later
changes Mic crossover.


## Mic HP reconnect closure — State A, 2026-10-04

The later `Reconnect_AllOutputFilterTypes_StateA.pcapng` snapshot supplies the
missing Mic HP reconnect evidence. The manufacturer UI shows:

```text
Mic HP  Bessel 12
Mic LP  Butter 12
HPF     96 Hz
LPF     16000 Hz
```

The full 939-byte readback resolves:

```text
activeMemory[0x0013] = 0x01 = HP Bessel 12
activeMemory[0x0014] = 0x02 = LP Butter 12
```

The frequency scalars decode to 96 / 16000 in the same snapshot, confirming the
Mic state is current rather than stale. This closes connect-time Mic HP
hydration at direct `0x0013`; the original LP sweep remains the stronger
multi-state evidence for `0x0014`.
