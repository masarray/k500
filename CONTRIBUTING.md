# Contributing to SonKuPik K500

Contributions are welcome, especially reproducible K500 protocol evidence, hardware qualification results, Qt/QML UX improvements, preset tooling, documentation, and regression tests.

`main` represents the current stable baseline. New work should be developed on a focused branch and merged through a pull request after the relevant guards pass.

## Start here

Before changing hardware-facing or preset-facing code, read:

- [Architecture](docs/ARCHITECTURE.md)
- [Porting Parity Matrix](docs/PORTING_PARITY_MATRIX.md)
- [Protocol Golden Vectors](docs/PROTOCOL_GOLDEN_VECTORS.md)
- [Hardware Acceptance Checklist](docs/HARDWARE_ACCEPTANCE_CHECKLIST.md)
- [Bit-perfect Preset Guide](docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md)
- [Security Policy](SECURITY.md)

The donor/capture is the specification. Do not infer a command merely because a GUI field exists.

## Engineering rules

1. Preserve the native boundary: `QML -> StudioEngine -> Controller/transaction coordinator -> DeviceManager -> WinIo`.
2. Keep K500 readback authoritative on connect and recall.
3. Do not guess unverified offsets, command bytes, ACK semantics, or persistent Mode Name writes.
4. Preserve unknown/reserved bytes and raw aliases.
5. Add or update a golden vector when verified protocol behavior changes.
6. Add a regression test before enabling destructive functionality.
7. Fail closed when permanent device state is uncertain.
8. Never weaken an older guard simply to make new code pass; evolve it only when the underlying source of truth has genuinely changed.
9. Preserve stable section/model lifetimes; do not reintroduce a single hot-swapped EQ graph across incompatible 10/7/5-band models.
10. Keep official-preset updates and local-user presets isolated: official sync may replace validated official cache entries, never user files.

## `.k500` changes

A no-edit round trip must remain byte-identical. Controlled changes must pass explicit whitelist/diff checks and preserve raw aliases/reserved bytes. A permanent device slot image is **not** `file.left(0x0290)`; use the proven codec conversion.

### Official preset changes

Changes under `resources/presets/` are release-quality data changes and require the same care as hardware code.

A pull request changing an official preset should include:

- exact source/donor identity;
- file size and additive checksum validation;
- SHA-256 before/after;
- intended semantic changes;
- changed-byte audit;
- hardware listening/behavior evidence when the change is claimed as hardware-approved.

Mode 01 is currently pinned to the exact native `CONCERT HIFI V4` donor with SHA-256:

`9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74`

Do not reconstruct or normalize that file without new evidence and an explicit review.

## Pull requests

Keep PRs focused. Include:

- user-visible behavior and motivation;
- architecture/protocol impact;
- donor/capture or file-layout evidence where relevant;
- exact safety boundary;
- tests/guards added or updated;
- hardware acceptance state: `not required`, `pending`, or `passed with evidence`;
- screenshots for material UI/landing-page changes when practical.

A documentation-only PR should still keep release/version/support claims consistent across README, docs, landing page, changelog, and CI documentation guards.

## Hardware evidence

For physical K500 testing, record the exact commit SHA, app version, transport, firmware, Windows version, procedure, result, and Support Report / trace reference. Permanent Save/Upload/Mass Upload acceptance additionally requires reconnect and power-cycle verification when persistence is the subject of the test.

Bluetooth SPP remains experimental in the v1.0 support policy; a Bluetooth-specific promotion therefore needs independent evidence instead of inheriting USB acceptance.

## Development workflow

Typical flow:

```text
main
  -> focused branch
  -> implementation + tests/docs
  -> pull request
  -> exact-head CI
  -> hardware acceptance when required
  -> merge
```

Do not commit generated build folders, installers, Qt runtime deployments, local preset caches, or personal test artifacts.

## License

By submitting a contribution, you agree that your contribution is provided under the repository's **GPL-3.0-or-later** license.
