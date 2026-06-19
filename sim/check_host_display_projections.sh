#!/usr/bin/env bash
# Verify host mod-rack consumers match HostPanelLayout projections.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

node scripts/generate-host-display.mjs --check
node scripts/verify-host-display-shape.mjs

DESKTOP_CPP="$ROOT/desktop/Source/ModRackPanel.cpp"
if ! grep -q 'kModRackCatalog' "$DESKTOP_CPP"; then
  echo "error: ModRackPanel must iterate HostPanelLayout::kModRackCatalog" >&2
  exit 1
fi
if ! grep -q 'includeDesktop' "$DESKTOP_CPP"; then
  echo "error: ModRackPanel must filter catalog by includeDesktop" >&2
  exit 1
fi

WASM_HPP="$ROOT/sim/WasmSimHost.hpp"
if ! grep -q 'kScopeModIndices = {0, 4, 5, 6}' "$WASM_HPP"; then
  echo "error: WasmSimHost web scope indices must be {0, 4, 5, 6}" >&2
  exit 1
fi

BINDINGS="$ROOT/wasm/bindings.cpp"
if grep -q '{0, 1, 4, 5, 6}' "$BINDINGS"; then
  echo "error: wasm bindings must not use desktop assignable pool {0,1,4,5,6}" >&2
  exit 1
fi

echo "host display projections ok (desktop catalog, web wasm pool)"
