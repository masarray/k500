# P5 Release Candidate Readiness

> Status: **software hardening in progress · physical K500 acceptance pending**.
>
> P5 may produce a public release candidate, but it must not promote hardware-facing rows to `LOCKED ✅` or publish a stable `v1.0` until reproducible physical USB/BT evidence is recorded.

## Purpose

P0–P4.2 completed the native Qt architecture, verified live-command surface, full device hydration, device-side preset transactions, bit-preserving `.k500` handling, controlled file edits, single permanent PC preset upload and deterministic multi-file Mass Upload.

P5 turns that code-complete port into a professional release candidate by hardening diagnostics, packaging, documentation and acceptance evidence.

## Release-candidate invariants

A P5 RC is allowed only when all of these software gates pass:

1. Full Windows/MSVC build succeeds from a clean checkout.
2. P0/P1 architecture and protocol guards pass unchanged.
3. P2 permanent Store/Mass Upload protocol self-test passes.
4. P3 synthetic codec and P3.2 donor corpus pass.
5. P3.4 donor controlled-edit persistence passes.
6. P4.2 donor batch-library test passes.
7. Deployed Qt runtime passes font, protocol/RX and StudioEngine self-tests.
8. Installer and true portable single-EXE both build and pass runtime self-tests.
9. Release artifacts include SHA-256 checksums and a machine-readable release manifest.
10. Public documentation states the physical hardware acceptance status without ambiguity.

## Hardware qualification boundary

The following require a real K500 and cannot be proven by CI:

- USB HID reconnect and long-session LIVE behavior.
- Bluetooth SPP reconnect and long-session LIVE behavior.
- Every P1 live control family with neighboring-byte preservation.
- Equipment Mode Recall followed by authoritative 939-byte resync.
- Use Init Volume device ACK behavior.
- Current-device permanent Save and power-cycle persistence.
- PC `.k500` single-slot Upload and power-cycle persistence.
- Multi-file Mass Upload ordering, chain continuity, other-slot preservation and power-cycle persistence.
- Cable removal / transport loss during LIVE and during a destructive preset transaction.

Until those tests are recorded, these features are **software implemented / hardware acceptance pending**, not `LOCKED ✅`.

## Evidence required for hardware acceptance

Every hardware session must record:

- K500 firmware/version;
- Windows version;
- transport (USB HID or Bluetooth SPP);
- exact SONKUPIK STUDIO commit SHA and release version;
- donor/capture reference where relevant;
- test date and tester;
- protocol/support log or capture reference;
- before/after values for destructive-write tests;
- reconnect result;
- power-cycle result for permanent storage operations.

A checked box without the exact commit and transport evidence is not sufficient.

## P5 deliverables

- Current project README and parity matrix.
- GPL-3.0 project licensing metadata/text.
- Support diagnostics export with bounded protocol history.
- Expanded P4/P5 physical acceptance runbook.
- Release-candidate version metadata.
- Installer + portable single EXE + SHA256SUMS.
- `release-manifest.json` recording version, commit, Qt/runtime target and hardware-acceptance status.
- CI gate that refuses an RC package when any software regression suite fails.

## Stable v1.0 gate

`v1.0` is intentionally out of scope until:

1. all release-candidate software gates remain green;
2. mandatory USB and Bluetooth hardware acceptance is recorded;
3. single Store, PC Upload and Mass Upload survive reconnect + power cycle;
4. no known P0–P4 safety regression remains open;
5. parity matrix hardware-facing rows can be promoted from `IMPLEMENTED ⚠️` to `LOCKED ✅` based on evidence rather than assumption.
