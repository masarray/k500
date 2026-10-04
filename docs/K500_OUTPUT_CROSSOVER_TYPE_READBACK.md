# K500 Output Crossover Type Reconnect Map

Status: physical USB reconnect capture mapped 2026-10-04.

## Evidence

| Capture | Bytes | SHA-256 |
| --- | ---: | --- |
| `Reconnect_AllOutputFilterTypes_StateA.pcapng` | 5,512 | `4f55ca87d6f30ac329b23fd222dd16990ae812496bd3d72886619343677d3edb` |

The capture contains one clean manufacturer-app CONNECT sequence:
heartbeat, handshake, then 17 `CMD 0x40` blocks reconstructing exactly
939 bytes (`0x03AB`) of active memory. No user live-write commands occur
inside the capture, so the resulting image is a pure reconnect snapshot.

## Screenshot / hardware state

The supplied screenshots and the reconstructed readback agree on:

| Section | HP type | LP type | HPF | LPF |
| --- | --- | --- | ---: | ---: |
| Mic | Bessel 12 | Butter 12 | 96 Hz | 16000 Hz |
| Main | Bessel 18 | Butter 18 | 40 Hz | 20000 Hz |
| Surround | Bessel 24 | Butter 24 | 115 Hz | 16000 Hz |
| Center | LR 24 | Bessel 12 | 95 Hz | 16500 Hz |
| Subwoofer | **LR 24** | **Butter 12** | 35 Hz | 90 Hz |

Important: the typed preparation note said Sub HP=Butter12 / LP=LR24, but the
actual screenshot and the physical readback show the opposite:
**Sub HP=LR24 / LP=Butter12**. Capture truth takes precedence.

Shared native filter enum remains:

```text
0 Bypass
1 Bessel 12
2 Butter 12
3 Bessel 18
4 Butter 18
5 Bessel 24
6 Butter 24
7 LR 24 / Linkwitz-Riley 24
```

## Authoritative active-memory type bytes

The State-A design intentionally uses different enum values per section. The
readback resolves to a regular output-block pattern:

| Section | HP active offset | HP raw | LP active offset | LP raw |
| --- | ---: | ---: | ---: | ---: |
| Mic | `0x0013` | 1 | `0x0014` | 2 |
| Main | `0x002C` | 3 | `0x002E` | 4 |
| Surround | `0x0040` | 5 | `0x0042` | 6 |
| Center | `0x0054` | 7 | `0x0056` | 1 |
| Subwoofer | `0x0068` | 7 | `0x006A` | 2 |

The output sections form a structural stride:

```text
Main block base      0x001C -> HP type base+0x10 = 0x002C, LP type base+0x12 = 0x002E
Surround block base  0x0030 -> HP type base+0x10 = 0x0040, LP type base+0x12 = 0x0042
Center block base    0x0044 -> HP type base+0x10 = 0x0054, LP type base+0x12 = 0x0056
Sub block base       0x0058 -> HP type base+0x10 = 0x0068, LP type base+0x12 = 0x006A
```

These are **single-byte active-memory fields**. The intervening bytes are not
part of the enum and must not be consumed as a uint16 type value.

The existing Mic LP mapping at `activeMemory[0x0014]` is independently
reconfirmed. This capture additionally identifies Mic HP at
`activeMemory[0x0013]`.

## Frequency cross-check

The independent crossover-frequency words in the same 939-byte snapshot decode:

```text
Mic       HP 96 / LP 16000
Main      HP 40 / LP 20000
Surround  HP 115 / LP 16000
Center    HP 95 / LP 16500
Sub       HP 35 / LP 90
```

They match the supplied screenshots exactly, confirming this is the intended
State-A snapshot rather than a stale Equipment Mode.

## Existing WRITE selectors

This reconnect capture maps READ state. The already-captured `CMD 0x11`
selector family remains:

| Section | HP selector | LP selector |
| --- | ---: | ---: |
| Mic | `0x00` | `0x01` |
| Main | `0x04` | `0x05` |
| Surround | `0x08` | `0x09` |
| Center | `0x0C` | `0x0D` |
| Subwoofer | `0x0E` | `0x0F` |

Together with the shared 0..7 enum, the readback side can now be implemented
without assuming default filter labels.

## Critical collision discovered: output-delay READ offsets

This capture falsifies the previous Main/Center/Sub delay **readback** assumption.

The current code/documentation interprets:

```text
file 0x0034 / active 0x002C as Main L Delay
file 0x0036 / active 0x002E as Main R Delay
file 0x005C / active 0x0054 as Center Output Delay
file 0x0070 / active 0x0068 as Sub Output Delay
```

But State A proves those live bytes are crossover filter types:

```text
active 0x002C = 3 = Main HP Bessel18
active 0x002E = 4 = Main LP Butter18
active 0x0054 = 7 = Center HP LR24
active 0x0068 = 7 = Sub HP LR24
```

The screenshots show Main delays 0/0 ms, Center delay 0 ms and Sub delay 0 ms,
so decoding those type bytes as 3/4/7/7 ms is demonstrably incorrect.

Do **not** repair this by guessing another offset.

A promising contiguous six-word region exists at active
`0x00CB..0x00D6` (file-like positions `0x00D4..0x00DF`) and currently reads:

```text
00 00 | 00 00 | 0E 00 | 14 00 | 00 00 | 00 00
  0       0       14      20      0       0
```

This exactly matches the screenshot sequence
Main L=0, Main R=0, Surround L=14, Surround R=20, Center=0, Sub=0 if ordered
that way, and the Surround 14/20 pair agrees with independently captured
Surround delay truth. However, four zero-valued fields are not enough to
uniquely assign every word. Keep Main/Center/Sub delay readback evidence-gated
until a reconnect capture uses distinct non-zero delay values.

## Minimal delay-isolation capture

Use one reconnect state:

```text
Main L       5 ms
Main R      10 ms
Surround L  15 ms
Surround R  20 ms
Center      25 ms
Sub         30 ms
```

Then disconnect -> start capture -> connect -> wait full readback -> disconnect.

If active `0x00CB..0x00D6` becomes
`05 00 0A 00 0F 00 14 00 19 00 1E 00`, the complete delay READ block is
isolated in one shot.

## Safety boundary

- Promote the filter-type bytes above as direct active-memory truth.
- Do not infer Reverb/Echo filter-type bytes from this output pattern.
- Do not treat the type-byte offsets as delay words.
- Do not rewrite preset-file delay offsets solely from this reconnect capture;
  live active memory and .k500 persistence layout must be qualified separately.
