#!/usr/bin/env python3
"""Parse a K500 CI command without evaluating any user-controlled shell text.

Only repository-owner comments on K500 issue #241 or on an actual PR may
request native builds. GitHub API validation of PR head/base happens in the
authenticated comment controller and AGAIN in the dispatched test job.
"""
from __future__ import annotations

import os
import re
from pathlib import Path

COMMAND = re.compile(r"/k500 ci (fast|smart|windows|installer|platform|full)")


def parse_ci_comment(comment: str, actor: str, owner: str,
                     number: str, is_pr: bool) -> str:
    if not owner or actor != owner:
        raise ValueError("CI commands require exact repository-owner identity")
    if not (number.isdecimal() and int(number) > 0):
        raise ValueError("missing issue or PR number")
    if not is_pr and number != "241":
        raise ValueError("main-branch CI commands are restricted to issue #241")
    match = COMMAND.fullmatch(comment)
    if not match:
        raise ValueError("use exactly '/k500 ci fast|smart|windows|installer|platform|full'")
    return match.group(1)


def main() -> int:
    mode = parse_ci_comment(
        os.environ.get("K500_CI_COMMENT", ""),
        os.environ.get("K500_CI_ACTOR", ""),
        os.environ.get("GITHUB_REPOSITORY_OWNER", ""),
        os.environ.get("K500_CI_NUMBER", ""),
        os.environ.get("K500_CI_IS_PR", "").lower() == "true",
    )
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        output.write(f"ci_mode={mode}\n")
    print(f"Authorized targeted CI command: {mode}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
