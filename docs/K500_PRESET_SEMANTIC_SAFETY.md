# K500 Preset Semantic Safety

Date: 2026-10-06

## Problem

Container validity is necessary but not sufficient:

```text
1144 bytes + additive checksum == structurally valid .k500
```

A structurally valid file can still contain a PEQ/scalar value that the native
656-byte equipment slot cannot represent exactly. The old converter silently
clamped Q/gain magnitudes and treated unknown PEQ types as Bell.

## PR-C contract

```text
.k500
  -> container validation
  -> strict native-slot semantic validation
  -> exact 0x0290 encoding (no clamp/fallback)
  -> shadow projection verification
  -> Preview / Mass Upload / device transfer
```

### Strict PEQ representation

| Field | Native exact domain |
|---|---|
| Frequency | 20..20000 Hz |
| Q raw | 1..250 (Q 0.1..25.0) |
| Gain raw | -240..240 (0.1 dB units) |
| Bell type | raw aliases 0x0000..0x0003 |
| Low Shelf | 0x0100 |
| High Shelf | 0x0200 |

Anything outside these domains fails conversion. Unknown types are never mapped
to Bell as a fallback.

### Evidence-backed scalar guard

Every field in `K500FieldContract::EvidenceBackedScalars` must satisfy its raw
range before slot conversion. Current critical examples include Mic HP/LP type
and FBE/FBX.

### Shadow verification

`verifyDeviceSlotProjection()` independently checks:

- all scalar file-to-active bytes across the verified +8/+9 geometry;
- every compact 5-byte native PEQ record;
- the 4-byte native slot tail;
- the 16-byte hardware-visible preset name.

The builder returns an empty image if shadow verification fails.

## Offline editing

A whitelisted edit is not committed merely because its checksum is correct.
`K500PresetFileBridge` runs strict compatibility before accepting working bytes.
Save/export is also blocked when the working document is not hardware-compatible.

```text
offline file intent == preview intent == uploaded native slot intent
```

## Differential production/research lock

`tools/k500_preset_lab.py` has an independent Python strict native-slot encoder.
The Windows consolidated regression computes slot SHA-256 through C++ and Python
for the same real donor and requires exact equality.

Fast Contracts validates the complete bundled official preset bank through the
Python semantic gate.

## Official cache promotion

Remote official presets use the same semantic compatibility gate as local
Preview/Save/Upload. A downloaded file is promoted only when both conditions hold:

1. strict K500 native-slot compatibility succeeds;
2. the downloaded bytes reproduce the exact Git blob SHA advertised by the
   GitHub catalog entry.

The cache decision is derived from the cached bytes themselves plus the current
catalog SHA, not merely from remembered settings. A failed semantic or blob
integrity check keeps the last-known-good cached/bundled preset.

## Change rule

Do not widen domains merely to make a failing preset pass. New values/types need:

1. physical K500/native-app evidence;
2. updated field/native range contract;
3. C++ and Python serializer updates;
4. deterministic regression;
5. hardware acceptance.
