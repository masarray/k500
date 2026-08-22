# Contributing

Contributions are welcome, especially reproducible K500 protocol evidence, hardware qualification results, Qt/QML UX improvements and regression tests.

## Before changing hardware-facing code

Read:

- `docs/PORTING_PARITY_MATRIX.md`
- `docs/PROTOCOL_GOLDEN_VECTORS.md`
- `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md`
- `docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md`

The donor/capture is the specification. Do not infer a command merely because a GUI field exists.

## Development rules

1. Preserve the architecture boundary: QML -> StudioEngine -> Controller/transaction coordinator -> DeviceManager -> WinIo.
2. Keep device state authoritative on connect/recall.
3. Do not guess unverified offsets, command bytes or ACK semantics.
4. Preserve unknown/reserved bytes.
5. Add or update a golden vector when protocol behavior changes.
6. Add a regression test before enabling destructive functionality.
7. Fail closed when permanent device state is uncertain.
8. Never weaken an older milestone guard simply to get a new milestone green; evolve the guard only when its declared dependency has genuinely been satisfied.

## `.k500` changes

A no-edit round trip must remain byte-identical. Controlled changes must pass the explicit whitelist/diff check and preserve raw aliases/reserved bytes. A native permanent slot image is **not** `file.left(0x0290)`.

## Pull requests

A focused PR should include:

- what user-visible behavior changes;
- donor/capture or file-layout evidence;
- exact safety boundary;
- automated tests added/updated;
- hardware acceptance state: not required / pending / passed with evidence.

Do not mark physical hardware acceptance as passed from CI alone.

## Hardware evidence

For real K500 testing, attach the exact commit SHA, transport, firmware, Windows version and a Support Report / trace reference. Permanent Save/Upload acceptance additionally requires reconnect and power-cycle verification.

## License

By submitting a contribution, you agree that your contribution is provided under the repository's GPL-3.0-or-later license.
