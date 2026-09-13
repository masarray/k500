# Optimization Completion / Final Physical Gate

The planned optimization and hardening roadmap is complete through **P5**:

```text
P0 runtime telemetry              COMPLETE
P1 native QSG PEQ rendering       COMPLETE
P2 async transport worker         SOFTWARE-COMPLETE / PHYSICAL GATE
P3 deterministic shutdown + RAII  SOFTWARE-COMPLETE
P4 ASan fuzz + lifecycle stress   SOFTWARE-COMPLETE
P5 QML object-load optimization   SOFTWARE-COMPLETE
```

No P6 optimization phase is planned.

The remaining work is **qualification**, not additional architecture change:

1. Build the exact final qualification commit.
2. Run the clean physical performance session on Windows x64 + K500 USB HID.
3. Run the separate unplug/failure/recovery session.
4. Attach the generated JSON evidence.
5. If physical acceptance passes, land P2 -> P3 -> P4 -> P5 -> final qualification tooling.
6. Run the exact-head release regression/package pass from `main`.

The authoritative physical procedure and performance thresholds are in `docs/FINAL_DEVICE_PERFORMANCE_ACCEPTANCE.md`.
