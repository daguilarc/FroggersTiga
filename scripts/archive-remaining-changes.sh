#!/usr/bin/env bash
# Finish archive-cleanup §4 for changes blocked by MODIFIED-delta validation.
# Run from repo root after desktop-header-hit-test manual verify (3.2).
set -euo pipefail
cd "$(dirname "$0")/.."

ARCHIVE=(desktop-vco-morph-fix sim-hosts-multi-ui desktop-sim-ux-polish desktop-host-corrections
  desktop-host-mutation-safety desktop-midi-input-clarity desktop-qwerty-midi-pitch-cv
  web-sim-page-ux desktop-chrome-cohesion)

for c in "${ARCHIVE[@]}"; do
  echo "Archiving ${c} (skip-specs)..."
  openspec archive "${c}" -y --skip-specs
done

openspec archive archive-cleanup -y --skip-specs
openspec list
openspec validate
