#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG="$ROOT/src/mk/config.mk"

fail() {
  echo "toolchain parity check FAILED: $*" >&2
  exit 1
}

grep -q 'APP_TYPE := BOOT_NONE' "$CONFIG" || fail 'APP_TYPE must be BOOT_NONE'
grep -q 'OPT_LEVEL ?= -Os' "$CONFIG" || fail 'OPT_LEVEL must be -Os'
grep -q 'USE_LTO ?= 1' "$CONFIG" || fail 'USE_LTO must default to 1'
grep -q '14.3.rel1' "$CONFIG" || fail 'toolchain must reference Arm GNU 14.3.rel1'

echo "toolchain parity check OK (BOOT_NONE, -Os, USE_LTO=1, Arm 14.3.rel1)"
