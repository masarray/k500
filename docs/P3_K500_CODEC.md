# P3 — Bit-perfect `.k500` codec

Status: **IMPLEMENTED IN BRANCH — CI/HARDWARE GATE PENDING**

## Contract

P3 treats the original `.k500` bytes as the source of truth. It does not normalize a complete preset object back into a new binary on a no-op edit.

Non-negotiable invariants:

- preset file length is `0x0478` / 1144 bytes;
- checksum byte is `0x0475` and the additive 8-bit sum of the whole file must be zero;
- visible preset name starts at `0x0454`, length `0x21`;
- parse → no-op serialize must be byte-identical;
- unknown/reserved bytes, PEQ raw aliases, name padding and `*Alt` blocks are preserved unless an explicit edit targets them;
- every mutation uses an explicit byte-offset whitelist;
- checksum is the only automatically permitted extra changed byte after a real mutation.

## Parser

`K500PresetCodec::Document` exposes the exact source bytes, checksum state, name, primitive little-endian reads and all 13 known EQ sections. Each EQ section retains:

- `enabledFlag`;
- exact `typeRaw` for every band;
- frequency, Q raw and signed gain raw;
- LP/HP type/frequency;
- the four unknown crossover-footer bytes unchanged.

No-op serialization is intentionally `serializeNoop()`: it returns the original byte array exactly.

## Controlled patching

`applyWhitelistedPatches()` rejects any requested byte that is outside the caller-provided whitelist. After the patch it diffs source vs output and fails if any unexpected offset changed. This prevents a future UI edit from silently rewriting unrelated firmware/reserved data.

## `.k500` → native device slot image

A permanent equipment slot is `0x0290` / 656 bytes, but it is **not** `preset.left(0x0290)`.

`buildDeviceSlotImage()` ports the verified donor/native mapping:

1. native scalar bytes `0x0000..0x00e6` are copied from `.k500` using the split mapping:
   - live `< 0x008f` → file `+0x08`;
   - live `>= 0x008f` → file `+0x09`;
2. 13 EQ sections are compacted from `.k500` 8-byte band records into native 5-byte records;
3. type is compacted to native bell / LS / HS nibble and gain sign/magnitude;
4. file bytes `0x044c..0x044f` map to slot `0x027c..0x027f`;
5. the first 16 preset-name bytes map to slot `0x0280..0x028f`.

This output is directly compatible with the P2 `massUploadSlotImages()` / Store transaction engine.

## Regression test

`k500_p3_selftest` verifies:

- valid 1144-byte file and checksum;
- byte-identical no-op round trip;
- PEQ `P` alias raw preservation (`0x0003` remains `0x0003` in the file model);
- signed EQ gain parsing;
- all 13 EQ sections;
- whitelist-only mutation;
- checksum repair after intentional mutation;
- preservation of an untouched `mainAlt` block;
- rejection of unauthorized offsets;
- exact scalar `+8/+9` slot mapping;
- 8-byte → 5-byte compact EQ conversion;
- tail/name slot mapping;
- explicit proof that slot conversion is not raw file slicing;
- rejection of malformed file sizes.

## Remaining P3 work before LOCKED

- run CI on Windows/MSVC and fix any compiler/test failures;
- validate against a corpus of real `.k500` files from different firmware generations;
- compare generated `0x0290` slot images against donor/native golden captures;
- expose safe file import/export and preset-to-slot conversion to the Qt application layer without weakening the byte whitelist contract.

Only after those gates pass should P3 be marked **LOCKED ✅** and P4 PC Preset Library / Mass Upload UI be enabled.
