# P3 — Authoritative State Reconciliation

## Purpose

P1 established session-bound ConfirmedState, DesiredState, and command envelopes. P2 made delivery deterministic, while transport acceptance deliberately remained distinct from hardware confirmation.

The first P3 implementation automatically paused LIVE and performed a complete 939-byte CMD 0x40 readback after every quiet burst of edits. Physical-device validation on 2026-09-18 proved that policy was too disruptive for a real-time DSP editor: every settled control change visibly entered SYNC and normal edits could be ignored while LIVE was disabled.

P3.1 keeps the canonical reconciliation machinery, but removes automatic full-memory reconciliation from the ordinary live-edit path.

## Normal runtime path (P3.1)

```text
user edit
 -> DesiredState / CommandPlan
 -> deterministic scheduler
 -> asynchronous native transport
 -> remain ONLINE/LIVE
 -> next edit remains immediately admissible
```

Transport acceptance still does **not** become hardware confirmation. DesiredState may remain unresolved until the next authoritative session-bound refresh.

## Authoritative refresh path

A complete 939-byte snapshot remains authoritative for:

- initial CONNECT;
- reconnect;
- Recall/resync;
- error recovery;
- explicit qualification/verification flows.

The reusable reconciliation API still performs:

```text
authoritative 939-byte readback
 -> reconcileSnapshot
 -> ConfirmedState rebuild
 -> matching DesiredState clears
 -> mismatching DesiredState remains explicit divergence
```

## Safety rules

1. Reconciliation requires the current active device session and an already-adopted initial snapshot.
2. A reconciliation snapshot must be exactly 0x03AB (939) bytes.
3. Reconciliation is rejected while canonical commands are still in flight.
4. Transport acceptance alone never clears DesiredState.
5. Initial connect and Recall use authoritative hardware truth before LIVE.
6. Ordinary knob/fader edits must not force ONLINE/LIVE to bounce through SYNC.
7. Normal live edits must not be dropped merely because background verification is running.
8. Range/domain validation must follow docs/K500_NATIVE_VALUE_RANGES.md.

## Host timing policy

The retained 700 ms / 120 ms reconciliation timers belong to the explicit reconciliation machinery. They are no longer automatically armed by every accepted live command.

## Diagnostics

Support Report contains canonical snapshot generation, desired count, in-flight count, divergence count, scheduler telemetry, and reconciliation activity. Raw active-memory bytes remain excluded.

## Qualification

Hardware-free canonical tests continue to prove:

- reconciliation cannot cross an in-flight command;
- transport success alone leaves DesiredState unresolved;
- a new authoritative snapshot preserves unresolved intent;
- matching hardware truth converges DesiredState;
- mismatching hardware truth remains explicit divergence;
- malformed snapshots fail closed;
- reconnect invalidates prior-session work.

The P3 workflow additionally guards that the ordinary live-write dispatch path does not auto-arm full reconciliation and that the native FX range contract remains aligned across the runtime layers.
