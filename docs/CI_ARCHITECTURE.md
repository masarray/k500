# CI Architecture

Status: consolidated CI contract for SonKuPik K500.

## Goals

The repository uses **tests as regression units, not workflows as regression
units**. A new bug fix should normally add or extend an executable self-test or
`tools/ci/contracts.py`; it should not create another GitHub Actions workflow.

The normal development budget is intentionally small:

1. **K500 CI / Fast contracts** — Ubuntu, seconds-to-minutes.
2. **K500 CI / Windows build + regression** — one Qt/MSVC build, then every
   hardware-free native regression executes from that same build.
3. **K500 CI / Installer smoke** — only when packaging/updater paths change;
   validates the standalone helper, branding, Inno compilation and a per-user
   install/uninstall smoke without taxing unrelated DSP/UI work.
4. **P4 Sanitizer Fuzz** — manual hardening only.
5. **Windows Updater Release Candidate** — manual immutable RC packaging.
6. **Windows Updater Stable Promotion** — manual byte-identical promotion.

Only **K500 CI** may auto-run on pull requests and `main`.

## Why this is safer than many workflows

The old layout accumulated milestone workflows P0/P1/P2/P3/P4/P5 plus
feature-specific guards. Many repeated checkout, Qt installation, CMake
configuration and compilation for the same commit. A single PR could fan out
into 30+ runs, making exact-head feedback arrive hours late.

The consolidated layout keeps the useful evidence while removing duplicated
orchestration:

- Qt/MSVC is installed once per normal Windows CI run.
- The tree is compiled once.
- Protocol, canonical state, scheduler, transport, RAII, preset codec,
  persistence, batch library, donation and updater self-tests run against the
  exact same binaries.
- QML lint runs from the same configured tree.
- Static invariants that cannot be expressed as runtime tests live in one
  cross-platform contract suite.
- Documentation-only PRs run fast contracts and may skip the Windows build.
- Every `main` commit always receives the Windows job because release
  qualification is exact-SHA.
- PR concurrency cancels obsolete heads automatically.
- Installer smoke is path-aware and does not run for ordinary DSP/UI changes.

## Gate hierarchy

### L0 — Fast contracts

`python tools/ci/contracts.py`

Covers immutable/static concerns such as:

- exact official preset hashes, sizes, checksums and embedded names;
- protocol/readback offsets that are backed by physical captures;
- Adj Manner ownership/readback and Dance fail-closed boundary;
- output delay and crossover evidence boundaries;
- stable QML delegate ownership and front-panel VR affordance;
- updater/installer provenance tokens;
- QRIS/support integration;
- official preset sync safety;
- CI topology itself.

A brittle marker that is not an actual invariant should not be added merely
because an old workflow checked it.

### L1 — Windows build + regression

The full Release tree is compiled once with the same Qt line used for public
release qualification (Qt 6.10.2). `tools/ci/windows_test_suite.ps1` then runs
all hardware-free self-test executables, QML lint, updater helper self-test and
deployed application runtime smoke tests.

### L2 — Hardening

AddressSanitizer/fuzz/soak is intentionally manual. It is appropriate before a
release or after parser/transport/codec work, not on every UI/documentation
commit.

### L3 — Release

RC and stable workflows remain manual and immutable.

`windows-updater-rc.yml` accepts a commit only when that exact `main` SHA has
a successful **K500 CI** workflow. The RC job then performs its own installer
build/install/uninstall/provenance qualification.

Stable promotion never rebuilds accepted application bytes.

## Rules for future work

- Do **not** add a workflow for a bug, phase, screen, protocol field or UI
  control.
- Add behavior coverage to a self-test executable whenever possible.
- Add a rule to `contracts.py` only for a true static invariant.
- Keep PR/main orchestration in `windows-build.yml`.
- Heavy diagnostics are manual or narrowly scheduled, never duplicated on
  `push` and `pull_request`.
- Feature branches do not run push CI; the PR exact head is the only
  development authority.
- `main` always runs exact-head CI before it can be used as an RC source.
- Release workflows may package only an exact successful `main` CI SHA.
