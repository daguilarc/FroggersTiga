#!/usr/bin/env bash
# Fails if shipped VCV panel SVGs contain live <text> (invisible in Rack nanosvg).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PANEL_DIR="$ROOT/vcv/res"

if [[ ! -d "$PANEL_DIR" ]]; then
    echo "check_vcv_panel_svg: skip (no vcv/res — local-only tree)"
    exit 0
fi

shopt -s nullglob
files=("$PANEL_DIR"/FroggersTiga*.svg)
if ((${#files[@]} == 0)); then
    echo "check_vcv_panel_svg: no FroggersTiga*.svg in vcv/res" >&2
    exit 1
fi

for svg in "${files[@]}"; do
    base="$(basename "$svg")"
    if grep -q '<text' "$svg"; then
        echo "check_vcv_panel_svg: live <text> in $base (convert to paths)" >&2
        exit 1
    fi
    if ! grep -q '<path' "$svg"; then
        echo "check_vcv_panel_svg: no <path> silkscreen in $base" >&2
        exit 1
    fi
done

echo "check_vcv_panel_svg: OK (${#files[@]} files, no live text)"
