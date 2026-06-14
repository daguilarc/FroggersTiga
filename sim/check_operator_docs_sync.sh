#!/usr/bin/env bash
# Verify operator docs mirrors match canonical root files.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

check_pair() {
  local canonical="$1"
  local mirror="$2"
  if [[ ! -f "$canonical" ]]; then
    echo "missing canonical: $canonical" >&2
    exit 1
  fi
  if [[ ! -f "$mirror" ]]; then
    echo "missing mirror: $mirror" >&2
    exit 1
  fi
  if ! cmp -s "$canonical" "$mirror"; then
    echo "drift: $mirror differs from $canonical" >&2
    exit 1
  fi
}

check_pair SIM_MANUAL.md docs/sim-manual.md
check_pair SIM_MANUAL.md web/public/sim-manual.md
check_pair QUICK_DICT.md docs/quick-dict.md
check_pair QUICK_DICT.md web/public/quick-dict.md

echo "operator docs mirrors in sync"
