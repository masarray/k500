# Final K500 Device + Performance Acceptance

This is the **last qualification step after P0-P5**. It does not introduce a P6 optimization phase. The optimization stack is software-complete; this runbook proves the async USB/shutdown/QML changes on a physical K500 before landing the stack to `main`.

The qualified stable target remains **Windows 10/11 x64 + USB HID**. Bluetooth SPP is experimental and is not part of this final gate.

## What the qualification build measures

Launch the application with:

```text
--device-perf --device-perf-report=<absolute-json-path>
```

The opt-in monitor is dormant in normal application runs. In qualification mode it records:

- bootstrap-to-event-loop time (core objects -> QML loaded -> event loop starts);
- CONNECT -> full 939-byte hydration -> LIVE timing;
- first `Read 0x0000` -> full 939-byte hydration timing;
- `K500Controller::frameReady` -> DeviceManager TX accepted/logged latency on the GUI thread;
- event-loop lag with a low-overhead 50 ms sampler;
- process private bytes, working set, handles and threads;
- reconnect/disconnect/error counts;
- controller frame/deferred/unsupported-path counts;
- exact build Git commit embedded at configure time.

The JSON is atomically refreshed every five seconds and again on orderly exit.

## Performance targets

These are qualification targets, not claims that every Windows machine will produce identical timings.

| Metric | Final target |
|---|---:|
| Worst CONNECT -> LIVE | <= 5000 ms |
| Worst full 939-byte readback | <= 3000 ms |
| Max Controller -> TX accepted latency | <= 5000 us |
| Event-loop ticks taking >250 ms | 0 during clean performance session |
| Steady-state private-byte growth vs first connected baseline | <= 8 MiB |
| Handle growth vs first connected baseline | <= 2 |
| Thread growth vs first connected baseline | <= 1 |
| Unsupported LIVE paths | 0 |
| Device/error log lines | 0 during clean performance session |

`automatedPerformancePass=true` means the clean performance portion meets all targets above. It **does not** replace the functional/destructive/unplug checklist.

## Build to test

Use the exact runnable artifact produced by the `Final Device Performance Qualification` workflow for the PR/commit under test. Do not mix an EXE from one commit with a report from another commit.

The report itself contains `gitCommit`, so the physical evidence remains tied to the exact binary.

## Session A — clean performance qualification

Run this session without intentional unplug/error injection.

### 1. Warm the final UI before the baseline

1. Close the manufacturer K500 software and any application that could own the HID device.
2. Connect the K500 by USB.
3. Start the qualification build with `tools/run-final-device-acceptance.ps1` and `-Session performance`.
4. **Before the first CONNECT**, visit every section once, including System.
5. Confirm the System page appears normally. This intentionally warms the P5 one-shot lazy workspace before the first-connected resource baseline is captured.
6. Return to a normal editing section.

### 2. Initial connect/readback

1. Select USB HID.
2. Press CONNECT.
3. Confirm the visible sequence reaches ONLINE/LIVE only after the full device state is hydrated.
4. Confirm the K500 is authoritative: current hardware values appear before any edit is made.
5. Do not accept a session where stale editor values are pushed before hydration.

Expected truth path:

```text
heartbeat -> handshake -> CMD 0x40 blocks -> 939-byte hydration -> LIVE ON
```

### 3. Rapid LIVE-edit stress

Perform at least all of the following while connected:

- drag Master Music continuously for ~10 seconds;
- drag Master Mic continuously for ~10 seconds;
- drag one Music/Mic PEQ point in frequency + gain for ~10 seconds;
- change PEQ Q repeatedly with the wheel;
- edit one output PEQ;
- change a verified HPF/LPF value;
- switch rapidly through:

```text
Mic A -> Reverb -> Mic B -> Reverb -> Echo -> Main -> Surround -> Center -> Sub -> System
```

Repeat the section sequence at least 10 times.

Expected behavior:

- controls track the mouse immediately;
- no visible multi-hundred-millisecond freeze;
- device follows the latest settled values;
- no crash, heap corruption, stale EQ page, or unintended write;
- no unsupported-path event.

### 4. Reconnect lifecycle stress

Perform **25 disconnect/reconnect cycles** as the standard final qualification target.

For every cycle:

1. Disconnect from the app.
2. Reconnect USB.
3. Wait for full readback and LIVE.
4. Confirm device truth is re-hydrated.
5. Make one small verified edit and confirm the K500 reacts.

Do not power-cycle between these normal reconnect cycles unless investigating a failure.

### 5. Short connected soak

Keep the app connected for at least 10 minutes while using normal controls and switching sections occasionally.

At the end of the clean session, close the app normally. The launcher prints the summary from `device-performance.json`.

### Clean performance PASS

All of these are required:

- `gates.automatedPerformancePass == true`;
- no crash/hang;
- no unexpected device error;
- no wrong/non-target hardware change;
- visual interaction remains subjectively smooth during rapid edit stress.

If the automated gate fails, preserve the JSON and do not merge the stack until the cause is understood.

## Session B — failure/recovery qualification

Run a **separate** session with `-Session recovery`. Intentional errors make `automatedPerformancePass` unsuitable as the verdict for this session.

Perform these tests:

1. **Unplug while LIVE** during normal idle operation.
   - App must go offline/error without crash.
   - Further LIVE writes must stop.
   - Reconnect must perform full hydration again.

2. **Unplug during rapid edits / pending writes**.
   - No hang or crash.
   - No unsafe success claim.
   - Reconnect must restore device truth.

3. **Unplug during initial readback**.
   - LIVE must never be promoted from a partial readback.
   - Reconnect must restart the full sync.

4. **Close the app while connected and traffic is active**.
   - Window/process must exit cleanly.
   - No stuck process.
   - Relaunch/reconnect must work normally.

5. **Preset transaction smoke** using test/sacrificial destination slots only.
   - Recall: C0 + full 939-byte readback before LIVE resumes.
   - Single Upload/Save: Store sequence completes, then Recall/readback before LIVE.
   - Mass Upload: use disposable/test slots and confirm descending hardware order plus final Slot 01 Recall/readback.
   - If USB is intentionally removed during one transaction, it must fail closed and never report false success.

Back up any device presets that matter before destructive Store/Mass Upload tests.

### Recovery PASS

- no crash;
- no permanent hang;
- no stuck worker/process on close;
- no LIVE promotion from incomplete readback;
- no false-success transaction state;
- reconnect always re-establishes hardware truth.

Expected error/status records in this recovery report are evidence of injected failures, not an automatic rejection.

## Launcher

From PowerShell:

```powershell
.\tools\run-final-device-acceptance.ps1 `
  -Exe ".\package\SONKUPIK-STUDIO-Native-UI.exe" `
  -Session performance `
  -RequirePerformancePass
```

Then run recovery separately:

```powershell
.\tools\run-final-device-acceptance.ps1 `
  -Exe ".\package\SONKUPIK-STUDIO-Native-UI.exe" `
  -Session recovery
```

Each run creates a timestamped folder under `artifacts/` with `device-performance.json`.

## Evidence to attach before landing

Attach both reports to the final qualification PR:

- clean `performance` JSON;
- `recovery` JSON;
- any Support Report captured for an anomaly;
- a short manual statement confirming the functional checks above and the test K500/Windows environment.

Then land the stack in order:

```text
P2 -> P3 -> P4 -> P5 -> final qualification tooling
```

After landing, rerun the exact-head Windows build/runtime/regression matrix and produce the final release package. No additional optimization phase is planned unless this physical evidence exposes a specific defect or measurable bottleneck.
