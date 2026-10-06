# SonKuPik K500 — AI / Agent Entry Point

This file is the mandatory restart point for a new ChatGPT/Codex/AI thread working on this repository. It exists so engineering can continue from repository truth instead of hidden conversation history.

## Current stable baseline

- Product: **SonKuPik K500**
- Public stable line: **v1.1.0**
- Qualified stable transport scope: **Windows x64 + USB HID**
- Bluetooth SPP: implemented, **experimental / not yet hardware-qualified**
- QML never owns raw device I/O.
- Device readback is authoritative after connect and recall.
- Unsupported hardware commands remain read-only; never guess packets.
- Stable section navigation uses fixed EQ graph/model lifetimes; do not hot-swap incompatible 10/7/5-band models through one graph instance.
- Official preset sync is validation-gated and isolated from Local user presets.

The current official Mode 01 is:

```text
resources/presets/01_KONSER_NYANYI.k500
internal name: KONSER NYANYI
SHA-256: 761d0ecf1f470ce433fcf760d7ee1317e994dbefbb16fc71e8498aea9d99d6c4
```

It was evolved surgically from the exact native `CONCERT HIFI V4` donor. Preserve the native donor as rollback/provenance evidence; future official changes require listening evidence, a byte-diff audit, and an updated golden reference.

## Read order by task

### Application / protocol / UX work

Read:

1. `README.md`
2. `docs/ARCHITECTURE.md`
3. `docs/PORTING_PARITY_MATRIX.md`
4. `docs/PROTOCOL_GOLDEN_VECTORS.md`
5. `docs/K500_CAPTURE_TODO_MAP.md`
6. `docs/MUSIC_TONE_CAPTURE_MAP.md`
7. `docs/K500_MUSIC_CROSSOVER_TYPE_CAPTURE.md`
8. `docs/K500_MIC_CROSSOVER_TYPE_CAPTURE.md`
9. `docs/K500_SYSTEM_CONTROLS_CAPTURE_MAP.md`
10. `docs/K500_OUTPUT_DELAY_CAPTURE_MAP.md`
11. `docs/HARDWARE_ACCEPTANCE_CHECKLIST.md`
12. `CONTRIBUTING.md`

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

## UI interaction-state invariant

Qt/QML GUI objects remain on the GUI thread. Do **not** create a background
"GUI worker" that mutates QML objects. The dedicated worker boundary is for
native transport/I/O.

For every interactive control surface:

- model/delegate identity must remain stable for the full pointer/keyboard gesture;
- a live value change must update the existing delegate in place, never rebuild
  the model that owns the active `MouseArea`;
- gesture-local presentation owns the value until release/cancel;
- StudioEngine owns semantic live UI intent; QML is presentation/gesture only;
- authoritative connect/Recall/reconciliation may replace values, but must not
  masquerade as an ordinary live edit;
- changing one semantic control must never reset unrelated sibling controls;
- dynamic JS arrays that embed live engine/device values must not be used as a
  `Repeater`/`ListView` model when those values can change during interaction.

This extends the existing crash-proof fixed-model rule beyond EQ pages to all
interactive racks. A visually correct refactor that recreates a delegate during
drag is a regression.
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
- visible official preset identity must come from the exact valid bytes actually selected (cache or bundled), never merely from a static catalog label;
- a checksum-valid cache/catalog identity mismatch must surface the exact embedded name from the selected bytes; do not label either side stale or promise refresh unless provenance proves that direction;
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
- FBE/FBX is proven as `.k500[0x0023] <-> activeMemory[0x001B]`, native values `0..4`; `.k500[0x001B]` and `.k500[0x001C]` are Mic HP/LP type bytes and must never be used as FBE/FBX;
- checksum is recomputed last;
- every mutation ends with a changed-byte audit.

## Current sonic reference

The current **gold music reference** remains:

```text
resources/presets/03_KAR_DANGDUT.k500
internal/hardware name: KAR DANGDUT
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

### Current official preset authority

Mode 01 is now `KONSER NYANYI`, evolved from the exact native `CONCERT HIFI V4` donor through iterative K500 listening. The native donor remains rollback/provenance authority; the current repository file is distribution authority.

The finalized official library is intentionally differentiated by use case:

- 01 `KONSER NYANYI` — fresh, large, forgiving concert karaoke;
- 02 `MC HOST RADIO` — dry/controlled broadcast utility; intentionally not Air-Focus styled;
- 03 `KAR DANGDUT` — refreshed vocal FX over the proven Hi-Fi Core V3 Music/Sub foundation;
- 04 `POP ROCK BALLAD` — pop/slow-rock with casual-user ambience plus stronger singer control;
- 05 `POP KENANGAN V2` — warm romantic 70s/80s slow-pop;
- 06 `SHOLAWAT SYAHDU` — fresh, soft, slow-tempo sholawat ambience;
- 07 `JAZZ LOUNGE` — intimate/classy lounge vocal with restrained echo;
- 08 `BLUES CLUB` — warm vintage blues with a single slap-style repeat;
- 09 `ACOUSTIC NATURAL` — natural/organic vocal with unobtrusive ambience;
- 10 `REGGAE DUB` — bass/groove-led preset with intentional rhythmic echo.

Do not homogenize the library by transplanting one mode's full EQ/FX architecture into another. Preserve each donor's genre-specific Music/Mic/Sub foundation unless hardware evidence justifies a targeted change.

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
- keep Bluetooth claims separate from USB acceptance until Bluetooth is independently tested;
- for v1.1+ updater releases, never rebuild after RC acceptance: promotion must verify and reuse the exact accepted RC binary hashes recorded in `docs/V1_1_UPDATER_RC_ACCEPTANCE.md`.

A new AI thread should prefer repository evidence over remembered chat context. If a fact conflicts, the current stable code, golden vectors, exact donor files, and evidence-backed documentation are authoritative.


## CI architecture invariant

The repository uses **tests as regression units, not GitHub workflows as regression units**.

- Normal pull requests and `main` use the single `K500 CI` workflow.
- New bug/feature coverage belongs in an executable self-test or `tools/ci/contracts.py`.
- Do not create one workflow per milestone, protocol field, UI control, or bug.
- Compile the Windows tree once, then run all hardware-free self-tests against that exact build.
- PR exact-head concurrency cancels obsolete runs.
- Heavy ASan/fuzz/soak is manual hardening, not default PR CI.
- RC/stable workflows remain manual and immutable.
- The active workflow budget is intentionally <= 4 files under `.github/workflows`.
- Any proposal to add another active workflow must first prove that it cannot be expressed as a test/job in `windows-build.yml`.

Read `docs/CI_ARCHITECTURE.md` before changing CI.
