# P3.3 — Safe `.k500` File UI

Status: **IMPLEMENTED IN BRANCH — CI GATE PENDING**

P3.3 exposes the validated P3.2 file bridge to the existing System workspace without registering a new QML hardware type or bypassing the native architecture.

## Architecture

`SystemWorkspace QML -> K500DeviceManager.presetFileBridge -> K500PresetFileBridge -> StudioEngine hydrate`

The file bridge is a lazy `QObject`, matching the existing P2 `presetManager` exposure pattern. It has no Controller or WinIo access.

## UI behavior

- Open a `.k500` file through native Qt Quick `FileDialog`.
- Show the actual source path, device-visible preset name and checksum status.
- Preview through the same 939-byte `StudioEngine` hydration path used by CONNECT.
- Save As writes the exact source bytes atomically; edited-file persistence is not claimed yet.
- Invalid size/checksum remains rejected by P3.2 backend validation.

## Safety boundary

PC preset preview is enabled only while the K500 is disconnected. This prevents an offline file from replacing the visible source-of-truth state while a hardware LIVE session is active.

`Upload to device` and `Mass upload` remain disabled. They will be enabled only after controlled UI-edit -> whitelist `.k500` patch persistence is proven and the generated `0x0290` image is connected to the P2 transactional store engine.
