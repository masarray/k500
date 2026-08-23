# K500 Preset Name Limit — Hardware Loader Rule

This document records a **hard compatibility rule** discovered from real K500 preset loading.

## Proven rule

The binary preset contains a physically larger name field:

```text
NAME_OFFSET = 0x0454
NAME_LENGTH = 0x21   # 33 bytes physical storage
```

However, the K500 preset loader accepts a **maximum visible preset name of 16 characters**.

> **Hardware-safe rule: preset name length MUST be <= 16 characters.**

Do **not** infer the allowed visible name length from the 33-byte physical field size.

A preset can still have:

- correct 1144-byte file size;
- valid modulo-256 checksum;
- otherwise valid parameter bytes;

and still fail to load if the visible preset name exceeds 16 characters.

## Required behavior for preset generators / AI patchers

1. Preserve the original name field bit-for-bit unless a rename is explicitly required.
2. If renaming, reject or truncate any requested visible name longer than 16 characters **before writing the binary**.
3. Preserve the source preset's padding convention whenever possible.
4. Recompute checksum only after all intended writes are complete.
5. Perform a post-write byte-diff audit.

Recommended validation:

```python
MAX_K500_PRESET_NAME_CHARS = 16
NAME_OFFSET = 0x0454
NAME_FIELD_LENGTH = 0x21

assert len(visible_name) <= MAX_K500_PRESET_NAME_CHARS
```

For ASCII preset names, a safe writer is conceptually:

```python
encoded = visible_name.encode("ascii")
assert len(visible_name) <= 16
assert len(encoded) <= 16

# Preserve the source field unless a rename is intentional.
# When writing a new name, follow the padding convention of the source firmware/preset.
```

## Compatibility note

Observed old K500 V2.00 presets may use a 16-byte visible-name region with space padding followed by zero bytes. This reinforces that the **visible-name contract is 16 characters even though the physical container reserves 33 bytes**.

## Failure mode that exposed this rule

A generated preset used the visible name:

```text
QORI SHOLAWAT HADROH
```

The file remained 1144 bytes and checksum-valid, but the K500 loader rejected it. The name exceeded the hardware-safe 16-character limit.

Therefore, checksum validity alone is **not** sufficient preset-loader validation.

## AI / automation hard rule

When generating `.k500` presets:

```text
physical field capacity != hardware-visible name limit

33-byte field
    !=
16-character allowed preset name
```

Treat `16 characters` as the maximum until new controlled hardware evidence proves otherwise.

This rule should be used together with `docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md` and is intended to become part of the authoritative bit-perfect modification contract.
