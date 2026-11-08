#!/usr/bin/env python3
"""Compute path-aware K500 CI scope without spawning extra workflows."""
from __future__ import annotations

import os
import re
import subprocess
from pathlib import Path


def changed_paths() -> list[str]:
    event = os.environ.get("GITHUB_EVENT_NAME", "")
    if event == "workflow_dispatch":
        return []

    base = os.environ.get("CI_BASE_SHA", "").strip()
    head = os.environ.get("GITHUB_SHA", "HEAD").strip() or "HEAD"
    if not base or set(base) == {"0"}:
        return ["<unknown-diff>"]
    try:
        out = subprocess.check_output(
            ["git", "diff", "--name-only", f"{base}...{head}"],
            text=True,
        )
    except subprocess.CalledProcessError:
        return ["<unknown-diff>"]
    return [line.strip() for line in out.splitlines() if line.strip()]


def main() -> int:
    event = os.environ.get("GITHUB_EVENT_NAME", "")
    paths = changed_paths()

    # CI_SELF_VALIDATION_SCOPE_V1 — changing the consolidated CI implementation
    # must exercise the Windows suite it controls. Do not let a tools/ci-only PR
    # validate only the fast Ubuntu contracts.
    build_prefixes = ("src/", "qml/", "resources/", "packaging/", "tests/", "tools/ci/")
    build_names = {"CMakeLists.txt", ".github/workflows/windows-build.yml"}
    # SMART_CI_SCOPE_V2: normal main pushes obey the same file-based rules
    # as PRs. Only an explicit manual diagnostic or missing diff forces build.
    force_build = event == "workflow_dispatch" or "<unknown-diff>" in paths
    build = force_build or any(p in build_names or p.startswith(build_prefixes) for p in paths)

    installer_prefixes = (
        "packaging/windows/",
        "tests/windows/",
        "src/AppUpdateManager",
        "qml/components/UpdateDialog.qml",
        "assets/SonKuPik-k500-logo.png",
    )
    installer_names = {
        "CMakeLists.txt",
        "src/AppVersionInit.cpp",
        "tools/ci/installer_smoke.ps1",
    }
    # Native Mac/Linux jobs run only when their real transport/build inputs change.
    platform_names = {
        # Version-only bumps do not alter native Mac/Linux transport. Do not
        # spin up 2 expensive platform jobs for a Windows GUI patch release.
        "src/k500/K500PosixIo.cpp",
        "src/k500/K500DeviceManager.cpp",
        "src/k500/K500TransactionScheduler.h",
        "src/k500/K500TransactionScheduler.cpp",
        "src/AppUpdateManager.cpp",
        # A packaging-workflow fix must qualify the actual native archives on
        # the next main push. Still only ONE consolidated run, no RC build.
        ".github/workflows/windows-build.yml",
    }
    platform = event == "push" and any(p in platform_names for p in paths)

    installer = any(
        p in installer_names
        or any(p.startswith(prefix) for prefix in installer_prefixes)
        for p in paths
    )

    # SMART_STABLE_PROMOTION_GATE_V1: no extra release workflow is spawned.
    # A single, verified acceptance-file change in main unlocks the existing
    # byte-identical promotion as a final job of K500 CI. Never auto-approve RC.
    gate_path = "docs/V1_1_5_UPDATER_RC_ACCEPTANCE.md"
    gate = Path(gate_path).read_text(encoding="utf-8") if Path(gate_path).is_file() else ""
    def gate_value(key: str) -> str:
        match = re.search(rf"(?m)^{re.escape(key)}=(.*)$", gate)
        return match.group(1).strip() if match else ""

    approval = gate_value("UPDATER_V1_1_5_ACCEPTANCE") == "accepted"
    provenance = all((
        re.fullmatch(r"v1\.1\.5-rc\.[1-9][0-9]*",
                     gate_value("UPDATER_V1_1_5_ACCEPTED_TAG")),
        re.fullmatch(r"[a-fA-F0-9]{40}",
                     gate_value("UPDATER_V1_1_5_ACCEPTED_COMMIT")),
        re.fullmatch(r"[a-fA-F0-9]{64}",
                     gate_value("UPDATER_V1_1_5_USER_SHA256")),
    ))
    promotion = event == "push" and gate_path in paths and approval and provenance

    output = Path(os.environ["GITHUB_OUTPUT"])
    with output.open("a", encoding="utf-8") as fh:
        fh.write(f"build_required={'true' if build else 'false'}\n")
        fh.write(f"installer_required={'true' if installer else 'false'}\n")
        fh.write(f"platform_required={'true' if platform else 'false'}\n")
        fh.write(f"promotion_required={'true' if promotion else 'false'}\n")
        fh.write(f"changed_count={len(paths)}\n")

    print(f"build_required={build}; installer_required={installer}; platform_required={platform}; promotion_required={promotion}; changed={len(paths)}")
    for path in paths[:80]:
        print(f"  {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
