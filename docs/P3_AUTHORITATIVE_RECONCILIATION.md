# P3 — Authoritative State Reconciliation

## Purpose

P1 established session-bound `ConfirmedState`, `DesiredState`, and command envelopes. P2 made delivery deterministic, but transport acceptance still deliberately did not prove that K500 hardware applied a live edit. P3 closes that gap without inventing a new protocol command.

The milestone reuses the already-proven full 939-byte CMD `0x40` active-memory readback as the authoritative verification barrier after a quiet burst of canonical live writes.

## Runtime path

```text
user edit
 -> P1 DesiredState / CommandPlan
 -> P2 deterministic scheduler
 -> asynchronous native transport accepted
 -> quiet debounce
 -> LIVE OFF
 -> proven 939-byte active-memory readback
 -> P3 reconcileSnapshot
 -> semantic ConfirmedState rebuild
 -> matching DesiredState clears
 -> mismatching DesiredState remains explicit divergence
 -> StudioEngine hydrates hardware truth
 -> LIVE ON
```

Transport acceptance remains distinct from hardware confirmation.

## Safety rules

1. Reconciliation requires the current active device session and an already-adopted initial snapshot.
2. A reconciliation snapshot must be exactly `0x03AB` (939) bytes.
3. Reconciliation is rejected while canonical commands are still in flight.
4. The scheduler queue must be empty before DeviceManager pauses LIVE and starts readback.
5. The new raw snapshot and SHA-256 identity replace the previous hardware image.
6. Existing `ConfirmedState` is cleared and rebuilt from the new image.
7. Unresolved `DesiredState` is preserved across the barrier.
8. `confirm(path, value)` clears DesiredState only when hardware truth equals the intended value.
9. A mismatching confirmed value leaves that path as explicit divergence rather than pretending success.
10. Initial connect and explicit Recall continue to use destructive `adoptSnapshot`; they do not inherit stale pre-recall intent.

## Host timing policy

The 700 ms quiet delay and 120 ms barrier retry are host-side scheduling policy, not reverse-engineered K500 constants. They exist only to avoid reading through an active burst. Device protocol bytes and the proven readback sequence are unchanged.

## Diagnostics

Support Report adds a bounded `canonicalState` summary containing snapshot generation, desired count, in-flight count, divergence count, and whether reconciliation is currently active. Raw active-memory bytes remain excluded.

## Qualification

`P3AuthoritativeReconciliationSelfTestMain.cpp` proves:

- reconciliation cannot cross an in-flight command;
- transport success alone leaves DesiredState unresolved;
- a new authoritative snapshot preserves unresolved intent;
- old ConfirmedState does not leak across generations;
- matching hardware truth converges DesiredState;
- mismatching hardware truth remains explicit divergence;
- malformed snapshots fail closed.

The dedicated P3 workflow also guards the DeviceManager readback barrier and builds the full Windows application so the integration remains compile-qualified.
