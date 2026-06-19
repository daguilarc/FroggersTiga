#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT/wasm"
if [[ -f "$ROOT/.emsdk/emsdk_env.sh" ]]; then
  # shellcheck source=/dev/null
  . "$ROOT/.emsdk/emsdk_env.sh"
fi
emcmake cmake -B build
cmake --build build
cp build/froggers.wasm "$ROOT/web/public/froggers.wasm"
node "$ROOT/scripts/verify-wasm-exports.mjs"
