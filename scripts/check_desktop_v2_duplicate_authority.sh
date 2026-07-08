#!/usr/bin/env bash
# Reject duplicate-authority definitions outside approved manifest/generated/test files.
# This is a local grep gate: it intentionally reports current drift in the legacy v2
# desktop and sim/core source surfaces so the convergence packet can prove the gap.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

APPROVED_PATHS=(
  "desktop-v2/Source/manifest/FroggersV2AppManifest.hpp"
  "desktop-v2/Source/control/MidiCvAssignmentTable.hpp"
  "desktop-v2/Source/control/MidiCvAssignmentTable.cpp"
  "desktop-v2/tests/FroggersV2Manifest_test.cpp"
  "desktop-v2/tests/FroggersV2ProjectionValidators_test.cpp"
  "desktop-v2/tools/FroggersV2ManifestSnapshot.cpp"
)

scan_category() {
  local label="$1"
  local pattern="$2"

  local matches
  matches="$(rg -n -e "$pattern" desktop-v2/Source sim src/core src/common desktop-v2/tools desktop-v2/tests 2>/dev/null || true)"
  for approved in "${APPROVED_PATHS[@]}"; do
    matches="$(printf '%s\n' "$matches" | grep -vF "$approved" || true)"
  done

  if [[ -n "${matches//[$'\n\r\t ']}" ]]; then
    echo "FAIL: $label"
    printf '%s\n' "$matches"
    return 1
  fi

  echo "OK: $label"
}

fail=0

scan_category "independent row labels and stable host IDs" 'stableIdStorage|buildStableId\(|kTable\[kNumHostPages\]|kPages\[kNumHostPages\]|kPairArRows|kExpansionTail' || fail=1
scan_category "desktop MIDI/controller target IDs" 'MidiCvButtonTarget|manifestTargetIdForButton|"midi_pitch"|"midi_gate"|"midi_shift_button"|"midi_scene_1"|"midi_scene_2"|"midi_scene_3"|"midi_qwerty_virtual"|"midi_external_clock"' || fail=1
scan_category "manifest-owned modulation eligibility tables" 'kNumModSources = 10|kModSourceMidiCcA|kModSourceMidiCcB|V2ModSourceLabel|V2ModTapBank::kNumTaps|IsV2ModSourceIndex|PickSimRandomModIndex' || fail=1
scan_category "sequencer field inventories" 'kMaxSteps = 64|m_patternLength|PatternLength|sequencer_pattern_length|sequencer_playhead|heldGestures|m_shiftHeld' || fail=1
scan_category "old v2 source catalog UI authority" 'V2ModSourceLabel|ModSourceCell|V2ModTapBank' || fail=1
scan_category "rejected variable-step-count controls" 'PatternLength|m_patternLength|stepsLabel|m_stepsLabel' || fail=1
scan_category "held-gesture state" 'm_shiftHeld|heldGestures|shiftHeld' || fail=1

if [[ "$fail" -ne 0 ]]; then
  echo "Duplicate-authority gate FAILED"
  exit 1
fi

echo "Duplicate-authority gate OK"
