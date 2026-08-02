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
#
# APP_NAME/APP_SOURCES/APP_BUILD_DIR/APP_INFO_PLIST are make command-line
# overrides, which beat the sheaf-patch Makefile's `:=` assignments -- they
# point the build at our direct-launch entry point (app/FroggersMain.cpp)
# instead of the picker (Main.cpp). APP_BUILD_DIR also moves our build
# artifacts out of the External/Sheaf submodule and into our own tree.
#
# APP_INFO_PLIST must be overridden TOGETHER WITH APP_NAME: juce_build.mk:154
# copies the plist verbatim with no templating, so leaving sheaf-patch's plist
# in place ships a bundle whose CFBundleExecutable (SheafPatch) names a binary
# that does not exist (Frogg3rs). A Finder double-click then fails while
# running the binary directly still works, so the breakage hides from any
# build-exit-code check.
nice make -j2 -C External/Sheaf/projects/synth/apps/sheaf-patch \
  EXTRA_APP_DIR="$REPO_ROOT/app" \
  EXTRA_APP_HEADER=FroggersRegistration.hpp \
  EXTRA_APP_TYPE=synth_froggers::FroggersApp \
  EXTRA_APP_REGISTRAR=synth_froggers::MakeFroggersRegistration \
  EXTRA_APP_HEADERS="$APP_HEADERS" \
  APP_NAME=Frogg3rs \
  APP_SOURCES="$REPO_ROOT/app/FroggersMain.cpp" \
  APP_BUILD_DIR="$REPO_ROOT/app/build-launcher" \
  APP_INFO_PLIST="$REPO_ROOT/app/Frogg3rs-Info.plist" \
  "$@"
