#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

if [[ -z "${RACK_DIR:-}" ]]; then
  echo "error: RACK_DIR is not set" >&2
  echo "  export RACK_DIR=~/Rack-SDK   # clone from https://github.com/VCVRack/Rack-SDK" >&2
  exit 1
fi

if [[ ! -f "$RACK_DIR/plugin.mk" ]]; then
  echo "error: RACK_DIR does not look like Rack-SDK (missing plugin.mk): $RACK_DIR" >&2
  exit 1
fi

make -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"
echo "Built: $ROOT/dist/FroggersTiga"
