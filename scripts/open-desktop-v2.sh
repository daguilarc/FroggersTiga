#!/usr/bin/env bash
# Open the desktop v2 Release .app from repository root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="${ROOT}/desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app"

if [[ ! -d "$APP" ]]; then
  echo "FAIL: Release FroggersTigaV2.app not found at:" >&2
  echo "  $APP" >&2
  echo "Build it first:" >&2
  echo "  cmake -S desktop-v2 -B desktop-v2/build -DBUILD_DESKTOP_V2=ON" >&2
  echo "  cmake --build desktop-v2/build --config Release" >&2
  exit 1
fi

open "$APP"
