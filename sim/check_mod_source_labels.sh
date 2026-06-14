#!/usr/bin/env bash
# Verify web mod-bay titles are empty placeholders (labels come from wasm at runtime).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HEADER="$ROOT/sim/ParamDisplayNames.hpp"
MAIN_TS="$ROOT/web/src/main.ts"

if [[ ! -f "$HEADER" || ! -f "$MAIN_TS" ]]; then
  echo "missing ParamDisplayNames.hpp or web/src/main.ts" >&2
  exit 1
fi

python3 - "$HEADER" "$MAIN_TS" <<'PY'
import re
import sys

header_path, main_ts_path = sys.argv[1:3]
header = open(header_path, encoding="utf-8").read()
main_ts = open(main_ts_path, encoding="utf-8").read()

expected = {}
for mod_index in (0, 1, 4, 5, 6):
    match = re.search(rf"case {mod_index}:\s*\n\s*return \"([^\"]+)\";", header)
    if not match:
        sys.exit(f"could not parse forModSource({mod_index})")
    expected[mod_index] = match.group(1)

if "HOST_PAGE_LABELS" in main_ts:
    sys.exit("HOST_PAGE_LABELS duplicate table must be removed from main.ts")

hardcoded = re.findall(r'new CvScopeCanvas\("([^"]*)"', main_ts)
hardcoded += re.findall(r'new ModLedIndicator\("([^"]*)"', main_ts)
for label in hardcoded:
    if label:
        sys.exit(f"hardcoded mod bay label {label!r}; use wasm modSourceNames")

if "applyModSourceLabels" not in main_ts:
    sys.exit("main.ts must apply modSourceNames from worklet")

if "coreKnobLabel" not in main_ts or "paramDisplayNames" not in main_ts:
    sys.exit("main.ts must use paramDisplayNames.ts for static knob labels")

if "wasmCore" not in main_ts and "row?.name" not in main_ts and "rows[i].name" not in main_ts:
    sys.exit("main.ts must read knob label names from wasm screen rows when present")

for match in re.finditer(r'<option value="(\d+)">([^<]+)</option>', main_ts):
    if match.group(1) != "255":
        sys.exit(
            f"main.ts must not hardcode mod-source option {match.group(1)!r} "
            f"({match.group(2)!r}); build from wasm assignableModOptions"
        )

if "populateModSelects" not in main_ts:
    sys.exit("main.ts must build mod selects from wasm via populateModSelects")

print("mod source labels delegated to wasm authority")
for idx, name in expected.items():
    print(f"  mod {idx}: {name}")
PY
