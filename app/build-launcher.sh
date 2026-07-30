#!/usr/bin/env bash
# Builds the sheaf-patch launcher with the Froggers app injected out-of-tree.
#
# EXTRA_APP_HEADERS must list EVERY header the app compiles: the sheaf-patch
# Makefile uses them as literal prerequisites (Makefile:47-48) and generates no
# -MMD/-MP dependency files, so an unlisted header is untracked and edits to it
# produce a build that succeeds while silently ignoring the change
# (design.md:188). Globbing keeps that list from rotting -- a hand-written list
# tracked 4 of 18 headers, missing all of app/dsp/.
set -euo pipefail

cd "$(dirname "$0")/.."
REPO_ROOT="$PWD"

APP_HEADERS="$(ls "$REPO_ROOT"/app/*.hpp "$REPO_ROOT"/app/dsp/*.hpp | tr '\n' ' ')"

# -j2 + nice: 8-core / 16 GB machine, higher parallelism freezes it.
nice make -j2 -C External/Sheaf/projects/synth/apps/sheaf-patch \
  EXTRA_APP_DIR="$REPO_ROOT/app" \
  EXTRA_APP_HEADER=FroggersRegistration.hpp \
  EXTRA_APP_TYPE=synth_froggers::FroggersApp \
  EXTRA_APP_REGISTRAR=synth_froggers::MakeFroggersRegistration \
  EXTRA_APP_HEADERS="$APP_HEADERS" \
  "$@"
