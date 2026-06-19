#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERIFY="$SCRIPT_DIR/verify-tag-version.sh"

pass=0
fail=0

assert_accepts() {
  local tag="$1"
  if "$VERIFY" "$tag" >/dev/null 2>&1; then
    pass=$((pass + 1))
  else
    echo "expected accept: $tag" >&2
    fail=$((fail + 1))
  fi
}

assert_rejects() {
  local tag="$1"
  if "$VERIFY" "$tag" >/dev/null 2>&1; then
    echo "expected reject: $tag" >&2
    fail=$((fail + 1))
  else
    pass=$((pass + 1))
  fi
}

assert_accepts "froggerstiga-v1"
assert_rejects "froggerstiga-v2"
assert_rejects "froggerstiga-v999"
assert_rejects "desktop-v1"
assert_rejects "froggerstiga-v1-extra"
assert_rejects "main"

if [[ "$fail" -ne 0 ]]; then
  echo "verify-tag-version tests failed: $fail failure(s), $pass pass(es)" >&2
  exit 1
fi

echo "verify-tag-version tests ok ($pass cases)"
