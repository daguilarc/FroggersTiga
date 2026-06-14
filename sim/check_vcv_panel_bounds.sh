#!/usr/bin/env bash
# Validates VCV panel layout constants in FieldParityWidget.hpp fit within HP bounds.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HEADER="$ROOT/sim/VcvPanelLayout.hpp"

if [[ ! -f "$HEADER" ]]; then
    echo "check_vcv_panel_bounds: missing $HEADER" >&2
    exit 1
fi

read_const() {
    local name="$1"
    grep -E "constexpr .* ${name} =" "$HEADER" | head -1 | sed -E 's/.*= *([^;]+);.*/\1/' | sed 's/\.f$//'
}

kPrimaryHp="$(read_const kPrimaryHp)"
kExpanderHp="$(read_const kExpanderHp)"
kColumnHp="$(read_const kColumnHp)"
kExpanderColumns="$(read_const kExpanderColumns)"
kRows="$(grep -E 'constexpr uint8_t kNumRows = [0-9]+' "$ROOT/sim/ParamDisplayNames.hpp" | sed -E 's/.*= *([0-9]+);.*/\1/')"

GRID=15
RACK_HEIGHT=380

primaryRightGrid="$(read_const kPrimaryRightmostIoGrid)"

primaryWidthPx="$(awk "BEGIN { print ${kPrimaryHp} * ${GRID} }")"
expanderWidthPx="$(awk "BEGIN { print ${kExpanderHp} * ${GRID} }")"
columnWidthPx="$(awk "BEGIN { print ${kColumnHp} * ${GRID} }")"

# Primary I/O rightmost jack must fit primary panel width.
primaryRightPx="$(awk "BEGIN { print ${primaryRightGrid} * ${GRID} }")"
if awk "BEGIN { exit !(${primaryRightPx} <= ${primaryWidthPx}) }"; then
    :
else
    echo "check_vcv_panel_bounds: primary I/O exceeds ${kPrimaryHp} HP (${primaryRightPx}px > ${primaryWidthPx}px)" >&2
    exit 1
fi

# Expander column widget span: knob centered + mod jack at +3 GRID → right edge ~+4.5 GRID from center.
halfSpanGrid=4.5
for ((col = 0; col < kExpanderColumns; col++)); do
    centerHp="$(awk "BEGIN { print ${kColumnHp} * (0.5 + ${col}) }")"
    leftEdgeHp="$(awk "BEGIN { print ${centerHp} - ${halfSpanGrid} }")"
    rightEdgeHp="$(awk "BEGIN { print ${centerHp} + ${halfSpanGrid} }")"
    colLeftHp="$(awk "BEGIN { print ${col} * ${kColumnHp} }")"
    colRightHp="$(awk "BEGIN { print (${col} + 1) * ${kColumnHp} }")"

    if awk "BEGIN { exit !(${leftEdgeHp} >= ${colLeftHp} && ${rightEdgeHp} <= ${colRightHp}) }"; then
        :
    else
        echo "check_vcv_panel_bounds: column ${col} widgets span outside ${kColumnHp} HP column" >&2
        exit 1
    fi
done

# Row Y positions must fit panel height.
rowStep="$(awk "BEGIN { print ${RACK_HEIGHT} / (${kRows} + 2) }")"
lastRowY="$(awk "BEGIN { print ${rowStep} * (1.5 + ${kRows} - 1) }")"
if awk "BEGIN { exit !(${lastRowY} <= ${RACK_HEIGHT}) }"; then
    :
else
    echo "check_vcv_panel_bounds: last row Y exceeds panel height (${lastRowY} > ${RACK_HEIGHT})" >&2
    exit 1
fi

# Expander width must equal columns × column HP.
expectedExpanderHp="$(awk "BEGIN { print ${kExpanderColumns} * ${kColumnHp} }")"
if awk "BEGIN { exit !(${kExpanderHp} == ${expectedExpanderHp}) }"; then
    :
else
    echo "check_vcv_panel_bounds: expander HP ${kExpanderHp} != ${kExpanderColumns}×${kColumnHp}" >&2
    exit 1
fi

echo "check_vcv_panel_bounds: OK (primary=${kPrimaryHp}HP expander=${kExpanderHp}HP columns=${kExpanderColumns})"
