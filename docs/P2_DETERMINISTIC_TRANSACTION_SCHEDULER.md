# P2 — Deterministic Transaction Scheduler

## Purpose

P2 inserts a bounded host-side scheduler between P1 `CommandPlan` envelopes and the asynchronous HID worker. It does **not** infer new K500 protocol behavior, mutate native frame bytes, or promote a successful host write to `ConfirmedState`.

The scheduler exists to make rapid UI edits deterministic under load: commands from an old device session are rejected, repeated edits to the same semantic/native key collapse to the latest queued value, queue growth is bounded, and the two native live-write families keep the pacing already used by the controller.

## Authority boundaries

The P1 truth model remains unchanged:

`RawSnapshot -> ConfirmedState -> DesiredState -> CommandPlan -> P2 queue -> HID worker`

A transaction leaving the P2 queue means only that the host handed it to transport. `DesiredState` remains pending until a later authoritative device readback confirms the semantic value.

## Host scheduling policy

These values are **host policy**, not newly reverse-engineered device constants:

- maximum queued transactions: 64;
- maximum queue age: 1500 ms;
- EQ family minimum spacing: 45 ms;
- complete-block family minimum spacing: 55 ms;
- immediate family spacing: 0 ms.

The 45/55 ms family spacing preserves the live-write pacing already present before P2. P2 centralizes that policy instead of allowing independent timers to create nondeterministic ordering.

## Deterministic rules

1. A transaction must carry the current non-zero `sessionEpoch`, token, native frame, semantic path, and coalescing key.
2. A transaction from another session is rejected before it can reach transport.
3. A queued transaction with the same coalescing key is replaced by the latest transaction. The replacement gets a new queue sequence and queue-age origin.
4. Ready selection is priority first, FIFO sequence second.
5. Family pacing is evaluated independently for Immediate, EQ, and Block traffic.
6. Queue capacity is hard-bounded. When full, an incoming transaction may evict the oldest transaction from the lowest priority not greater than the incoming priority. Lower-priority input cannot displace higher-priority work.
7. A transaction older than the configured maximum queue age expires rather than being sent late.
8. Beginning a new session clears queued work and pacing history.

## Telemetry

The scheduler exposes deterministic counters for accepted/enqueued intents, same-key coalescing, capacity eviction, back-pressure rejection, stale/invalid rejection, expiry, dispatch, current queue depth, and peak queue depth. These counters are intended for the later runtime/device-performance UI and qualification harness; they are not device truth.

## Qualification

`P2TransactionSchedulerSelfTestMain.cpp` is hardware-free and covers stale-session rejection, invalid envelope rejection, latest-wins coalescing, priority ordering, FIFO tie-breaking, 45 ms EQ pacing, 55 ms block pacing, bounded back-pressure behavior, expiry, telemetry, and reconnect queue invalidation.

The next P2 slice integrates this scheduler into `K500DeviceManager`: `commandReady` will enqueue rather than write immediately, a single-shot pump will dispatch only ready work, and `K500Controller::markDispatched()` will move from controller flush time to actual scheduler dispatch time.
