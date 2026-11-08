#!/usr/bin/env bash
# Offline regression: grep -q exits before tar completes, causing false
# SIGPIPE with pipefail when the launcher is near the start of a Qt archive.
set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
workflow="$repo_root/.github/workflows/windows-build.yml"
grep -F 'tar -tzf "dist/${{ matrix.artifact }}" | grep -Fx' "$workflow" >/dev/null
if grep -F 'tar -tzf "dist/${{ matrix.artifact }}" | grep -q' "$workflow" >/dev/null; then
  echo "Unsafe early-exit grep -q is back in Linux release pipeline" >&2
  exit 1
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/stage/bin" "$tmp/stage/lib"
printf '%s\n' '#!/bin/sh' 'exit 0' > "$tmp/stage/run-sonkupik-k500"
chmod +x "$tmp/stage/run-sonkupik-k500"
for n in $(seq 1 1500); do
  printf 'qt-library-fixture-%s\n' "$n" > "$tmp/stage/lib/Qt-$n.txt"
done
tar -czf "$tmp/K500-Linux-QA.tar.gz" -C "$tmp/stage" .
tar -tzf "$tmp/K500-Linux-QA.tar.gz" | grep -Fx './run-sonkupik-k500' >/dev/null
test -s "$tmp/K500-Linux-QA.tar.gz"
echo "Linux tar listing: PASS (full stream consumed, no pipefail SIGPIPE)"
