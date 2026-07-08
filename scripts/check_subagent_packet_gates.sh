#!/usr/bin/env bash
# Mandatory OMNI gates for every subagent packet merge. Exit nonzero on any failure.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail=0

run_gate() {
  local name="$1"
  shift
  echo "== GATE: ${name} =="
  if "$@"; then
    echo "PASS: ${name}"
  else
    echo "FAIL: ${name}"
    fail=1
  fi
  echo
}

run_gate "duplicate-authority" bash scripts/check_desktop_v2_duplicate_authority.sh

if [[ -x scripts/check_desktop_v2_layout_authority.sh ]]; then
  run_gate "layout-authority" bash scripts/check_desktop_v2_layout_authority.sh
fi

if [[ -x desktop-v2/build/FroggersV2ProjectionValidators_test ]]; then
  run_gate "projection-validators" desktop-v2/build/FroggersV2ProjectionValidators_test
else
  echo "SKIP: projection-validators (desktop-v2/build/FroggersV2ProjectionValidators_test missing)"
  echo
fi

if [[ -x desktop-v2/build/FroggersV2Manifest_test ]]; then
  run_gate "manifest-foundation" desktop-v2/build/FroggersV2Manifest_test
else
  echo "SKIP: manifest-foundation (desktop-v2/build/FroggersV2Manifest_test missing)"
  echo
fi

if [[ "${fail}" -ne 0 ]]; then
  echo "Subagent packet gates FAILED"
  exit 1
fi

echo "Subagent packet gates OK"
