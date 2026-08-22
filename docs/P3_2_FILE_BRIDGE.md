# P3.2 — `.k500` File Bridge + Real Corpus

Status: **IMPLEMENTED IN BRANCH — CI GATE PENDING**

P3.2 connects the P3 byte-preserving codec to the Qt application without weakening the P0/P1/P2 device-control baseline.

## Added

- `K500PresetFileBridge` as a native Qt backend prepared for later QML wiring;
- safe `.k500` import from a local file URL;
- strict `0x0478` size validation;
- strict additive checksum validation before hydration;
- `.k500 -> 0x0290` conversion through the verified P3 converter;
- offline hydration through the same 939-byte `StudioEngine` decoder used by CONNECT;
- atomic `QSaveFile` export;
- no-op Save As writes the exact imported source bytes;
- `deviceSlotImage()` exposes the verified 656-byte native representation for the later P4 Upload/Mass Upload workflow;
- a real donor fixture copied byte-for-byte from `ktv-studio-mixer-pro/src/assets/sample.k500`;
- a corpus test that checks donor name, checksum, byte-identical no-op round trip, scalar `+8/+9` split, compact PEQ conversion, tail mapping, and 16-byte name mapping.

## Safety boundary

P3.2 deliberately validates the backend before exposing it to QML. The first CI run showed that Qt auto type registration itself could fail independently of the codec/file logic, so QML registration and FileDialog UI are deferred to P3.3 after this backend/corpus gate is green.

P3.2 also does **not** yet serialize arbitrary UI edits back into the source file. Until an edit is routed through the P3 explicit whitelist patcher, Save As remains source-byte exact.

This deliberately separates three guarantees:

1. **Load/preview/export safety** — implemented here.
2. **QML FileDialog/UI wiring** — P3.3, after backend acceptance.
3. **Editable file persistence** — requires per-parameter whitelist patch coverage before P4 device upload uses edited files.

The System page Upload/Mass Upload buttons remain disabled. P4 will connect the validated slot image to the existing P2 permanent Store/Mass Upload transaction engine only after file editing and UI paths are proven.
