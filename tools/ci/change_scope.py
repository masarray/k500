#!/usr/bin/env python3
"""Compute whether a PR needs the Windows compile/runtime job.

Main pushes and manual dispatches always build, because release qualification is
exact-SHA. Pull requests that only change documentation/metadata stay on the
fast contract job.
"""
from __future__ import annotations

import os
import subprocess
from pathlib import Path


def changed_paths() -> list[str]:
    event = os.environ.get("GITHUB_EVENT_NAME", "")
    if event in {"push", "workflow_dispatch"}:
        return ["<force-build>"]

    base = os.environ.get("CI_BASE_SHA", "").strip()
    head = os.environ.get("GITHUB_SHA", "HEAD").strip() or "HEAD"
    if not base or set(base) == {"0"}:
        return ["<force-build>"]
    try:
        out = subprocess.check_output(
            ["git", "diff", "--name-only", f"{base}...{head}"],
            text=True,
        )
    except subprocess.CalledProcessError:
        return ["<force-build>"]
    return [line.strip() for line in out.splitlines() if line.strip()]


def main() -> int:
    paths = changed_paths()
    build_prefixes = ("src/", "qml/", "resources/", "packaging/", "tests/")
    build_names = {"CMakeLists.txt"}
    build = any(
        p == "<force-build>"
        or p in build_names
        or p.startswith(build_prefixes)
        for p in paths
    )
    output = Path(os.environ["GITHUB_OUTPUT"])
    with output.open("a", encoding="utf-8") as fh:
        fh.write(f"build_required={'true' if build else 'false'}\n")
        fh.write(f"changed_count={len(paths)}\n")
    print(f"build_required={build}; changed={len(paths)}")
    for path in paths[:80]:
        print(f"  {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
