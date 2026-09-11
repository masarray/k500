# P0 Runtime Telemetry

This document defines the first performance-qualification layer for SonKuPik K500.
It is intentionally hardware-free and does not change any K500 protocol, preset,
readback, LIVE, or transport behavior.

## Goal

Make runtime optimization measurable and regression-resistant before deeper UI or
transport changes are attempted.

The P0 harness records:

- measured stress duration for two equivalent phases;
- process working set;
- process private bytes;
- peak working set;
- Windows process handle count;
- Windows process thread count;
- steady-state resource growth between equivalent post-warmup phases.

The output is one machine-readable JSON object using schema:

```text
sonkupik-k500-runtime-telemetry-v1
```

## Why steady-state comparison is used

Comparing process startup directly with the end of a stress run is not a useful
leak test for Qt applications. Qt, the C++ runtime, and the Windows heap may keep
allocator arenas or caches after first use.

The harness therefore:

1. creates a real `StudioEngine` and warms its fixed EQ models;
2. runs lifecycle churn before the baseline is established;
3. captures a warm snapshot;
4. runs measured phase A and captures another snapshot;
5. repeats the same work as measured phase B;
6. gates only the resource growth from phase A to phase B.

This makes a one-time allocator/cache warm-up much less likely to be misreported
as a memory leak.

## Current CI guardrails

The Windows qualification fails when the post-warmup second phase exceeds any of
these intentionally conservative bounds:

- private-memory growth: more than 8 MiB;
- handle growth: more than 2 handles;
- thread growth: more than 1 thread.

Working-set growth and timing are recorded but are not hard-gated yet because
both are sensitive to Windows scheduling and page residency on shared CI hosts.
They are baseline evidence for later optimization PRs.

These thresholds are regression tripwires, not product performance claims. They
must not be tightened or relaxed solely to make CI green; threshold changes need
an explanation backed by repeated measurements.

## Workload

The harness exercises all persistent EQ model families used by the application:

- Music;
- Mic A;
- Mic B;
- Reverb;
- Echo;
- Main;
- Surround;
- Center;
- Sub.

It repeatedly performs band frequency/gain/Q edits, occasional filter-type and
crossover edits, top-level tone/key edits, and complete `StudioEngine` lifecycle
creation/destruction. A checksum is emitted so the workload remains observable
and cannot be reduced to a no-op accidentally.

## Running locally

After configuring a normal Qt/MSVC build:

```powershell
cmake --build build --config Release --target k500_p0_perf_selftest
build\k500_p0_perf_selftest.exe
```

The dedicated GitHub Actions workflow stores the JSON result as the
`K500-P0-Runtime-Telemetry` artifact.

## Scope boundary

P0 is an observability and qualification layer. It deliberately does **not**:

- replace fixed EQ page/model lifetimes;
- modify device bytes or protocol ordering;
- change LIVE gating;
- change preset transactions;
- move Windows I/O to another thread;
- claim absolute crash-proof or leak-proof behavior from one CI run.

Future performance work should reuse this baseline and add narrower measurements
for UI frame latency and transport/shutdown behavior without weakening existing
architecture invariants.
