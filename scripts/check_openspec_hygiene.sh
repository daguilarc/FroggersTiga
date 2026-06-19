#!/usr/bin/env bash
# Host-scoped OpenSpec hygiene for omni-repository-harmonization (task 9.1).
#
# Modes (default PRE_CLOSURE):
#   PRE_CLOSURE  — strict validate omni + baseline specs; fail on TBD purposes and
#                  struck-through / placeholder tasks in omni; WARN on other active
#                  changes and unresolved duplicate capability ownership.
#   POST_CLOSURE — same hard checks plus FAIL on any non-omni active change,
#                  unresolved duplicate ownership, and struck-through tasks in any
#                  in-scope active change.
#
# Usage:
#   scripts/check_openspec_hygiene.sh
#   scripts/check_openspec_hygiene.sh --post-closure
#   OPENSPEC_HYGIENE_MODE=POST_CLOSURE scripts/check_openspec_hygiene.sh
#
# Requires: openspec CLI on PATH (npm i -g @fission-ai/openspec).
# Pages CI skips when openspec is absent; run locally before OpenSpec closure (9.2+).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OMNI_CHANGE="omni-repository-harmonization"
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

echo "== strict validate: $OMNI_CHANGE =="
if ! openspec validate "$OMNI_CHANGE" --strict; then
  note_fail "openspec validate --strict failed for $OMNI_CHANGE"
fi

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
      if [[ "$line" =~ (^|[[:space:][:punct:]])(TBD|PLACEHOLDER|FIXME)[[:space:][:punct:]|$ ]]; then
        if [[ "$severity" == fail ]]; then
          note_fail "placeholder task in $label: $line"
        else
          note_warn "placeholder task in $label: $line"
        fi
      fi
    fi
  done < "$tasks_file"
}

omni_tasks="openspec/changes/$OMNI_CHANGE/tasks.md"
check_tasks_file "$omni_tasks" "$OMNI_CHANGE tasks.md" fail

has_documented_handoff() {
  local change_a="$1"
  local change_b="$2"
  local dir_a="openspec/changes/$change_a"
  local dir_b="openspec/changes/$change_b"
  local f

  if [[ "$change_a" == "$OMNI_CHANGE" ]] && grep -qF "$change_b" "openspec/changes/$OMNI_CHANGE/disposition.md" 2>/dev/null; then
    return 0
  fi
  if [[ "$change_b" == "$OMNI_CHANGE" ]] && grep -qF "$change_a" "openspec/changes/$OMNI_CHANGE/disposition.md" 2>/dev/null; then
    return 0
  fi

  for f in "$dir_a"/{proposal,design,tasks,disposition}.md "$dir_b"/{proposal,design,tasks,disposition}.md; do
    [[ -f "$f" ]] || continue
    if grep -qiE 'handoff|supersed|reconciled \(omni|absorbed by omni' "$f" 2>/dev/null \
      && grep -qF "$change_a" "$f" 2>/dev/null \
      && grep -qF "$change_b" "$f" 2>/dev/null; then
      return 0
    fi
  done

  for f in "$dir_a"/tasks.md "$dir_b"/tasks.md; do
    [[ -f "$f" ]] || continue
    if head -n 3 "$f" | grep -qiE 'reconciled \(omni|handoff|supersed'; then
      return 0
    fi
  done

  return 1
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

other_changes=()
for change in "${active_changes[@]}"; do
  [[ "$change" == "$OMNI_CHANGE" ]] && continue
  other_changes+=("$change")
done

if ((${#other_changes[@]} > 0)); then
  if [[ "$MODE" == POST_CLOSURE ]]; then
    note_fail "non-omni active changes remain (${#other_changes[@]}): ${other_changes[*]}"
  else
    note_warn "non-omni active changes (${#other_changes[@]}): ${other_changes[*]} (expected until task 9.2+ closure)"
  fi
fi

if [[ "$MODE" == POST_CLOSURE ]]; then
  for change in "${active_changes[@]}"; do
    [[ "$change" == "$OMNI_CHANGE" ]] && continue
    check_tasks_file "openspec/changes/$change/tasks.md" "$change tasks.md" fail
  done
fi

echo "== duplicate capability ownership =="
ownership_tmp="$(mktemp)"
trap 'rm -f "$ownership_tmp"' EXIT

for change in "${active_changes[@]}"; do
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
  if has_documented_handoff "$owner_a" "$owner_b"; then
    continue
  fi
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
