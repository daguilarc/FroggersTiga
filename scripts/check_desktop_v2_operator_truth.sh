#!/usr/bin/env bash
# Operator-truth gate: rejects the three known desktop-v2 wiring defects
# described in openspec/changes/desktop-v2-operator-truth-repair.
#
#   (a) Rand Mods routes to the per-sequencer-step message instead of the
#       global mod-depth message, in GlobalStripV2.cpp and/or
#       CenterGlobalClusterV2.cpp.
#   (b) FroggersV2HostBridge::syncToHost only pushes the single active page
#       to the host instead of every page.
#   (c) DesktopV2ChromeLayout.hpp lets moduleRowColumns().modW expand past
#       kModCellW instead of being capped at it. (ADVISORY ONLY — the
#       authoritative gate for this defect is the LayoutBounds_test binary;
#       this grep is a cheap early signal, not the source of truth.)
#
# EXPECTED BASELINE BEHAVIOR: at the desktop-v2-operator-truth-repair
# baseline, all three defects are present, so running this script is
# EXPECTED to exit nonzero. Packets 1 (modW cap), 3 (host sync), and 4
# (randomization authority) repair these one at a time; this script should
# turn fully green once all three land.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail=0

# ---------------------------------------------------------------------------
# (a) Rand Mods wiring bug
# ---------------------------------------------------------------------------
# Extract the body of a ClassName::pushRandMods() function (from its
# definition line through the function's closing brace, which is the first
# unindented "}" that follows) and report every line inside that body that
# still routes to RandSequencerMods instead of the global-mods message.
check_rand_mods_wiring() {
  local file="$1"
  local qualifier="$2"

  if [[ ! -f "$file" ]]; then
    echo "SKIP: ${qualifier}pushRandMods (file absent: $file)"
    return 0
  fi

  local body
  body="$(awk -v q="${qualifier}pushRandMods" '
    $0 ~ q { in_func = 1 }
    in_func { print NR ":" $0 }
    in_func && /^}[[:space:]]*$/ { in_func = 0 }
  ' "$file")"

  if [[ -z "${body//[$'\n\r\t ']}" ]]; then
    echo "FAIL: could not locate ${qualifier}pushRandMods in $file"
    return 1
  fi

  local bad
  bad="$(printf '%s\n' "$body" | grep -E 'type[[:space:]]*=.*RandSequencerMods' || true)"

  if [[ -n "${bad//[$'\n\r\t ']}" ]]; then
    echo "FAIL: ${qualifier}pushRandMods still routes to RandSequencerMods (should push a global-mods message)"
    printf '%s\n' "$bad" | while IFS=: read -r lineno rest; do
      echo "  ${file}:${lineno}: ${rest}"
    done
    return 1
  fi

  echo "PASS: ${qualifier}pushRandMods no longer routes to RandSequencerMods ($file)"
  return 0
}

echo "== GATE: rand-mods-wiring (GlobalStripV2) =="
if ! check_rand_mods_wiring "desktop-v2/Source/ui/GlobalStripV2.cpp" "GlobalStripV2::"; then
  fail=1
fi
echo

echo "== GATE: rand-mods-wiring (CenterGlobalClusterV2) =="
if ! check_rand_mods_wiring "desktop-v2/Source/ui/CenterGlobalClusterV2.cpp" "CenterGlobalClusterV2::"; then
  fail=1
fi
echo

# ---------------------------------------------------------------------------
# (b) Host-sync partial bug
# ---------------------------------------------------------------------------
# Extract the body of FroggersV2HostBridge::syncToHost and confirm it loops
# across every host page (kNumHostPages), the same pattern already used by
# syncAllModRoutesToHost/syncFromHostModRoutes in this file. A body that
# reads the single m_core.activePage() without ever iterating kNumHostPages
# is the partial-sync bug.
echo "== GATE: host-sync-full-pages =="
HOST_BRIDGE="desktop-v2/Source/control/FroggersV2HostBridge.cpp"
if [[ ! -f "$HOST_BRIDGE" ]]; then
  echo "FAIL: expected file missing: $HOST_BRIDGE"
  fail=1
else
  sync_body="$(awk '
    /FroggersV2HostBridge::syncToHost[[:space:]]*\(/ { in_func = 1 }
    in_func { print NR ":" $0 }
    in_func && /^}[[:space:]]*$/ { in_func = 0 }
  ' "$HOST_BRIDGE")"

  if [[ -z "${sync_body//[$'\n\r\t ']}" ]]; then
    echo "FAIL: could not locate FroggersV2HostBridge::syncToHost in $HOST_BRIDGE"
    fail=1
  else
    single_page_line="$(printf '%s\n' "$sync_body" | grep -E 'activePage[[:space:]]*\(' || true)"
    all_pages_line="$(printf '%s\n' "$sync_body" | grep -E 'kNumHostPages' || true)"

    if [[ -n "${single_page_line//[$'\n\r\t ']}" && -z "${all_pages_line//[$'\n\r\t ']}" ]]; then
      echo "FAIL: syncToHost still syncs only the active page, not all pages"
      printf '%s\n' "$single_page_line" | while IFS=: read -r lineno rest; do
        echo "  ${HOST_BRIDGE}:${lineno}: ${rest}"
      done
      fail=1
    else
      echo "PASS: syncToHost loops across all pages (kNumHostPages) ($HOST_BRIDGE)"
    fi
  fi
fi
echo

# ---------------------------------------------------------------------------
# (c) Mod-cell width bug — ADVISORY ONLY
# ---------------------------------------------------------------------------
# LayoutBounds_test is the authoritative gate for modW correctness. This grep
# is a cheap, fast, advisory early-warning check on the plan's known bad
# pattern: capping modW with std::max(kModCellW, ...) instead of std::min, or
# not capping it at all. It does not fail the overall gate.
echo "== GATE: mod-cell-width (advisory; LayoutBounds_test is authoritative) =="
LAYOUT_FILE="desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp"
if [[ ! -f "$LAYOUT_FILE" ]]; then
  echo "ADVISORY-FAIL: expected file missing: $LAYOUT_FILE"
else
  bad_modw="$(grep -nE 'modW[[:space:]]*=[[:space:]]*std::max\([[:space:]]*kModCellW' "$LAYOUT_FILE" || true)"
  if [[ -n "${bad_modw//[$'\n\r\t ']}" ]]; then
    echo "ADVISORY-FAIL: modW is allowed to expand past kModCellW (std::max cap instead of a min/fixed cap)"
    printf '%s\n' "$bad_modw" | while IFS=: read -r lineno rest; do
      echo "  ${LAYOUT_FILE}:${lineno}: ${rest}"
    done
  else
    echo "ADVISORY-PASS: no std::max(kModCellW, ...) expansion pattern found for modW ($LAYOUT_FILE)"
  fi
fi
echo

if [[ "${fail}" -ne 0 ]]; then
  echo "Operator-truth gate FAILED (see above; modW check above is advisory-only and does not affect this exit code)"
  exit 1
fi

echo "Operator-truth gate OK"
