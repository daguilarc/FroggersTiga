#!/usr/bin/env bash
# Validates VCV panel layout constants in VcvPanelLayout.hpp fit within HP bounds.
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
kVoicingHp="$(read_const kVoicingHp)"
kFxHp="$(read_const kFxHp)"
kColumnHp="$(read_const kColumnHp)"
kVoicingColumns="$(read_const kVoicingColumns)"
kFxColumns="$(read_const kFxColumns)"
kRows="$(grep -E 'constexpr uint8_t kNumRows = [0-9]+' "$ROOT/sim/ParamDisplayNames.hpp" | sed -E 's/.*= *([0-9]+);.*/\1/')"
primaryGateGridX="$(read_const kPrimaryGateGridX)"
globalCrunchyGridX="$(read_const kPrimaryGlobalCrunchyGridX)"
globalCrunchyCvGridX="$(read_const kPrimaryGlobalCrunchyCvGridX)"
globalCrunchyGridY="$(read_const kPrimaryGlobalCrunchyGridY)"

GRID=15
RACK_HEIGHT=380

primaryRightGrid="$(read_const kPrimaryRightmostIoGrid)"

primaryWidthPx="$(awk "BEGIN { print ${kPrimaryHp} * ${GRID} }")"
kMainHp="$(awk "BEGIN { print ${kPrimaryHp} + ${kVoicingHp} }")"
mainWidthPx="$(awk "BEGIN { print ${kMainHp} * ${GRID} }")"
fxWidthPx="$(awk "BEGIN { print ${kFxHp} * ${GRID} }")"

if grep -q 'constexpr float kMainHp = kPrimaryHp + kVoicingHp' "$HEADER"; then
    :
else
    echo "check_vcv_panel_bounds: kMainHp must equal kPrimaryHp + kVoicingHp in header" >&2
    exit 1
fi

primaryRightPx="$(awk "BEGIN { print ${primaryRightGrid} * ${GRID} }")"
if awk "BEGIN { exit !(${primaryRightPx} <= ${primaryWidthPx}) }"; then
    :
else
    echo "check_vcv_panel_bounds: primary I/O exceeds ${kPrimaryHp} HP (${primaryRightPx}px > ${primaryWidthPx}px)" >&2
    exit 1
fi

# Global Crunchy knob/CV must stay inside the primary panel and clear the gate jack horizontally.
globalGateXDelta="$(awk "BEGIN { print ${globalCrunchyGridX} - ${primaryGateGridX} }")"
if awk "BEGIN { exit !(${globalGateXDelta} >= 2) }"; then
    :
else
    echo "check_vcv_panel_bounds: global Crunchy too close to gate on X (dx=${globalGateXDelta} GRID)" >&2
    exit 1
fi

if awk "BEGIN { exit !(${globalCrunchyGridX} > 0 && ${globalCrunchyCvGridX} > 0 && ${globalCrunchyCvGridX} <= ${kPrimaryHp} && ${globalCrunchyGridY} > 0) }"; then
    :
else
    echo "check_vcv_panel_bounds: global Crunchy placement falls outside primary bounds" >&2
    exit 1
fi

check_expander_columns() {
    local hp="$1"
    local columns="$2"
    local label="$3"
    local hpOffset="${4:-0}"
    local halfSpanGrid=4.5
    local colWidthHp
    colWidthHp="$(awk "BEGIN { print ${hp} / ${columns} }")"
    for ((col = 0; col < columns; col++)); do
        local centerHp leftEdgeHp rightEdgeHp colLeftHp colRightHp
        centerHp="$(awk "BEGIN { print ${hpOffset} + ${colWidthHp} * (0.5 + ${col}) }")"
        leftEdgeHp="$(awk "BEGIN { print ${centerHp} - ${halfSpanGrid} }")"
        rightEdgeHp="$(awk "BEGIN { print ${centerHp} + ${halfSpanGrid} }")"
        colLeftHp="$(awk "BEGIN { print ${hpOffset} + ${col} * ${colWidthHp} }")"
        colRightHp="$(awk "BEGIN { print ${hpOffset} + (${col} + 1) * ${colWidthHp} }")"

        if awk "BEGIN { exit !(${leftEdgeHp} >= ${colLeftHp} && ${rightEdgeHp} <= ${colRightHp}) }"; then
            :
        else
            echo "check_vcv_panel_bounds: ${label} column ${col} widgets span outside column bounds" >&2
            exit 1
        fi
    done
}

check_expander_columns "$kVoicingHp" "$kVoicingColumns" "main voicing" "$kPrimaryHp"
check_expander_columns "$kFxHp" "$kFxColumns" "fx" "0"

rowStep="$(awk "BEGIN { print ${RACK_HEIGHT} / (${kRows} + 2) }")"
headerGrid="$(read_const kHeaderStripGridY)"
headerPx="$(awk "BEGIN { print ${headerGrid} * ${GRID} }")"
lastRowY="$(awk "BEGIN { print ${headerPx} + ${rowStep} * (1.5 + ${kRows} - 1) }")"
primaryIoY="$(awk "BEGIN { print ${headerPx} + ${RACK_HEIGHT} - 2.5 * ${GRID} }")"
if awk "BEGIN { exit !(${lastRowY} <= ${RACK_HEIGHT}) }"; then
    :
else
    echo "check_vcv_panel_bounds: last row Y exceeds panel height (${lastRowY} > ${RACK_HEIGHT})" >&2
    exit 1
fi

if awk "BEGIN { exit !(${primaryIoY} <= ${RACK_HEIGHT}) }"; then
    :
else
    echo "check_vcv_panel_bounds: primary I/O row exceeds panel height (${primaryIoY} > ${RACK_HEIGHT})" >&2
    exit 1
fi

expectedVoicingHp="$(awk "BEGIN { print ${kVoicingColumns} * ${kColumnHp} }")"
if awk "BEGIN { exit !(${kVoicingHp} == ${expectedVoicingHp}) }"; then
    :
else
    echo "check_vcv_panel_bounds: voicing HP ${kVoicingHp} != ${kVoicingColumns}×${kColumnHp}" >&2
    exit 1
fi

expectedFxHp="$(awk "BEGIN { print ${kFxColumns} * ${kColumnHp} + 12 }")"
if awk "BEGIN { exit !(${kFxHp} == ${expectedFxHp}) }"; then
    :
else
    echo "check_vcv_panel_bounds: fx HP ${kFxHp} != ${kFxColumns}×${kColumnHp}+12" >&2
    exit 1
fi

echo "check_vcv_panel_bounds: OK (main=${kMainHp}HP fx=${kFxHp}HP)"
