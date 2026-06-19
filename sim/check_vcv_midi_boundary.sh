#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VCV_SRC="$ROOT/vcv/src"

if [[ ! -d "$VCV_SRC" ]]; then
  echo "skip: vcv/src not present"
  exit 0
fi

PATTERN='midi::|MidiButton|MIDI CC|CC1_ENABLE|CC2_ENABLE|PushMidiCc|tickMidiOut|enqueueVcvMidi|CvMidiBridge'
if rg -n "$PATTERN" "$VCV_SRC" >/dev/null 2>&1; then
  echo "error: Froggers-owned MIDI boundary remains in vcv/src:" >&2
  rg -n "$PATTERN" "$VCV_SRC" >&2
  exit 1
fi

echo "check_vcv_midi_boundary: OK (no MIDI/CC boundary in vcv/src)"
