#!/usr/bin/env bash
# Verify web UI delegates row labels to wasm (no duplicate HOST_PAGE_LABELS table).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAIN_TS="$ROOT/web/src/main.ts"

if [[ ! -f "$MAIN_TS" ]]; then
  echo "missing web/src/main.ts" >&2
  exit 1
fi

python3 - "$MAIN_TS" <<'PY'
import sys

main_ts = open(sys.argv[1], encoding="utf-8").read()

if "HOST_PAGE_LABELS" in main_ts:
    sys.exit("HOST_PAGE_LABELS must be removed; labels come from wasm rows[].name")

if "applyKnobLabelsFromRows" not in main_ts:
    sys.exit("missing applyKnobLabelsFromRows")

if "row?.name" not in main_ts and "row.name" not in main_ts:
    sys.exit("knob labels must use row.name from worklet screen payload")

print("web knob labels use wasm authority")
PY
