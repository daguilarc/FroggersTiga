#!/usr/bin/env bash
# Local OpenSpec hygiene for workspace planning artifacts.
#
# Modes (default PRE_CLOSURE):
#   PRE_CLOSURE  — strict validate active changes + baseline specs; fail on TBD
#                  purposes and struck-through / placeholder active tasks; WARN on
#                  unresolved duplicate active capability ownership.
#   POST_CLOSURE — same hard checks plus FAIL on any remaining active change and
#                  unresolved duplicate ownership.
#
# Usage:
#   scripts/check_openspec_hygiene.sh
#   scripts/check_openspec_hygiene.sh --post-closure
#   OPENSPEC_HYGIENE_MODE=POST_CLOSURE scripts/check_openspec_hygiene.sh
#
# Requires: openspec CLI on PATH.
# OpenSpec is local-only planning state in this workspace. This script uses
# filesystem/OpenSpec CLI checks only; it intentionally does not inspect git.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# shellcheck source=scripts/repo_path_policy.sh
source "$ROOT/scripts/repo_path_policy.sh"

MODE="${OPENSPEC_HYGIENE_MODE:-PRE_CLOSURE}"

for arg in "$@"; do
  case "$arg" in
    --pre-closure) MODE="PRE_CLOSURE" ;;
    --post-closure) MODE="POST_CLOSURE" ;;
    -h|--help)
      sed -n '1,20p' "$0" | tail -n +2
      exit 0
      ;;
    *)
      echo "Unknown option: $arg (use --pre-closure or --post-closure)" >&2
      exit 2
      ;;
  esac
done

fail=0
warn=0

note_fail() {
  echo "FAIL: $1" >&2
  fail=1
}

note_warn() {
  echo "WARN: $1" >&2
  warn=1
}

if ! command -v openspec >/dev/null 2>&1; then
  echo "FAIL: openspec CLI not found; install with: npm i -g @fission-ai/openspec" >&2
  exit 1
fi

echo "OpenSpec hygiene ($MODE)"

echo "== strict validate: baseline specs =="
if ! openspec validate --specs --strict; then
  note_fail "openspec validate --specs --strict failed"
fi

echo "== baseline Purpose placeholders =="
while IFS= read -r spec; do
  purpose="$(awk '/^## Purpose$/{found=1; next} found && /^## /{exit} found{print}' "$spec")"
  if printf '%s\n' "$purpose" | grep -qiE '\bTBD\b|^[[:space:]]*placeholder[[:space:]]*$'; then
    note_fail "baseline Purpose still placeholder: $spec"
  fi
done < <(find openspec/specs -mindepth 2 -maxdepth 2 -name spec.md | sort)

check_tasks_file() {
  local tasks_file="$1"
  local label="$2"
  local severity="$3"

  [[ -f "$tasks_file" ]] || return 0

  while IFS= read -r line; do
    if [[ "$line" =~ ^-[[:space:]]*\[[[:space:][:alnum:]_xX-]*\] ]]; then
      if [[ "$line" == *'~~'* ]]; then
        if [[ "$severity" == fail ]]; then
          note_fail "struck-through task in $label: $line"
        else
          note_warn "struck-through task in $label: $line"
        fi
      fi
      if [[ "$line" =~ (^|[^[:alnum:]_])(TBD|PLACEHOLDER|FIXME)([^[:alnum:]_]|$) ]]; then
        if [[ "$severity" == fail ]]; then
          note_fail "placeholder task in $label: $line"
        else
          note_warn "placeholder task in $label: $line"
        fi
      fi
    fi
  done < "$tasks_file"
}

echo "== active change inventory =="
if ! active_json="$(openspec list --json 2>/dev/null)"; then
  note_fail "openspec list --json failed"
  active_json='{"changes":[]}'
fi

active_changes=()
while IFS= read -r change; do
  [[ -n "$change" ]] && active_changes+=("$change")
done < <(printf '%s' "$active_json" | python3 -c '
import json, sys
data = json.load(sys.stdin)
for c in data.get("changes", []):
    print(c["name"])
')

if ((${#active_changes[@]} == 0)); then
  echo "No active OpenSpec changes"
fi

# macOS ships bash 3.2, which treats "${arr[@]}" on a ZERO-LENGTH array as an unbound
# variable under `set -u`. Fired for the first time on 2026-08-10, when this repo first
# reached zero active changes. The ${arr[@]+"${arr[@]}"} idiom expands to nothing when the
# array is empty instead of erroring.
for change in ${active_changes[@]+"${active_changes[@]}"}; do
  echo "== strict validate: $change =="
  if ! openspec validate "$change" --strict; then
    note_fail "openspec validate --strict failed for $change"
  fi
  check_tasks_file "openspec/changes/$change/tasks.md" "$change tasks.md" fail
done

if [[ "$MODE" == POST_CLOSURE ]]; then
  if ((${#active_changes[@]} > 0)); then
    note_fail "active OpenSpec changes remain (${#active_changes[@]}): ${active_changes[*]}"
  fi
fi

echo "== OpenSpec path classes =="
for path in openspec/.cache/state.json openspec/.sessions/current.json openspec/changes/example/.cache/state.json; do
  if ! repo_policy_is_openspec_ephemeral "$path"; then
    note_fail "OpenSpec ephemeral path is not classified by repo_path_policy.sh: $path"
  fi
done

echo "== duplicate capability ownership =="
ownership_tmp="$(mktemp)"
trap 'rm -f "$ownership_tmp"' EXIT

# macOS ships bash 3.2, which treats "${arr[@]}" on a ZERO-LENGTH array as an unbound
# variable under `set -u`. Fired for the first time on 2026-08-10, when this repo first
# reached zero active changes. The ${arr[@]+"${arr[@]}"} idiom expands to nothing when the
# array is empty instead of erroring.
for change in ${active_changes[@]+"${active_changes[@]}"}; do
  specs_dir="openspec/changes/$change/specs"
  [[ -d "$specs_dir" ]] || continue
  for cap_dir in "$specs_dir"/*; do
    [[ -d "$cap_dir" ]] || continue
    cap="$(basename "$cap_dir")"
    printf '%s\t%s\n' "$cap" "$change" >> "$ownership_tmp"
  done
done

while IFS=$'\t' read -r cap owner_a owner_b; do
  [[ -n "$cap" ]] || continue
  msg="capability '$cap' owned by both '$owner_a' and '$owner_b' without documented handoff"
  if [[ "$MODE" == POST_CLOSURE ]]; then
    note_fail "$msg"
  else
    note_warn "$msg (resolve before POST_CLOSURE)"
  fi
done < <(awk -F'\t' '
{
  cap = $1
  change = $2
  n = ++count[cap]
  owners[cap, n] = change
}
END {
  for (cap in count) {
    if (count[cap] <= 1) continue
    for (i = 1; i <= count[cap]; i++) {
      for (j = i + 1; j <= count[cap]; j++) {
        print cap "\t" owners[cap, i] "\t" owners[cap, j]
      }
    }
  }
}' "$ownership_tmp" | sort -u)

if [[ "$fail" -ne 0 ]]; then
  echo "OpenSpec hygiene FAILED ($MODE)" >&2
  exit 1
fi

if [[ "$warn" -ne 0 ]]; then
  echo "OK: OpenSpec hygiene passed with warnings ($MODE)"
  exit 0
fi

echo "OK: OpenSpec hygiene ($MODE)"
