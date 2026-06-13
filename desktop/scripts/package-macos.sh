#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DESKTOP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${DESKTOP_DIR}/build"
ARTEFACTS_DIR="${BUILD_DIR}/FroggersTigaDesktop_artefacts"
DIST_DIR="${DESKTOP_DIR}/dist"

VERSION="$("$SCRIPT_DIR/read-version.sh")"
APP_NAME="FroggersTiga.app"
RELEASE_APP="${ARTEFACTS_DIR}/Release/${APP_NAME}"
FALLBACK_APP="${ARTEFACTS_DIR}/${APP_NAME}"

if [[ -d "$RELEASE_APP" ]]; then
  SOURCE_APP="$RELEASE_APP"
elif [[ -d "$FALLBACK_APP" ]]; then
  SOURCE_APP="$FALLBACK_APP"
else
  echo "error: Release build not found." >&2
  echo "  expected: ${RELEASE_APP}" >&2
  echo "  fallback: ${FALLBACK_APP}" >&2
  echo "Run: cd desktop && cmake -B build && cmake --build build --config Release" >&2
  exit 1
fi

OUTPUT_DMG="${DIST_DIR}/FroggersTiga-${VERSION}-macOS.dmg"
STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGE_DIR"' EXIT

mkdir -p "$DIST_DIR"
cp -R "$SOURCE_APP" "${STAGE_DIR}/${APP_NAME}"
ln -s /Applications "${STAGE_DIR}/Applications"

rm -f "$OUTPUT_DMG"
hdiutil create -volname "FroggersTiga ${VERSION}" -srcfolder "$STAGE_DIR" -ov -format UDZO "$OUTPUT_DMG"

echo "created: $OUTPUT_DMG"
