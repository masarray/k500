# P3.3 — Safe `.k500` File UI

Status: **STABLE ✅ — device-truth staging contract + unified preset library**

P3.3 exposes the validated file bridge to the System workspace without registering a second hardware path or allowing a PC file selection to masquerade as K500 state.

## State authority

Connected K500 editor truth:

```text
K500 -> full 939-byte readback -> StudioEngine -> QML
```

PC preset staging:

```text
System UI -> K500PresetManager / K500PresetFileBridge -> validated .k500
```

The two remain intentionally separate.

## Unified PC preset collection

The v1.0 System/Mass Upload UI combines two preset sources in one collection:

### SONKUPIK

Official presets are always available from the bundled application resources. The app may asynchronously synchronize newer official `.k500` files from `masarray/k500/main/resources/presets` into a local official cache.

A remote file becomes eligible only after K500 validation succeeds. If synchronization fails or a file is invalid, the existing last-known-good cache/bundled preset remains available.

### LOCAL

The user may select a writable local folder containing personal `.k500` presets. Local files are scanned/validated and appear beside official entries with a distinct source identity.

Official sync never overwrites Local files.

## Selection / Preview behavior

- Selecting an Official or Local preset stages that file only.
- Selection alone does **not** hydrate `StudioEngine` and does not change K500 audio.
- **Preview** is explicit and creates an offline preview/edit session.
- Controlled edits are tracked against the staged preset only while that preview mode is valid.
- Loading another preset resets the edit session.
- Connecting a K500 restores hardware authority and prevents LIVE hardware edits from silently mutating the staged PC file.
- `Save As` writes through the atomic byte-preserving/whitelist path.

## Single Upload behavior

Single preset Upload is explicit and USB/store gated:

1. selected PC preset is validated and converted to a native `0x0290` / 656-byte slot image;
2. Store uses that PC image directly — no fresh device readback may replace it before Store;
3. after commit, the destination slot is recalled;
4. `RSP 0xC0` is required;
5. the K500 is re-read for the full 939-byte active memory;
6. only then does `StudioEngine`/QML become the newly active hardware state;
7. normal LIVE editing resumes through the existing canonical path.

## Mass Upload transfer-list behavior

Mass Upload uses a reviewable transfer-list window:

- left list: unified **SONKUPIK + LOCAL** validated collection;
- official bundled presets keep the list useful even with no Local folder and no network;
- left list is not capped to 10 entries;
- right list: explicit Device Slot 01…10 mapping, maximum 10 entries;
- Add / Add All / Remove / Clear operate on staging only;
- visible right-list row 1 maps to Slot 01 through row 10 -> Slot 10;
- the entire batch is validated before device writes begin;
- the permanent transaction engine then performs the native descending hardware sequence, highest selected slot down to Slot 01;
- for a full bank this is **Slot 10 -> 09 -> … -> 01**;
- after the batch, Slot 01 is recalled and the full active memory is refreshed before LIVE returns.

The visual ascending slot map and descending transmission order serve different purposes. The latter is protocol parity, not a UI sort preference.

## Official Mode 01 integrity

The v1.0 official Mode 01 is the exact native `CONCERT HIFI V4` donor:

```text
resources/presets/01_ALL_GENRE.k500
SHA-256 9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

It replaced an earlier reconstructed file after physical hardware testing exposed a music-path failure during Mass Upload. The native donor identity is now guarded as part of the stable baseline.

## Safety boundary

PC preset selection is staging-only even while a K500 is connected. Permanent hardware changes occur only through explicit Upload/Mass Upload actions after validation and transaction gating.

The UI may improve how sources are presented, searched, tagged, or refreshed, but it must not collapse Hardware State, Staged PC Preset, Offline Preview, and Mass Upload Staging into one ambiguous state.
