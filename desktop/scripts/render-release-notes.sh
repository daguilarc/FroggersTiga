#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TAG="${1:-froggerstiga-v1}"
VERSION="$("$SCRIPT_DIR/read-version.sh")"
MANUAL="$ROOT/SIM_MANUAL.md"

if [[ ! -f "$MANUAL" ]]; then
  echo "error: missing $MANUAL" >&2
  exit 1
fi

printf 'Desktop app for FroggersTiga (release channel `%s`, package version **v%s**).\n\n' "$TAG" "$VERSION"
printf 'Download **FroggersTiga.dmg** (macOS) or **FroggersTiga-Setup.exe** (Windows).\n\n'
printf 'Web sim: https://daguilarc.github.io/frogg3rs/\n\n'
printf '%s\n\n' '---'
sed -n '/^## Version history$/,$p' "$MANUAL"
