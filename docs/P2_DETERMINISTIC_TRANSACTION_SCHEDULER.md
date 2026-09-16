# P2 — Deterministic Transaction Scheduler

## Purpose

P2 inserts a bounded host-side scheduler between P1 `CommandPlan` envelopes and the asynchronous HID worker. It does **not** infer new K500 protocol behavior, mutate native frame bytes, or promote a successful host write to `ConfirmedState`.

The scheduler makes rapid UI edits deterministic under load: commands from an old device session are rejected, repeated edits to the same semantic/native key collapse to the latest queued value, queue growth is bounded, and native live-write families retain controlled host pacing.

## Authority boundaries

The P1 truth model remains unchanged:

`RawSnapshot -> ConfirmedState -> DesiredState -> CommandPlan -> P2 queue -> K500WinIo`

A transaction leaving the P2 queue means only that the host is handing it to transport. `K500Controller::markCommandDispatched()` is called at that release point, not when Controller first builds or flushes the command. Transport acceptance remains distinct from hardware confirmation, and `DesiredState` is not promoted to confirmed truth without a later authoritative device readback.

## Integrated transport path

The implemented live path is:

`Controller commandReady -> K500DeviceManager::sendPlannedCommand -> K500TransactionScheduler -> dispatchScheduledCommands -> K500WinIo`

Controller still performs producer-side batching/latest-value debounce before `commandReady`. P2 is the final transport scheduler: it owns the deterministic queue, session rejection, bounded back-pressure, queue-age expiry, and minimum dispatch spacing immediately before the asynchronous I/O worker. The producer timers therefore do not mark a command as dispatched and cannot bypass the P2 queue.

When LIVE ends or a device session is reset, queued scheduler work is cancelled before the scheduler session is ended. Coalesced, evicted, expired, and explicitly cancelled envelopes are surfaced through `takeDropped()` so the owner can release/reject any still-current transport state. This is safe with P1 latest-wins behavior: an already-superseded token is ignored by `CanonicalState`, while a still-current dropped token is completed as a rejected transport attempt.

## Host scheduling policy

These values are **host policy**, not newly reverse-engineered device constants:

- maximum queued transactions: 64;
- maximum queue age: 1500 ms;
- EQ family minimum spacing: 45 ms;
- complete-block family minimum spacing: 55 ms;
- immediate family spacing: 0 ms.

The 45/55 ms values preserve the established live-write timing policy. P2 enforces those minimums at the final transport-release boundary.

## Deterministic rules

1. A transaction must carry the current non-zero `sessionEpoch`, token, native frame, semantic path, and coalescing key.
2. A transaction from another session is rejected before it can reach transport.
3. A queued transaction with the same coalescing key is replaced by the latest transaction. The superseded envelope is surfaced for owner cleanup; the replacement gets a new queue sequence and queue-age origin.
4. Ready selection is priority first, FIFO sequence second.
5. Family pacing is evaluated independently for Immediate, EQ, and Block traffic.
6. Queue capacity is hard-bounded. When full, an incoming transaction may evict the oldest transaction from the lowest priority not greater than the incoming priority. Lower-priority input cannot displace higher-priority work.
7. A transaction older than the configured maximum queue age expires rather than being sent late.
8. Beginning a new session clears queued work and pacing history. Ending LIVE cancels queued work before transport state is reset.
9. `markDispatched()` occurs only after the scheduler selects a ready transaction and immediately before `writeFrame()` hands it to the asynchronous worker.
10. A successful transport enqueue/write is not hardware confirmation.

## Telemetry

The scheduler exposes deterministic counters for accepted/enqueued intents, same-key coalescing, capacity eviction, back-pressure rejection, stale/invalid rejection, expiry, dispatch, current queue depth, and peak queue depth. `K500DeviceManager::supportReportJson()` includes this scheduler telemetry while continuing to omit active-memory and preset payload bytes.

## Qualification

`P2TransactionSchedulerSelfTestMain.cpp` is hardware-free and covers stale-session rejection, invalid envelope rejection, latest-wins coalescing, superseded-token recovery, priority ordering, FIFO tie-breaking, 45 ms EQ pacing, 55 ms block pacing, bounded back-pressure behavior, eviction/cancellation/expiry recovery, telemetry, and reconnect queue invalidation.

The dedicated P2 workflow additionally guards the real Controller-to-scheduler-to-DeviceManager integration: Controller must not pre-mark a command as dispatched, `K500DeviceManager` must enqueue every canonical live command into P2, and the actual dispatch barrier must remain immediately on the scheduler release path. Windows builds also compile the scheduler with MSVC/Qt6 so host type-width regressions such as `qsizetype` versus `int` are caught in CI.
