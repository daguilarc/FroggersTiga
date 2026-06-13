#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CMAKE_FILE="${SCRIPT_DIR}/../CMakeLists.txt"

if [[ ! -f "$CMAKE_FILE" ]]; then
  echo "error: CMakeLists.txt not found at $CMAKE_FILE" >&2
  exit 1
fi

VERSION="$(grep -E '^project\(FroggersTigaDesktop VERSION [0-9]+\.[0-9]+\.[0-9]+' "$CMAKE_FILE" \
  | head -n 1 \
  | sed -E 's/.*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/')"

if [[ -z "$VERSION" ]]; then
  echo "error: could not parse VERSION from $CMAKE_FILE" >&2
  exit 1
fi

echo "$VERSION"
