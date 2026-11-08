#!/usr/bin/env python3
"""K500 SMART_CI_SCOPE_V3: fast-on-change, native-on-owner-comment only.

All automatic PR/main CI is cheap/offline. A deliberately authenticated
workflow_dispatch (from the existing GitHub issue_comment controller) may
request a targeted native regression against an exact immutable Git SHA.
No release or stable promotion is ever implied by a source push.
"""
from __future__ import annotations

import os
import subprocess
from pathlib import Path

ALLOWED = {"fast", "smart", "windows", "installer", "platform", "full"}

WINDOWS_NAMES = {
    "CMakeLists.txt",
    ".github/workflows/windows-build.yml",
    "src/k500/K500WinIo.cpp",
    "src/k500/K500WinIo.h",
}
WINDOWS_PREFIXES = (
    "src/", "qml/", "resources/", "packaging/windows/", "tests/windows/",
    "tools/ci/windows_", "tools/ci/installer_",
)
INSTALLER_PREFIXES = (
    "packaging/windows/", "tests/windows/", "src/AppUpdateManager",
    "qml/components/UpdateDialog.qml",
)
INSTALLER_NAMES = {
    "CMakeLists.txt", "src/AppVersionInit.cpp",
    "tools/ci/installer_smoke.ps1",
}
PLATFORM_PREFIXES = (
    "src/k500/K500PosixIo.", "src/k500/K500TransactionScheduler.",
    "src/k500/K500DeviceManager.", "src/k500/K500CanonicalState.",
)
PLATFORM_NAMES = {"CMakeLists.txt", ".github/workflows/windows-build.yml"}


def changed_paths(base: str, head: str) -> list[str]:
    if not base or not head or set(base) == {"0"}:
        return ["<unknown-diff>"]
    try:
        out = subprocess.check_output(
            ["git", "diff", "--name-only", f"{base}...{head}"],
            text=True, stderr=subprocess.DEVNULL,
        )
    except (subprocess.CalledProcessError, OSError):
        return ["<unknown-diff>"]
    return [line.strip() for line in out.splitlines() if line.strip()]


def scopes(mode: str, paths: list[str], manual: bool) -> tuple[bool, bool, bool]:
    """Return (Windows, Installer, Linux+macOS); never start native in auto CI."""
    if mode not in ALLOWED:
        raise ValueError(f"Unsupported CI mode: {mode}")
    if not manual or mode == "fast":
        return False, False, False
    if mode == "full":
        return True, True, True
    if mode == "windows":
        return True, False, False
    if mode == "installer":
        return False, True, False
    if mode == "platform":
        return False, False, True
    # smart: if git history is unavailable, fail safe by qualifying all.
    if "<unknown-diff>" in paths:
        return True, True, True
    # POSIX transport is excluded from Windows-only compilation by CMake.
    windows = any(
        (p in WINDOWS_NAMES or p.startswith(WINDOWS_PREFIXES))
        and not p.startswith("src/k500/K500PosixIo.")
        for p in paths
    )
    installer = any(
        p in INSTALLER_NAMES or p.startswith(INSTALLER_PREFIXES)
        for p in paths
    )
    platform = any(
        p in PLATFORM_NAMES or p.startswith(PLATFORM_PREFIXES)
        for p in paths
    )
    return windows, installer, platform


def main() -> int:
    event = os.environ.get("GITHUB_EVENT_NAME", "")
    manual = event == "workflow_dispatch"
    mode = os.environ.get("CI_REQUEST_SCOPE", "fast") if manual else "fast"
    if mode not in ALLOWED:
        raise ValueError(f"Unsupported CI mode {mode!r}")
    base = os.environ.get("CI_BASE_SHA", "").strip()
    head = os.environ.get("CI_HEAD_SHA", os.environ.get("GITHUB_SHA", "HEAD")).strip()
    # Skip git diff entirely for automatic fast-only runs or explicit suites.
    paths = changed_paths(base, head) if manual and mode == "smart" else []
    build, installer, platform = scopes(mode, paths, manual)

    output = Path(os.environ["GITHUB_OUTPUT"])
    with output.open("a", encoding="utf-8") as fh:
        fh.write(f"build_required={str(build).lower()}\n")
        fh.write(f"installer_required={str(installer).lower()}\n")
        fh.write(f"platform_required={str(platform).lower()}\n")
        # SMART_STABLE_PROMOTION_GATE_V1 was previously push-driven. Removed:
        # only authenticated issue-comment owner acceptance promotes RC bytes.
        fh.write("promotion_required=false\n")
        fh.write(f"changed_count={len(paths)}\n")
    print(
        f"SMART_CI_SCOPE_V3 event={event} mode={mode} manual={manual} "
        f"windows={build} installer={installer} platform={platform} "
        f"files={len(paths)}"
    )
    for path in paths[:70]:
        print(f"  {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
