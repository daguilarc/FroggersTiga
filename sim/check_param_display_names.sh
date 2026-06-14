#!/usr/bin/env bash
# paramDisplayNames.ts must match sim/ParamDisplayNames.hpp (single label authority).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
python3 - "$ROOT/sim/ParamDisplayNames.hpp" "$ROOT/web/src/paramDisplayNames.ts" <<'PY'
import re
import sys

hpp = open(sys.argv[1], encoding="utf-8").read()
param_ts = open(sys.argv[2], encoding="utf-8").read()

def table_from_hpp(text: str) -> list[list[str]]:
    block = re.search(r"kTable\[kNumHostPages\]\[kNumRows\] = \{(.*?)\};", text, re.S)
    if not block:
        sys.exit("could not parse kTable from ParamDisplayNames.hpp")
    rows: list[list[str]] = []
    for line in block.group(1).split("\n"):
        line = line.strip().rstrip(",")
        if line.startswith("{"):
            rows.append(re.findall(r'"([^"]+)"', line))
    return rows

def table_from_ts(text: str) -> list[list[str]]:
    block = re.search(r"HOST_PAGE_KNOB_LABELS.*?=\s*\[(.*?)\];", text, re.S)
    if not block:
        sys.exit("could not parse HOST_PAGE_KNOB_LABELS from paramDisplayNames.ts")
    rows: list[list[str]] = []
    for line in block.group(1).split("\n"):
        line = line.strip().rstrip(",")
        if line.startswith("["):
            rows.append(re.findall(r'"([^"]+)"', line))
    return rows

hpp_rows = table_from_hpp(hpp)
ts_rows = table_from_ts(param_ts)
if hpp_rows != ts_rows:
    sys.exit("HOST_PAGE_KNOB_LABELS diverges from ParamDisplayNames.hpp")

hpp_pair = re.findall(
    r'"([^"]+)"',
    re.search(r"kLabels\[4\] = \{(.*?)\};", hpp, re.S).group(1),
)
ts_pair = re.findall(r'"([^"]+)"', re.search(r"PAIR_AR_KNOB_LABELS = \[(.*?)\]", param_ts, re.S).group(1))
if hpp_pair != ts_pair:
    sys.exit("PAIR_AR_KNOB_LABELS diverges from ParamDisplayNames.hpp")

print("paramDisplayNames.ts matches ParamDisplayNames.hpp")
PY
