# K500 AI / Agent Entry Point

This file exists so a new ChatGPT/Codex/AI thread can work on K500 presets without relying on hidden conversation history.

## If the task involves `.k500` preset analysis, sonic tuning, simulation, graphs, or creating a new preset

**Read these files first, in this order:**

1. `docs/K500_AI_PRESET_ENGINEERING_PLAYBOOK.md` — workflow, sonic model, research method, plotting, iteration strategy.
2. `docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md` — authoritative binary map, offsets, signal-flow facts, round-trip hazards.
3. `docs/K500_PRESET_NAME_LIMIT.md` — hard hardware name limit.
4. `tools/k500_preset_lab.py` — executable analysis / simulation / donor-based patch tool.

Do not improvise a `.k500` format from memory when the repository provides the proven map.

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

## Preferred AI workflow

```text
1. define listening goal / failure mode
2. choose the closest proven donor
3. inspect donor + reference presets
4. form a narrow sonic hypothesis
5. simulate cumulative paths and guardrail regions
6. patch only intended fields
7. recompute checksum
8. byte-diff audit
9. generate graphs / CSV / JSON
10. test on real K500 hardware
11. use listening feedback to choose the next *small* change
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
- changed semantic parameters;
- changed byte offsets;
- checksum/size/name validation;
- Mic, Music, Main and cumulative path graphs;
- guardrail metrics (especially body, mud, `i`-ring, detail, air, sub and punch);
- real hardware listening feedback;
- what is locked and must not be changed in the next iteration.

If a later thread has only the repository, this file plus the linked playbook must be sufficient to restart the preset-engineering workflow safely.