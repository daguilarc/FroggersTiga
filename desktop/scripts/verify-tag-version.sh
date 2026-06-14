#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TAG="${1:-${GITHUB_REF_NAME:-}}"

if [[ -z "$TAG" ]]; then
  echo "error: pass release tag as first argument or set GITHUB_REF_NAME" >&2
  exit 1
fi

CMAKE_VERSION="$("$SCRIPT_DIR/read-version.sh")"

if [[ "$TAG" == desktop-v* ]]; then
  TAG_VERSION="${TAG#desktop-v}"
  if [[ "$TAG_VERSION" != "$CMAKE_VERSION" ]]; then
    echo "error: tag version ($TAG_VERSION) != CMake VERSION ($CMAKE_VERSION)" >&2
    echo "Bump project(FroggersTigaDesktop VERSION ...) in desktop/CMakeLists.txt before tagging." >&2
    exit 1
  fi
elif [[ "$TAG" == froggerstiga-v* ]]; then
  echo "froggerstiga release tag: CMake VERSION $CMAKE_VERSION"
else
  echo "error: expected tag desktop-vX.Y.Z or froggerstiga-v*, got: $TAG" >&2
  exit 1
fi

echo "version ok: $CMAKE_VERSION"
