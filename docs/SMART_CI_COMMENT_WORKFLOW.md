# K500 Smart CI — owner-comment, pinned SHA, minimal runner usage

**Applies after the workflow governance PR merges to main.**
This supersedes older CI trigger instructions in AGENTS.md. No new Actions
workflow file is added; the existing CI and release-comment hub are reused.

## Default: small and fast
- On PR updates: one offline Fast Contracts job (Ubuntu), no Qt/installer.
- On main commits: one offline Fast Contracts job, no native builds.
- Native Windows, Inno installer, Linux/macOS packaging: *only* after a
  repository-owner comment or explicitly authorized manual dispatch.
- No automatic stable release from accepting a documentation file: the
  existing owner-comment stable release path enforces accepted RC evidence.
- The current v1.1.5 stable and v1.1.6-rc.1 prerelease are immutable.

## One-line owner comments

Post a comment on an open, same-repo PR targeting main, or on issue #241
for a current-main check.

| Comment | What runs |
| --- | --- |
| `/k500 ci fast` | Offline contracts only |
| `/k500 ci smart` | Minimal targeted suites according to actual file diff |
| `/k500 ci windows` | One Windows Qt build + regression |
| `/k500 ci installer` | Inno updater/rollback lifecycle smoke |
| `/k500 ci platform` | Linux and macOS package QA |
| `/k500 ci full` | All suites (reserved for major source or release work) |

Remove spaces around the command and backticks before posting. Comments
must contain *only* the command; additional prose is rejected.

The previously released owner commands on issue #241 still work independently:
`/k500 release rc v1.1.6-rc.N` and
`/k500 release stable v1.1.6`.
The stable command requires owner physical acceptance, exact RC SHA-256 and
source commit verification; it never recompiles the accepted installer.

## Guardrails
1. Only GitHub comments created by the repository owner may request native CI.
   Non-owner or fork PRs, closed PRs, wrong target base/issue, injected shell
   commands and altered PR SHA/base are rejected before a build starts.
2. The comment hub dispatches existing K500 CI on trusted main, passing the
   precise source SHA, base SHA, requested test scope and optional PR number.
   The CI job checks out that SHA and re-verifies it against the live PR/main.
   A moved branch is never silently substituted for the requested commit.
3. A successful or active run matching mode+SHA+base is reused; repeated
   identical comments do not launch additional native runner jobs.
4. Automatic CI *always* computes only the fast scope. Explicit smart uses
   changed-path classification; missing Git diff information causes a
   fail-safe full suite rather than skipping required checks.
5. Windows, installer and platform jobs are conditional parts of a single
   consolidated workflow; no extra feature-specific workflows are created.
6. Runner results do not replace testing with a real K500 USB HID device.
   Issue #241 remains the owner field-acceptance gate.

## Team/AI workflow
- Make focused commits and use the cheap automatic PR Fast Contracts as a
  continuous safety net. No Windows/Inno/macOS/Linux builds per commit.
- Before merge, comment `/k500 ci smart` **once**. On failure, repair and
  comment again at the new SHA. Use explicit windows/installer/platform for
  a focused regression; full only when cross-platform release risk warrants it.
- Merge the reconciled PR. Main receives only a lightweight validation.
- Use the owner-comment RC workflow when a new candidate is needed. RC itself
  performs independent Windows build, installer install and deep updater QA.
