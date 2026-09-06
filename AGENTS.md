# K500 AI / Agent Entry Point

This file exists so a new ChatGPT/Codex/AI thread can work on K500 presets without relying on hidden conversation history.

## If the task involves `.k500` preset analysis, sonic tuning, simulation, graphs, or creating a new preset

**Read these files first, in this order:**

1. `docs/K500_PROVEN_SONIC_BASELINE.md` — **current hardware-proven sonic reference, Hi-Fi Music Core V3, cross-mode transplant strategy, lock policy, and thread handoff state. Read this first so a new thread does not restart sonic research from zero.**
2. `docs/K500_AI_PRESET_ENGINEERING_PLAYBOOK.md` — workflow, sonic model, research method, plotting, iteration strategy.
3. `docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md` — authoritative binary map, offsets, signal-flow facts, round-trip hazards.
4. `docs/K500_PRESET_NAME_LIMIT.md` — hard hardware name limit.
5. `tools/k500_preset_lab.py` — executable analysis / simulation / donor-based patch tool.

Do not improvise a `.k500` format from memory when the repository provides the proven map.

## Current sonic baseline that must not be forgotten

The current **gold music reference** is the repository's Mode 03 preset:

```text
resources/presets/03_DANGDUT_SUPREME.k500
```

Its current sonic state is documented in `docs/K500_PROVEN_SONIC_BASELINE.md` and was approved through real K500 listening after the following progression:

```text
Core V1: bass became enjoyable; mid still ordinary
Core V2: mid refinement -> user reported the mid became enjoyable
Core V3: small very-bottom extension -> user reported the overall result was enjoyable
```

Therefore a new thread MUST NOT casually rebuild the music tuning from a generic karaoke curve.

Product-level rule:

> **All modes should preserve their own vocal identity while giving the user a consistent premium Hi-Fi music-enhancement benefit. Enhancement means maximum listening enjoyment, not maximum loudness.**

When the user reports that a region is already good, treat that as a **LOCK** for the next revision unless the user explicitly asks to revisit it.

## Non-negotiable preset rules

- `.k500` size is exactly **1144 bytes / `0x478`**.
- A valid file satisfies `sum(all bytes) % 256 == 0`.
- Checksum byte is `0x475`; recompute it **last**.
- Hardware-visible preset name is **maximum 16 characters**.
- Start from a known-good donor preset. **Do not build a preset from a zero-filled buffer.**
- Patch only proven offsets. Preserve all unknown/reserved bytes exactly.
- Preserve raw PEQ type aliases when a band type is not intentionally changed.
- Preserve `mainAlt`, `surroundAlt`, `centerAlt`, `subAlt` unless a controlled hardware experiment explicitly targets them.
- No-op must be byte-identical.
- Every write must finish with a byte-diff audit; unexpected changed offsets are a failure.
- Real K500 hardware listening is authoritative. Simulation is a comparative engineering tool, not proof of exact device DSP coefficients.

## Sonic architecture to keep in mind

```text
Music Input -> Music PEQ/XO -------------------------> output routing -> output PEQ/XO

Mic A/B -> mic gain/dynamics -> Mic PEQ/XO -> dry --+-> Main / Center / Surround / Sub
                                                     |
                                                     +-> Reverb PEQ/XO -> wet returns --+
                                                     +-> Echo PEQ/XO  -> wet returns ----+-> output PEQ/XO
```

Important consequence: dry Mic, Reverb and Echo are **parallel contributions** before each output path; do not model wet branches as if Reverb and Echo were serial inserts.

Recommended conceptual output roles:

- **Main** = front image, tonal body, music punch, primary vocal.
- **Center** = lead/vocal anchor.
- **Surround** = width, air, ambience, decorrelation; not a second Main.
- **Sub** = deep music foundation; normally keep Mic/Reverb/Echo out unless deliberately proven useful.

## Hi-Fi music strategy

The current design strategy is:

```text
mode identity = vocal architecture + FX/spatial vocal behavior
music quality = shared premium Hi-Fi target
```

For music-only improvement requests:

- preserve vocal EQ/routing/dynamics unless explicitly requested;
- use the proven Mode 03 Hi-Fi Core V3 as the tonal reference;
- match music routing using output-aware effective energy rather than copying raw route values blindly;
- protect 2.5-4.5 kHz from excessive brightness;
- prefer controlled 5-7 kHz detail and 10-14 kHz air for premium polish;
- use the dedicated Sub path for additional 53-65 Hz very-bottom satisfaction;
- use the ~158 Hz region for roundness/punch rather than solving everything with deep-sub gain;
- do not change a region that the user has already approved.

For route compensation, the comparative proxy is:

```text
Effective route amplitude ~= route * 10^(output_dB / 20)
```

If an output gain is changed for music scale, compensate Mic/Reverb/Echo source routes as needed so vocal ambience remains stable. Mode 02 Broadcast in the proven sonic baseline is the reference example.

## Preferred AI workflow

```text
1. read the proven sonic baseline and identify what is already GOLD / LOCKED
2. define listening goal / failure mode
3. choose the closest proven donor
4. inspect donor + reference presets
5. form a narrow sonic hypothesis
6. simulate cumulative paths and guardrail regions
7. patch only intended fields
8. recompute checksum
9. byte-diff audit
10. generate graphs / CSV / JSON
11. test on real K500 hardware
12. use listening feedback to choose the next *small* change
```

For iterative tuning, prefer one causal experiment per revision over broad multi-parameter rewrites.

## Standard tool commands

```bash
python tools/k500_preset_lab.py validate preset.k500
python tools/k500_preset_lab.py inspect preset.k500 --json audit.json
python tools/k500_preset_lab.py plot preset.k500 --out-dir analysis/
python tools/k500_preset_lab.py compare old.k500 new.k500 --out-dir comparison/
python tools/k500_preset_lab.py patch donor.k500 patch.json candidate.k500
```

Plot/compare require:

```bash
python -m pip install -r tools/requirements-preset-lab.txt
```

## What a preset-research handoff should contain

When handing work to another thread, include or regenerate:

- exact donor file / version;
- exact output file / version;
- intended listening goal;
- current GOLD reference and all LOCKED regions;
- changed semantic parameters;
- changed byte offsets;
- checksum/size/name validation;
- Mic, Music, Main and cumulative path graphs;
- guardrail metrics (especially body, mud, `i`-ring, detail, air, sub and punch);
- real hardware listening feedback;
- what is modeled/transplanted but still awaiting hardware validation;
- what must not be changed in the next iteration.

If a later thread has only the repository, **this file plus `docs/K500_PROVEN_SONIC_BASELINE.md` are the required restart point** and must be sufficient to continue preset engineering without restarting from a generic karaoke preset.
