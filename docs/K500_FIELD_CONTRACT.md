# K500 Canonical Preset Field Contract

Date: 2026-10-06

## Purpose

`src/k500/K500FieldContract.h` is the single C++ authority for the proven scalar
projection between `.k500` preset-file offsets and native active-memory / slot
offsets. It prevents Controller, Protocol, PresetCodec and PresetEditMapper from
carrying independent copies of the +8/+9 mapping or assigning different meanings
to the same raw byte.

## Scalar geometry

```text
file 0x0008..0x0096 -> active 0x0000..0x008E   (file - 0x08)
file 0x0097         -> structural hole / not a scalar
file 0x0098..0x00EF -> active 0x008F..0x00E6   (file - 0x09)
```

The contract exposes both directions and compile-time boundary assertions.
Preset-slot construction consumes the inverse mapping instead of duplicating a
delta/split loop.

## Evidence policy

`EvidenceLevel` distinguishes structural knowledge from captured fields and
round-trip-proven fields. `FileMutationPolicy` distinguishes fields that may be
written from fields that must be preserved. A field enters this contract only
after its evidence has landed in `main`; temporary parallel-branch ownership is
never treated as permanent protocol truth.

Current critical contracts:

| Field | File | Active | Raw | Evidence | File policy |
|---|---:|---:|---:|---|---|
| Mic HP type | `0x001B` | `0x0013` | `0..7` | Captured | PreserveOnly |
| Mic LP type | `0x001C` | `0x0014` | `0..7` | Captured | PreserveOnly |
| Mic FBE/FBX | `0x0023` | `0x001B` | `0..4` | ProvenRoundTrip | Writable |
| Adj Manner / VR OFF | `0x0094` | `0x008C` | `0..1` | ProvenRoundTrip | Writable |

`PreserveOnly` means the scalar must not become writable merely because an offset
is known. A dedicated physical experiment and regression evidence are required to
promote that policy. Adj Manner / VR OFF was promoted only after the paired
connect/reconnect captures and native parity work were merged.

## Engineering rule

New scalar reverse-engineering must land in this order:

```text
capture evidence
 -> evidence fixture / notes
 -> K500FieldContract field spec
 -> consumer migration
 -> self-test + existing regression suite
```

Do not introduce a second file-to-active translation helper or duplicate a
critical scalar magic offset in a consumer.
