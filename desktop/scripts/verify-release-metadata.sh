#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
CMAKE_VERSION="$("$SCRIPT_DIR/read-version.sh")"
errors=0

fail() {
  echo "error: $1" >&2
  errors=$((errors + 1))
}

json_version() {
  node -e "const p=require(process.argv[1]); process.stdout.write(p.version||'')" "$1"
}

MAIN_CPP="$REPO_ROOT/desktop/Source/Main.cpp"
if ! grep -q 'JUCE_APPLICATION_VERSION_STRING' "$MAIN_CPP"; then
  fail "desktop/Source/Main.cpp must return JUCE_APPLICATION_VERSION_STRING"
fi
if grep -q 'return "1\.0\.0"' "$MAIN_CPP"; then
  fail "desktop/Source/Main.cpp still hardcodes application version literal"
fi

WEB_PKG="$REPO_ROOT/web/package.json"
WEB_LOCK="$REPO_ROOT/web/package-lock.json"
WEB_VERSION="$(json_version "$WEB_PKG")"
LOCK_VERSION="$(node -e "const l=require(process.argv[1]); process.stdout.write(l.version||'')" "$WEB_LOCK")"
if [[ "$WEB_VERSION" != "$CMAKE_VERSION" ]]; then
  fail "web/package.json version $WEB_VERSION != CMake $CMAKE_VERSION"
fi
if [[ "$LOCK_VERSION" != "$CMAKE_VERSION" ]]; then
  fail "web/package-lock.json root version $LOCK_VERSION != CMake $CMAKE_VERSION"
fi

README="$REPO_ROOT/README.md"
SIM_MANUAL="$REPO_ROOT/SIM_MANUAL.md"
RELEASE_HEADING="**Release v${CMAKE_VERSION}**"
if ! grep -qF "$RELEASE_HEADING" "$README"; then
  fail "README.md missing current-release heading: $RELEASE_HEADING"
fi
if ! grep -qF "$RELEASE_HEADING" "$SIM_MANUAL"; then
  fail "SIM_MANUAL.md missing current-release heading: $RELEASE_HEADING"
fi

if [[ "$errors" -ne 0 ]]; then
  echo "verify-release-metadata failed with $errors error(s)" >&2
  exit 1
fi

echo "release metadata ok: CMake $CMAKE_VERSION, web $WEB_VERSION, docs $RELEASE_HEADING"
