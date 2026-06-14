#!/usr/bin/env bash
# GPL boundary: rack SDK headers only under vcv/; MIT core must not include rack.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail=0

if ! rg -q 'GPL-3.0' vcv/LICENSE; then
  echo "FAIL: vcv/LICENSE must be GPL-3.0-or-later" >&2
  fail=1
fi

if rg -l 'rack\.hpp|<rack/' --glob '!vcv/**' --glob '!openspec/**' --glob '!*.md' --glob '!sim/check_vcv_license_boundary.sh' . 2>/dev/null; then
  echo "FAIL: rack.hpp referenced outside vcv/" >&2
  fail=1
fi

if rg -l '#include\s*[<"]rack' src/core sim wasm desktop/Source web 2>/dev/null; then
  echo "FAIL: rack include in MIT host trees" >&2
  fail=1
fi

VCV_PLUGIN="vcv/dist/FroggersTiga/plugin.dylib"
if [[ -f "$VCV_PLUGIN" ]]; then
  if nm -gU "$VCV_PLUGIN" 2>/dev/null | rg -q 'rack::'; then
    echo "OK: plugin links Rack runtime (expected for GPL wrapper)"
  fi
else
  echo "SKIP: $VCV_PLUGIN not built — run vcv/build.sh first for artifact check"
fi

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

echo "OK: VCV license boundary"
