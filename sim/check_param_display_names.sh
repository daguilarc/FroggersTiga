#!/usr/bin/env bash
# Verify web knob labels: WASM row names when synced, static fallback from paramDisplayNames.ts (hpp parity).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAIN_TS="$ROOT/web/src/main.ts"
PARAM_TS="$ROOT/web/src/paramDisplayNames.ts"
HPP="$ROOT/sim/ParamDisplayNames.hpp"

for f in "$MAIN_TS" "$PARAM_TS" "$HPP"; do
  if [[ ! -f "$f" ]]; then
    echo "missing $f" >&2
    exit 1
  fi
done

python3 - "$MAIN_TS" "$PARAM_TS" "$HPP" <<'PY'
import re
import sys

main_ts = open(sys.argv[1], encoding="utf-8").read()
param_ts = open(sys.argv[2], encoding="utf-8").read()
hpp = open(sys.argv[3], encoding="utf-8").read()

if "HOST_PAGE_LABELS" in main_ts:
    sys.exit("HOST_PAGE_LABELS must be removed")

if "applyKnobLabelsFromRows" not in main_ts:
    sys.exit("missing applyKnobLabelsFromRows")

if "paramDisplayNames" not in main_ts or "coreKnobLabel" not in main_ts:
    sys.exit("main.ts must import coreKnobLabel from paramDisplayNames.ts")

if "wasmCore" not in main_ts or "?.name" not in main_ts:
    sys.exit("knob labels must read wasm row names when screen rows are present")

if "coreKnobLabel(hostPage" not in main_ts:
    sys.exit("knob labels must fall back to static coreKnobLabel on page change")

if "HOST_PAGE_KNOB_LABELS" not in param_ts or "PAIR_AR_KNOB_LABELS" not in param_ts:
    sys.exit("paramDisplayNames.ts must export HOST_PAGE_KNOB_LABELS and PAIR_AR_KNOB_LABELS")

hpp_rows = re.findall(r'\{"([^"]+)"(?:,\s*"([^"]+)"){7}\}', hpp)
if len(hpp_rows) != 6:
    sys.exit(f"expected 6 host pages in ParamDisplayNames.hpp, found {len(hpp_rows)}")

ts_rows = re.findall(r'\["([^"]+)"(?:,\s*"([^"]+)"){7}\]', param_ts)
if len(ts_rows) != 6:
    sys.exit(f"expected 6 host pages in paramDisplayNames.ts, found {len(ts_rows)}")

hpp_table = [list(re.findall(r'"([^"]+)"', row[0] + '",' + '","'.join(row[1:]))) for row in hpp_rows]
# Re-parse hpp table lines directly
hpp_block = re.search(
    r'kTable\[kNumHostPages\]\[kNumRows\] = \{(.*?)\};',
    hpp,
    re.S,
)
if not hpp_block:
    sys.exit("could not parse kTable from ParamDisplayNames.hpp")

hpp_labels: list[list[str]] = []
for line in hpp_block.group(1).split("\n"):
    line = line.strip().rstrip(",")
    if not line.startswith("{"):
        continue
    hpp_labels.append(re.findall(r'"([^"]+)"', line))

ts_block = re.search(r"HOST_PAGE_KNOB_LABELS.*?=\s*\[(.*?)\];", param_ts, re.S)
if not ts_block:
    sys.exit("could not parse HOST_PAGE_KNOB_LABELS from paramDisplayNames.ts")

ts_labels: list[list[str]] = []
for line in ts_block.group(1).split("\n"):
    line = line.strip().rstrip(",")
    if not line.startswith("["):
        continue
    ts_labels.append(re.findall(r'"([^"]+)"', line))

if hpp_labels != ts_labels:
    sys.exit("HOST_PAGE_KNOB_LABELS diverges from ParamDisplayNames.hpp kTable")

hpp_pair = re.search(r'kLabels\[4\] = \{(.*?)\};', hpp, re.S)
ts_pair = re.search(r"PAIR_AR_KNOB_LABELS = \[(.*?)\]", param_ts, re.S)
if not hpp_pair or not ts_pair:
    sys.exit("could not parse pair-AR labels")

hpp_pair_labels = re.findall(r'"([^"]+)"', hpp_pair.group(1))
ts_pair_labels = re.findall(r'"([^"]+)"', ts_pair.group(1))
if hpp_pair_labels != ts_pair_labels:
    sys.exit("PAIR_AR_KNOB_LABELS diverges from ParamDisplayNames.hpp")

print("web knob labels: wasm rows + static paramDisplayNames parity ok")
PY
