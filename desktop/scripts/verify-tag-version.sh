#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TAG="${1:-${GITHUB_REF_NAME:-}}"
CANONICAL_TAG="froggerstiga-v1"

if [[ -z "$TAG" ]]; then
  echo "error: pass release tag as first argument or set GITHUB_REF_NAME" >&2
  exit 1
fi

CMAKE_VERSION="$("$SCRIPT_DIR/read-version.sh")"

if [[ "$TAG" == "$CANONICAL_TAG" ]]; then
  echo "release channel tag ok: $CANONICAL_TAG (CMake VERSION $CMAKE_VERSION)"
else
  echo "error: expected exact release channel tag $CANONICAL_TAG, got: $TAG" >&2
  exit 1
fi

echo "version ok: $CMAKE_VERSION"
