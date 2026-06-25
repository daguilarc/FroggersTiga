#!/usr/bin/env bash
# Reject tracked host build outputs and blanket OpenSpec ignores.
# Firmware, vendored, and docs/ publication paths are out of scope.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# shellcheck source=scripts/repo_path_policy.sh
source "$ROOT/scripts/repo_path_policy.sh"

fail=0
GITIGNORE="$ROOT/.gitignore"

if [[ ! -f "$GITIGNORE" ]]; then
  echo "FAIL: missing .gitignore" >&2
  exit 1
fi

if grep -E '^openspec/?$' "$GITIGNORE"; then
  echo "FAIL: .gitignore has blanket openspec/ ignore; use selective ephemeral cache rules" >&2
  fail=1
fi

for pattern in "${REPO_REQUIRED_IGNORES[@]}"; do
  if ! grep -qxF "$pattern" "$GITIGNORE"; then
    echo "FAIL: .gitignore missing required ignore: $pattern" >&2
    fail=1
  fi
done

is_excluded() {
  repo_policy_is_firmware_excluded "$1" || repo_policy_is_publication_output "$1"
}

while IFS= read -r tracked; do
  [[ -z "$tracked" ]] && continue
  if is_excluded "$tracked"; then
    continue
  fi
  if repo_policy_is_prohibited_public_artifact "$tracked"; then
    echo "FAIL: prohibited tracked host artifact: $tracked" >&2
    fail=1
  fi
done < <(git ls-files)

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

echo "OK: host artifact hygiene"
