#!/usr/bin/env bash
# Reject independent gridPx(31) mod-column placement outside DesktopV2ChromeLayout.hpp.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

APPROVED="desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp"

matches="$(rg -n 'gridPx\(31\)|kModuleRowModX|kModuleRowCenterClusterX|kCenterGlobalClusterW' desktop-v2/Source 2>/dev/null || true)"
matches="$(printf '%s\n' "$matches" | grep -vF "$APPROVED" || true)"

if [[ -n "${matches//[$'\n\r\t ']}" ]]; then
  echo "FAIL: independent layout magic constants outside DesktopV2ChromeLayout.hpp"
  printf '%s\n' "$matches"
  exit 1
fi

panel_matches="$(rg -n 'moduleRowColumns' desktop-v2/Source/ui/SubmodulePagePanel.cpp desktop-v2/Source/ui/AdsrPagePanel.cpp desktop-v2/Source/ui/PageCarouselComponent.cpp 2>/dev/null || true)"
if [[ -z "${panel_matches//[$'\n\r\t ']}" ]]; then
  echo "FAIL: panel layout code must call moduleRowColumns"
  exit 1
fi

echo "Layout-authority gate OK"
