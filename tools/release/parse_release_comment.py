#!/usr/bin/env python3
"""Small, offline, fail-closed parser for owner-initiated K500 release comments.

The GitHub job separately confirms the live main SHA and exact-main CI.
Never execute a comment as shell or infer owner acceptance from a command.
"""
from __future__ import annotations

import os
import re
from pathlib import Path

RC = re.compile(r"/k500 release rc v([0-9]+\.[0-9]+\.[0-9]+)-rc\.([1-9][0-9]{0,3})")
STABLE = re.compile(r"/k500 release stable v([0-9]+\.[0-9]+\.[0-9]+)")
CMAKE_VERSION = re.compile(
    r"(?m)^project\(SonkupikStudioNative VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES CXX\)$"
)
RELEASE_ISSUE = "241"


def parse(comment: str, actor: str, owner: str, issue: str,
          is_pull_request: bool, cmake_text: str) -> tuple[str, str, str]:
    """Return (mode, version, rc_number), with no side effects."""
    if not owner or actor != owner or issue != RELEASE_ISSUE or is_pull_request:
        raise ValueError("release commands require repository owner on issue #241")
    version_match = CMAKE_VERSION.search(cmake_text)
    if not version_match:
        raise ValueError("CMake project version is missing or ambiguous")
    version = version_match.group(1)
    if version == "1.1.5":
        raise ValueError("v1.1.5 public release line is immutable")

    if match := RC.fullmatch(comment):
        if match.group(1) != version:
            raise ValueError("requested RC version must match current CMake version")
        return "rc", version, match.group(2)
    if match := STABLE.fullmatch(comment):
        if match.group(1) != version:
            raise ValueError("requested stable version must match current CMake version")
        return "stable", version, ""
    raise ValueError(
        "invalid release command; use exactly '/k500 release rc vX.Y.Z-rc.N' "
        "or '/k500 release stable vX.Y.Z'"
    )


def main() -> int:
    mode, version, number = parse(
        os.environ.get("K500_RELEASE_COMMENT", ""),
        os.environ.get("K500_RELEASE_ACTOR", ""),
        os.environ.get("GITHUB_REPOSITORY_OWNER", ""),
        os.environ.get("K500_RELEASE_ISSUE", ""),
        os.environ.get("K500_RELEASE_IS_PR", "").lower() == "true",
        Path("CMakeLists.txt").read_text(encoding="utf-8"),
    )
    output = Path(os.environ["GITHUB_OUTPUT"])
    with output.open("a", encoding="utf-8") as stream:
        stream.write(f"mode={mode}\nversion={version}\nrc_number={number}\n")
    print(f"Authenticated owner release request: {mode} v{version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
