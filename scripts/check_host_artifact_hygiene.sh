#!/usr/bin/env bash
# Reject tracked host build outputs and blanket OpenSpec ignores.
# Firmware, vendored, and docs/ publication paths are out of scope.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail=0
GITIGNORE="$ROOT/.gitignore"

if [[ ! -f "$GITIGNORE" ]]; then
  echo "FAIL: missing .gitignore" >&2
  exit 1
fi

if grep -E '^openspec/?$' "$GITIGNORE"; then
  echo "FAIL: .gitignore has blanket openspec/ ignore; use selective ephemeral cache rules" >&2
  fail=1
fi

required_ignores=(
  sim/build/
  desktop/build/
  desktop/dist/
  wasm/build/
  web/dist/
  vcv/build/
  vcv/dist/
  vcv/dep/
)
for pattern in "${required_ignores[@]}"; do
  if ! grep -qxF "$pattern" "$GITIGNORE"; then
    echo "FAIL: .gitignore missing required ignore: $pattern" >&2
    fail=1
  fi
done

is_excluded() {
  case "$1" in
    External/*|src/FroggersTiga/*|src/common/*|src/mk/*|src/Blink/*|src/TestControl/*|MANUAL.md|docs/*)
      return 0
      ;;
  esac
  return 1
}

prohibited_prefixes=(
  sim/build/
  desktop/build/
  desktop/dist/
  wasm/build/
  web/dist/
  vcv/build/
  vcv/dist/
  vcv/dep/
  .emsdk/
  desktop/FroggersTigaPlugin_artefacts/
  Rack-SDK/
  vcv/Rack-SDK/
  openspec/.cache/
  openspec/.sessions/
)

is_prohibited_tracked() {
  local path="$1"
  local prefix

  for prefix in "${prohibited_prefixes[@]}"; do
    if [[ "$path" == "$prefix"* ]]; then
      return 0
    fi
  done

  case "$path" in
    sim/CMakeCache.txt|sim/CMakeFiles/*|sim/cmake_install.cmake|sim/Makefile|sim/CTestTestfile.cmake)
      return 0
      ;;
    sim/*.o|sim/*.o.d|sim/*_test)
      return 0
      ;;
    desktop/CMakeCache.txt|desktop/CMakeFiles/*|desktop/cmake_install.cmake|desktop/Makefile)
      return 0
      ;;
    desktop/*.o|desktop/**/*.o|desktop/*.o.d)
      return 0
      ;;
    wasm/CMakeCache.txt|wasm/CMakeFiles/*|wasm/cmake_install.cmake|wasm/Makefile|wasm/*.o|wasm/*.wasm)
      return 0
      ;;
    web/dist/*|web/node_modules/*)
      return 0
      ;;
    vcv/CMakeCache.txt|vcv/CMakeFiles/*|vcv/cmake_install.cmake|vcv/Makefile|vcv/*.o)
      return 0
      ;;
    openspec/.cache/*|openspec/.sessions/*|openspec/*/.cache/*|openspec/*/.sessions/*)
      return 0
      ;;
  esac

  return 1
}

while IFS= read -r tracked; do
  [[ -z "$tracked" ]] && continue
  if is_excluded "$tracked"; then
    continue
  fi
  if is_prohibited_tracked "$tracked"; then
    echo "FAIL: prohibited tracked host artifact: $tracked" >&2
    fail=1
  fi
done < <(git ls-files)

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

echo "OK: host artifact hygiene"
