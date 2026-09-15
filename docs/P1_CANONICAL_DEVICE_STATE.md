# P1 — Canonical Device State Architecture

P1 separates hardware truth from UI intent so a reconnect, slider burst, or failed transport write cannot silently corrupt the state model.

## State layers

1. **RawSnapshot** — exact 939-byte (`0x03AB`) Retrieve-All image captured from the current device session. It is copied, detached, SHA-256 fingerprinted, and never edited optimistically.
2. **ConfirmedState** — semantic values decoded from that snapshot. Every value carries an evidence class (`snapshot-captured`, `snapshot-derived`, or `assumed-metadata`).
3. **DesiredState** — current-session user intent. Every edit gets a monotonically increasing revision. Desired values never overwrite ConfirmedState.
4. **InFlightState** — the single latest command plan for each coalescing key. Plans carry session epoch, revision, token, semantic path and immutable frame bytes.

## Session epoch

A new connection attempt starts a new monotonically increasing `sessionEpoch`. Disconnect/error invalidates snapshot, DesiredState and InFlightState immediately. A command planned under an older epoch is stale by definition and must not be dispatched after reconnect.

## Authority barriers

A full Retrieve-All snapshot is required before native writes are eligible. Adopting a new complete snapshot is an authority barrier: it replaces RawSnapshot and clears pre-snapshot intent/queued plans. Transport acceptance only means that Windows accepted the write; it does **not** promote DesiredState to ConfirmedState. Only a later authoritative readback can do that.

## Latest-wins preparation for P2

P1 already gives every semantic command a coalescing key and replaces older queued plans for the same key. P2 will build the deterministic priority scheduler/back-pressure layer on top of these session-safe command envelopes without changing the state semantics.

## Reliability invariants

- no native write without a complete session-bound snapshot;
- raw snapshot bytes are never optimistically mutated;
- unknown/reserved bytes remain seeded from device truth;
- reconnect invalidates every old command token;
- command revisions are monotonic within a session;
- successful transport is not confused with hardware confirmation;
- hardware-free self-tests exercise snapshot size, immutability, latest-wins replacement, reconciliation and stale-session rejection.
