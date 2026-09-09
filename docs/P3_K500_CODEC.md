# P3 — Bit-perfect `.k500` Codec

Status: **LOCKED ✅ — part of the v1.0 stable baseline**

## Contract

P3 treats the original `.k500` bytes as the source of truth. It does not normalize a complete preset object back into a newly generated binary on a no-op edit.

Non-negotiable invariants:

- preset file length is `0x0478` / **1144 bytes**;
- checksum byte is `0x0475` and the additive 8-bit sum of the whole file must be zero;
- visible preset name starts at `0x0454`, physical field length `0x21`;
- hardware-safe visible preset name is **<= 16 characters**;
- parse → no-op serialize must be byte-identical;
- unknown/reserved bytes, PEQ raw aliases, name padding, and `*Alt` blocks are preserved unless an explicit edit targets them;
- every mutation uses an explicit byte-offset whitelist;
- checksum is the only automatically permitted extra changed byte after a real mutation.

## Parser

`K500PresetCodec::Document` exposes the exact source bytes, checksum state, name, primitive little-endian reads, and all 13 known EQ sections. Each EQ section retains:

- `enabledFlag`;
- exact `typeRaw` for every band;
- frequency, Q raw, and signed gain raw;
- LP/HP type/frequency;
- unknown crossover-footer bytes unchanged.

No-op serialization is intentionally byte-preserving: the source array is returned exactly.

## Controlled patching

`applyWhitelistedPatches()` rejects requested bytes outside the caller-provided whitelist. After mutation, source and output are diffed and the operation fails if an unexpected offset changed. This prevents UI/file edits from silently rewriting unrelated firmware or reserved data.

## `.k500` → native device slot image

A permanent equipment slot is `0x0290` / **656 bytes**, but it is **not** `preset.left(0x0290)`.

`buildDeviceSlotImage()` performs the donor/native mapping:

1. native scalar bytes `0x0000..0x00e6` are copied from `.k500` using the verified split mapping:
   - live `< 0x008f` → file `+0x08`;
   - live `>= 0x008f` → file `+0x09`;
2. 13 EQ sections are compacted from `.k500` 8-byte band records into native 5-byte records;
3. EQ type is compacted to native bell / LS / HS representation with signed gain;
4. file bytes `0x044c..0x044f` map to slot `0x027c..0x027f`;
5. the first 16 preset-name bytes map to slot `0x0280..0x028f`.

That 656-byte result feeds the same permanent Store transaction path used by single Upload and Mass Upload.

## Stable regression coverage

The P3/P3.2/P3.4 test set verifies:

- valid 1144-byte file and checksum;
- byte-identical no-op round trip;
- raw PEQ alias preservation;
- signed EQ gain parsing;
- all 13 EQ sections;
- whitelist-only mutation;
- checksum repair after an intentional mutation;
- preservation of untouched `*Alt` / unknown regions;
- rejection of unauthorized offsets;
- exact scalar split mapping;
- 8-byte → 5-byte compact EQ conversion;
- tail/name slot mapping;
- explicit proof that slot conversion is not raw file slicing;
- rejection of malformed sizes/checksums;
- controlled edit persistence against a real donor fixture;
- batch validation before Mass Upload.

The Windows stable-release pipeline reruns the preset regression suite before packaging.

## Official preset integrity

The v1.0 official library is bundled into the application and may also receive validated GitHub-backed updates. Remote files must pass the same `.k500` validation before they can replace a last-known-good official cache entry.

Mode 01 is intentionally pinned to the exact native `CONCERT HIFI V4` donor:

`SHA-256 9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74`

A checksum-valid file is not automatically native-equivalent. The Mode 01 recovery before v1.0 is the reason donor identity is now treated as a first-class regression concern.

## Change policy

A codec/layout change must preserve no-op byte identity, update the appropriate donor/golden tests, document the exact proven mapping, and physically revalidate any destructive hardware behavior it changes. Never weaken byte-preservation rules to make a new serializer or UI path easier to implement.
