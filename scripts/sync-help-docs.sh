#!/usr/bin/env sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cp "$ROOT/SIM_MANUAL.md" "$ROOT/web/public/sim-manual.md"
cp "$ROOT/QUICK_DICT.md" "$ROOT/web/public/quick-dict.md"
cp "$ROOT/LICENSE" "$ROOT/web/public/license.md"
echo "Synced help docs to web/public/"
