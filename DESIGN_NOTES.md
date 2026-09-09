# SonKuPik K500 — Native UI Design Notes

## Product character

**Obsidian Console** — quiet, precise, high-density, tactile, and premium.

The interface borrows interaction principles rather than branding or pixel geometry from professional audio tools:

- graph-first equalizer workflow and direct band manipulation;
- selected-channel immediacy and high information density;
- clear fader/encoder hierarchy with unambiguous active state;
- restrained motion and color so the UI feels like one hardware-control system, not a collection of cards.

## Visual rules

- Plus Jakarta Sans is the primary application typeface.
- Controls first; explanation belongs in documentation, not the active workspace.
- PEQ receives the largest visual area where EQ is the primary task.
- Active state earns color; idle state stays calm.
- Avoid decorative glow as a default state.
- Avoid oversized cards, headings, and unnecessary empty padding.
- Numeric readouts remain proportional and visually aligned.
- Disable accidental text-selection behavior in control surfaces.
- Motion communicates state continuity; it does not decorate the UI.
- Prefer Qt Quick scene-graph-friendly geometry and simple opacity/position transitions.

## Interaction rules

- Drag PEQ nodes on X/Y to change frequency/gain.
- Drag knobs vertically; hold Shift for fine control; double-click resets where supported.
- Mouse wheel adjusts knobs/faders; Shift gives fine adjustment.
- Faders track the pointer immediately and use restrained visual smoothing only where it does not delay control feedback.
- Selected controls are clearly highlighted; neighboring inactive controls must not look selected.
- Hardware actions are explicit. Merely selecting a PC preset or Device slot must not silently change processor audio.

## System workspace information architecture

The System workspace represents two different authorities and must keep them visually distinct:

- **Device Mode** — actual K500 slot/readback state.
- **PC Presets** — staged files that may come from SONKUPIK official presets or LOCAL user presets.

Mass Upload is a reviewable mapping workflow:

```text
Unified PC collection
    -> explicit transfer list Slot 01…10
    -> validated batch
    -> permanent hardware transaction
```

The UI may show slots in natural ascending visual order while the backend executes the proven native descending Store order.

## Crash-resistant workspace rule

The v1.0 baseline intentionally keeps incompatible EQ models on stable page lifetimes. Mic, Reverb/Echo, and output sections have different band counts; do **not** reintroduce a single `SectionEqGraph` instance that hot-swaps 10/7/5-band models during navigation.

Reason: a previous hot-swap design allowed Canvas/Repeater/Inspector state to overlap a new model during a section transition and could terminate the deployed Qt application. The stable design uses fixed graph/model ownership and switches visibility/page selection instead of graph identity.

Any future navigation refactor must pass the repeated runtime section stress test before merge.

## Performance rule

Responsiveness is part of the product quality bar. Prefer stable object lifetimes, bounded model updates, and targeted repaints over unnecessary object destruction/recreation or broad binding churn. UI optimization must not weaken device-truth or transaction-safety behavior.
