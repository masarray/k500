# SonKuPik K500 — AI / Agent Entry Point

This file is the mandatory restart point for a new ChatGPT/Codex/AI thread working on this repository. It exists so engineering can continue from repository truth instead of hidden conversation history.

## Current stable baseline

- Product: **SonKuPik K500**
- Public stable line: **v1.0.x**
- Qualified v1.0 transport scope: **Windows x64 + USB HID**
- Bluetooth SPP: implemented, **experimental / not yet v1.0 hardware-qualified**
- QML never owns raw device I/O.
- Device readback is authoritative after connect and recall.
- Unsupported hardware commands remain read-only; never guess packets.
- Stable section navigation uses fixed EQ graph/model lifetimes; do not hot-swap incompatible 10/7/5-band models through one graph instance.
- Official preset sync is validation-gated and isolated from Local user presets.

The exact native Mode 01 donor is:

```text
resources/presets/01_ALL_GENRE.k500
internal name: CONCERT HIFI V4
SHA-256: 9aebeb908295abda1182ddbadc3aa537ea16b4cfea241b64b5a5180e66670e74
```

Do not reconstruct, normalize, or replace Mode 01 casually. A change requires explicit evidence, a byte-diff audit, and an updated golden reference.

## Read order by task

### Application / protocol / UX work

Read:

1. `README.md`
2. `docs/ARCHITECTURE.md`
3. `docs/PORTING_PARITY_MATRIX.md`
4. `docs/PROTOCOL_GOLDEN_VECTORS.md`
5. `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md`
6. `CONTRIBUTING.md`

### `.k500` preset analysis, sonic tuning, simulation, graphs, or new presets

Read:

1. `docs/K500_PROVEN_SONIC_BASELINE.md`
2. `docs/K500_AI_PRESET_ENGINEERING_PLAYBOOK.md`
3. `docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md`
4. `docs/K500_PRESET_NAME_LIMIT.md`
5. `tools/k500_preset_lab.py`

Do not improvise the `.k500` format from memory when the repository provides the proven map.

## Architecture invariants

Normal LIVE control path:

```text
QML
  -> StudioEngine
  -> K500Controller
  -> K500DeviceManager
  -> K500WinIo
  -> K500 hardware
```

Transactional preset path:

```text
System UI
  -> K500PresetManager
  -> K500DeviceManager
  -> K500WinIo
```

State layers must remain distinct:

1. **Hardware State** — actual K500 readback, C0 active slot, device mode names.
2. **Staged PC Preset** — selected `.k500`; selection alone never changes hardware.
3. **Offline Preview State** — explicit Preview hydration/edit session only.
4. **Mass Upload Staging** — explicit Slot 01…10 transfer mapping before permanent write.

A refactor that makes one layer masquerade as another is a regression even if the UI looks simpler.

## Stable preset transaction truth

- Active-memory readback is exactly `0x03AB` = **939 bytes**.
- Native permanent slot image is exactly `0x0290` = **656 bytes**.
- A `.k500` file is exactly `0x0478` = **1144 bytes**.
- Store: `CMD 0x41` begin → `CMD 0x42` chunks → `CMD 0x43` commit.
- Recall: `CMD 0x01` → settle → `CMD 0x3F` → require `RSP 0xC0` → full 939-byte readback.
- Mass Upload validates the whole batch first and uses the proven descending hardware order **10 → 1**.
- After a full bank, Slot 01 is recalled and hardware is re-read before LIVE resumes.
- Use Init Volume OFF: `AA 03 12 00 03 E8`.
- Use Init Volume ON: `AA 03 12 01 03 E7`.
- Use Init ACK: `0xED`.

Do not replace these transactions with a more convenient sequence without donor/capture evidence and corresponding golden-vector changes.

## Official + Local preset library

The Mass Upload source list is a unified collection:

- **SONKUPIK** — bundled official presets plus validated GitHub cache updates;
- **LOCAL** — user-owned presets in the selected local folder.

Rules:

- app must remain useful offline through bundled official presets;
- remote official files must validate before cache promotion;
- failed/invalid sync must preserve last-known-good official cache;
- official sync must never overwrite Local user files;
- transfer list remains max 10 device slots;
- UI mapping is ascending Slot 01…10 while hardware execution is descending.

## Crash-proof UI rule

A previous design reused one `SectionEqGraph` while switching between models with different band counts. In deployed Qt this could produce invalid transitional state and heap corruption when crossing sections such as Mic ↔ Reverb.

The stable baseline uses fixed page/model ownership for Mic A, Mic B, Reverb, Echo, Main, Surround, Center, and Sub. Navigation changes which page is visible; it does not hot-swap a graph's model identity.

Any navigation refactor must keep the runtime section stress test green. Never remove that test because a new design appears visually correct.

## Non-negotiable `.k500` rules

- file size: exactly **1144 bytes / `0x0478`**;
- checksum byte: `0x0475`;
- valid file: `sum(all bytes) % 256 == 0`;
- visible hardware-safe preset name: **<= 16 characters**;
- start from a known-good donor; never build from a zero-filled buffer;
- patch only proven offsets;
- preserve unknown/reserved bytes and raw PEQ aliases;
- preserve `mainAlt`, `surroundAlt`, `centerAlt`, `subAlt` unless intentionally targeted by a proven experiment;
- no-op must be byte-identical;
- checksum is recomputed last;
- every mutation ends with a changed-byte audit.

## Current sonic reference

The current **gold music reference** remains:

```text
resources/presets/03_DANGDUT_SUPREME.k500
internal/hardware name: DGT HIFI CORE V3
```

Its successful hardware-listening progression was:

```text
Core V1: bass became enjoyable; mid still ordinary
Core V2: mid refinement -> mid became enjoyable
Core V3: small very-bottom extension -> overall result became enjoyable
```

Do not casually rebuild that music core from a generic karaoke curve.

Product-level rule:

> All modes should preserve their own vocal identity while giving the listener a consistent premium music-enhancement benefit. Enhancement means listening enjoyment, not maximum loudness.

When the user reports that a region is already good, treat it as a **LOCK** for the next revision unless explicitly asked to revisit it.

### Mode 01 exception / authority update

Mode 01 is no longer a reconstructed Hi-Fi transplant. The repository now intentionally carries the exact native `CONCERT HIFI V4` donor that passed the physical K500 test. For Mode 01, **native donor identity wins over older modeled transplant assumptions** in historical sonic notes.

## Signal-flow model

```text
Music Input -> Music PEQ/XO -------------------------> output routing -> output PEQ/XO

Mic A/B -> mic gain/dynamics -> Mic PEQ/XO -> dry --+-> Main / Center / Surround / Sub
                                                     |
                                                     +-> Reverb PEQ/XO -> wet returns --+
                                                     +-> Echo PEQ/XO  -> wet returns ----+-> output PEQ/XO
```

Dry Mic, Reverb, and Echo are parallel contributions before each output path. Do not model Reverb and Echo as serial inserts.

Conceptual output roles:

- **Main** = front image, tonal body, music punch, primary vocal.
- **Center** = lead/vocal anchor.
- **Surround** = width, air, ambience, decorrelation; not a second Main.
- **Sub** = deep music foundation; normally keep Mic/Reverb/Echo out unless deliberately proven useful.

## Preferred preset-research workflow

```text
1. read the proven sonic baseline and identify GOLD / LOCKED regions
2. define one listening goal / failure mode
3. choose the closest proven donor
4. inspect donor + reference presets
5. form a narrow sonic hypothesis
6. simulate cumulative paths and guardrail regions
7. patch only intended fields
8. recompute checksum
9. byte-diff audit
10. generate graphs / CSV / JSON
11. test on real K500 hardware
12. use listening feedback to choose the next small change
```

Useful commands:

```bash
python tools/k500_preset_lab.py validate preset.k500
python tools/k500_preset_lab.py inspect preset.k500 --json audit.json
python tools/k500_preset_lab.py plot preset.k500 --out-dir analysis/
python tools/k500_preset_lab.py compare old.k500 new.k500 --out-dir comparison/
python tools/k500_preset_lab.py patch donor.k500 patch.json candidate.k500
```

Plot/compare dependencies:

```bash
python -m pip install -r tools/requirements-preset-lab.txt
```

## Pull-request / release discipline

`main` is the stable baseline. New engineering should use a focused branch and pull request.

Before merge:

- preserve exact-head CI evidence;
- do not weaken older guards to make new work pass;
- physically revalidate destructive hardware behavior when the change affects it;
- update README/docs/changelog/landing page when a public contract changes;
- keep Bluetooth claims separate from USB acceptance until Bluetooth is independently tested.

A new AI thread should prefer repository evidence over remembered chat context. If a fact conflicts, the current stable code, golden vectors, exact donor files, and evidence-backed documentation are authoritative.
