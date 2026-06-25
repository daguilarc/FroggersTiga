#!/usr/bin/env bash
# Keep firmware compatibility mirrors in src/common thin.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail=0

while IFS= read -r common; do
  name="$(basename "$common")"
  core="src/core/$name"
  [[ -f "$core" ]] || continue

  if ! diff -u <(printf '#pragma once\n#include "../core/%s"\n' "$name") "$common" >/dev/null; then
    echo "FAIL: $common must remain a thin wrapper around $core" >&2
    fail=1
  fi
done < <(find src/common -maxdepth 1 -type f -name '*.hpp' | sort)

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

echo "OK: src/common core wrappers are thin"
