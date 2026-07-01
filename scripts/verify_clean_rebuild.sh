#!/usr/bin/env bash
# Task 8.6: rebuild host targets from clean output trees; fail on tracked drift or stale generated files.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail=0

note_fail() {
  echo "FAIL: $1" >&2
  fail=1
}

IGNORE_PREFIXES=(
  sim/build/
  desktop/build/
  desktop/build-vst-test/
  desktop/build-v2-vst/
  desktop-v2/build/
  desktop/dist/
  wasm/build/
  web/dist/
  web/node_modules/
  vcv/build/
  vcv/dist/
  vcv/dep/
  .emsdk/
  desktop/FroggersTigaPlugin_artefacts/
  desktop/FroggersTigaPluginV2_artefacts/
  Rack-SDK/
  vcv/Rack-SDK/
  openspec/.cache/
  openspec/.sessions/
)

is_ignored_status_path() {
  local path="$1"
  local prefix

  for prefix in "${IGNORE_PREFIXES[@]}"; do
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
    desktop-v2/CMakeCache.txt|desktop-v2/CMakeFiles/*|desktop-v2/cmake_install.cmake|desktop-v2/Makefile|desktop-v2/CTestTestfile.cmake)
      return 0
      ;;
    desktop-v2/*.o|desktop-v2/**/*.o|desktop-v2/*.o.d|desktop-v2/*_test)
      return 0
      ;;
    desktop/*.o|desktop/**/*.o|desktop/*.o.d)
      return 0
      ;;
    wasm/CMakeCache.txt|wasm/CMakeFiles/*|wasm/cmake_install.cmake|wasm/Makefile|wasm/*.o|wasm/*.wasm)
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

collect_unexpected_porcelain() {
  local line status path rest
  while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    status="${line:0:2}"
    rest="${line:3}"
    if [[ "$rest" == \"* ]]; then
      path="${rest#\"}"
      path="${path%%\"*}"
    elif [[ "$rest" == *" -> "* ]]; then
      path="${rest%% -> *}"
    else
      path="${rest%% *}"
    fi
    [[ -z "$path" ]] && continue
    if is_ignored_status_path "$path"; then
      continue
    fi
    printf '%s\n' "$line"
  done < <(git status --porcelain)
}

baseline_porcelain="$(mktemp)"
after_porcelain="$(mktemp)"
trap 'rm -f "$baseline_porcelain" "$after_porcelain"' EXIT

collect_unexpected_porcelain | sort -u > "$baseline_porcelain"

echo "== generate-host-display --check =="
if ! node scripts/generate-host-display.mjs --check; then
  note_fail "hostDisplay.generated.ts is stale; run: node scripts/generate-host-display.mjs"
fi

echo "== sim build + test =="
cmake -S sim -B sim/build
cmake --build sim/build
if ! ctest --test-dir sim/build --output-on-failure; then
  note_fail "sim ctest failed"
fi

echo "== web tsc + verify:host-display =="
if [[ ! -d web/node_modules ]]; then
  npm --prefix web install
fi
if ! (cd web && ./node_modules/.bin/tsc --noEmit); then
  note_fail "web tsc failed"
fi
if ! npm --prefix web run verify:host-display; then
  note_fail "web verify:host-display failed"
fi

echo "== desktop cmake build (BUILD_VST=ON) =="
cmake -S desktop -B desktop/build -DBUILD_VST=ON
if ! cmake --build desktop/build --config Release; then
  note_fail "desktop build failed"
fi
if [[ -f desktop/build/HostParameterProcessor_test ]]; then
  if ! ctest --test-dir desktop/build --output-on-failure -R 'HostParameter'; then
    note_fail "desktop HostParameter tests failed"
  fi
elif [[ -f desktop/build/Release/HostParameterProcessor_test.exe ]]; then
  if ! ctest --test-dir desktop/build --output-on-failure -C Release -R 'HostParameter'; then
    note_fail "desktop HostParameter tests failed"
  fi
fi

if [[ "${VERIFY_V2:-}" == "1" ]]; then
  echo "== desktop-v2 cmake build (BUILD_DESKTOP_V2=ON) =="
  cmake -S desktop-v2 -B desktop-v2/build -DBUILD_DESKTOP_V2=ON
  if ! cmake --build desktop-v2/build --config Release; then
    note_fail "desktop-v2 build failed"
  fi
  if ! ctest --test-dir desktop-v2/build --output-on-failure -R 'ControlCoreBridge'; then
    note_fail "desktop-v2 ControlCoreBridge_test failed"
  fi

  echo "== desktop cmake build (BUILD_VST_V2=ON) =="
  if cmake -S desktop -B desktop/build-v2-vst -DBUILD_VST_V2=ON -LAH 2>/dev/null | grep -q '^BUILD_VST_V2:BOOL'; then
  if ! cmake -S desktop -B desktop/build-v2-vst -DBUILD_VST_V2=ON; then
    note_fail "desktop BUILD_VST_V2 configure failed"
  elif ! cmake --build desktop/build-v2-vst --config Release; then
    note_fail "desktop BUILD_VST_V2 build failed"
  elif [[ -f desktop/build-v2-vst/HostParameterProcessorV2_test ]] || [[ -f desktop/build-v2-vst/Release/HostParameterProcessorV2_test.exe ]]; then
    if ! ctest --test-dir desktop/build-v2-vst --output-on-failure -R 'HostParameterProcessorV2'; then
      note_fail "desktop HostParameterProcessorV2 tests failed"
    fi
  fi
  else
    echo "SKIP: BUILD_VST_V2 option not present yet (OpenSpec §6)"
  fi
fi

echo "== git worktree cleanliness =="
collect_unexpected_porcelain | sort -u > "$after_porcelain"
new_drift="$(comm -13 "$baseline_porcelain" "$after_porcelain" || true)"
if [[ -n "$new_drift" ]]; then
  note_fail "builds introduced new tracked changes outside ignored output trees:"
  printf '%s\n' "$new_drift" >&2
fi

if [[ "$fail" -ne 0 ]]; then
  echo "verify_clean_rebuild FAILED" >&2
  exit 1
fi

echo "OK: verify_clean_rebuild (generator fresh, sim/web/desktop builds clean, worktree unchanged${VERIFY_V2:+; v2 targets verified})"
