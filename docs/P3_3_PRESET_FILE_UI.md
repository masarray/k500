# P3.3 — Safe `.k500` File UI

Status: **IMPLEMENTED — DEVICE-TRUTH STAGING CONTRACT**

P3.3 exposes the validated P3.2 file bridge to the existing System workspace without registering a new QML hardware type or bypassing the native architecture.

## Architecture

Connected K500 editor truth remains:

`K500 -> full 939-byte readback -> StudioEngine -> QML`

PC preset staging remains separate:

`SystemWorkspace QML -> K500DeviceManager.presetFileBridge -> K500PresetFileBridge`

The file bridge is a lazy `QObject`, matching the existing P2 `presetManager` exposure pattern. It has no Controller or WinIo access.

## UI behavior

- Open or select a validated `.k500` file into the **PC staging document**.
- Show the source/device-visible preset name and checksum status in the PC library area.
- **Do not hydrate `StudioEngine` from PC preset selection.** The editor must keep showing the actual connected K500 state.
- LIVE fader/PEQ edits must not silently mutate the staged PC preset.
- `Save As` writes the staged source document atomically.
- Invalid size/checksum remains rejected by P3.2 backend validation.

## Upload behavior

Single preset Upload is explicit and USB-store gated:

1. selected PC preset is already validated and converted to a native `0x0290` slot image;
2. Store uses that PC image directly — no fresh device readback may replace it before Store;
3. after commit, the destination slot is recalled;
4. the K500 is re-read for the full 939-byte active memory;
5. only then does `StudioEngine`/QML change to the newly active hardware state;
6. normal LIVE editing resumes through the existing canonical path.

This makes the hardware, not the PC selection, the source of truth for the main editor.

## Mass Upload transfer-list behavior

Mass Upload uses a reviewable transfer-list window:

- left list: local PC preset collection, unlimited by UI design;
- right list: explicit Device Slot 01…10 mapping, maximum 10 entries;
- visible right-list order maps row 1 -> Slot 01 through row 10 -> Slot 10;
- the P2 transaction engine then performs the **native descending hardware sequence**, highest selected destination slot down to Slot 01;
- for a full bank this is **Slot 10 -> 09 -> … -> 01**;
- after the batch, donor-compatible behavior recalls Slot 01 and refreshes active memory.

The descending transmission order is protocol parity, not a UI sorting preference.

## Safety boundary

PC preset selection is staging-only even while a K500 is connected. Selection alone must never change audio, faders, PEQ, active slot, or device memory.

Permanent hardware changes occur only through explicit `Upload` / `Mass Upload` actions after validation and USB transaction gating.
