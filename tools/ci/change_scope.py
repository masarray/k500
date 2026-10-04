#!/usr/bin/env python3
"""Compute path-aware K500 CI scope without spawning extra workflows."""
from __future__ import annotations

import os
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
    force_build = event in {"push", "workflow_dispatch"} or "<unknown-diff>" in paths
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
    installer = any(
        p in installer_names
        or any(p.startswith(prefix) for prefix in installer_prefixes)
        for p in paths
    )

    output = Path(os.environ["GITHUB_OUTPUT"])
    with output.open("a", encoding="utf-8") as fh:
        fh.write(f"build_required={'true' if build else 'false'}\n")
        fh.write(f"installer_required={'true' if installer else 'false'}\n")
        fh.write(f"changed_count={len(paths)}\n")

    print(f"build_required={build}; installer_required={installer}; changed={len(paths)}")
    for path in paths[:80]:
        print(f"  {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
