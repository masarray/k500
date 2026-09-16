# P3 Implementation Plan

1. Add canonical `reconcileSnapshot()` that replaces raw hardware truth while preserving unresolved DesiredState.
2. Track authoritative snapshot generation and explicit desired-vs-confirmed divergence.
3. Add a separate Controller reconciliation hydration path so initial connect/Recall remain destructive.
4. After a quiet burst of accepted canonical writes, pause LIVE and reuse the proven full 939-byte active-memory readback.
5. Rehydrate StudioEngine from verified hardware truth, then resume LIVE.
6. Add bounded support-report reconciliation telemetry.
7. Qualify with hardware-free canonical self-test plus Windows build/regression CI.
