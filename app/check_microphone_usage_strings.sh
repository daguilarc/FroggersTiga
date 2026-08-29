#!/usr/bin/env bash
# The microphone usage description is one sentence the operator reads, and it
# is declared twice because the two platforms ship from two build systems that
# share no substrate:
#
#   - Windows builds from app/standalone/CMakeLists.txt, where the string is a
#     JUCE argument (MICROPHONE_PERMISSION_TEXT) that JUCE's own plist
#     generator turns into the key.
#   - macOS builds from app/build-launcher.sh, which drives Sheaf's
#     juce_build.mk. That Makefile has no plist-generation step at all -- it
#     copies APP_INFO_PLIST verbatim -- so the key has to be written out in
#     app/Frogg3rs-Info.plist by hand.
#
# Neither can read the other's literal, so the duplication cannot be collapsed.
# What it can have is a check that fails when the two drift, which is the same
# standard the browser ABI version is held to.
set -euo pipefail

APP_DIR="${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
CMAKE_FILE="$APP_DIR/standalone/CMakeLists.txt"
PLIST_FILE="$APP_DIR/Frogg3rs-Info.plist"

fail() { echo "check-microphone-usage: $*" >&2; exit 1; }

[ -f "$CMAKE_FILE" ] || fail "missing $CMAKE_FILE"
[ -f "$PLIST_FILE" ] || fail "missing $PLIST_FILE"

cmake_text="$(sed -n 's/.*MICROPHONE_PERMISSION_TEXT[[:space:]]*"\(.*\)".*/\1/p' "$CMAKE_FILE" | head -1)"
[ -n "$cmake_text" ] || fail "no MICROPHONE_PERMISSION_TEXT in $CMAKE_FILE -- has the Windows build stopped declaring microphone use?"

plist_text="$(plutil -extract NSMicrophoneUsageDescription raw -o - "$PLIST_FILE" 2>/dev/null || true)"
[ -n "$plist_text" ] || fail "no NSMicrophoneUsageDescription in $PLIST_FILE -- macOS shows no prompt without it, and this app opens an input device"

if [ "$cmake_text" != "$plist_text" ]; then
    fail "the two microphone usage strings have drifted:
  $CMAKE_FILE: $cmake_text
  $PLIST_FILE: $plist_text"
fi

# A declaration on a build path is not a declaration in the artifact. The macOS
# bundle is assembled by Sheaf's $(APP_BUNDLE) rule, which copies APP_INFO_PLIST
# into Contents/Info.plist -- so the key is read back out of a bundle that rule
# actually produced, not out of the file handed to it.
#
# Driven with a stub binary and every source prerequisite overridden to empty,
# so the real rule runs without a JUCE compile. This is the same shape Sheaf's
# own scripts/check_app_bundle_plist.sh uses against the same rule.
JUCE_BUILD_MK="$APP_DIR/../External/Sheaf/projects/synth/runtime/juce_build.mk"
[ -f "$JUCE_BUILD_MK" ] || fail "missing $JUCE_BUILD_MK -- the macOS bundle rule cannot be exercised"

APP_NAME="$(plutil -extract CFBundleExecutable raw -o - "$PLIST_FILE")"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT
mkdir -p "$TMP_DIR/build"
printf '#!/bin/sh\necho stub\n' > "$TMP_DIR/build/$APP_NAME"
chmod +x "$TMP_DIR/build/$APP_NAME"

if ! bundle_output="$(make -f "$JUCE_BUILD_MK" "$TMP_DIR/build/$APP_NAME.app" \
    APP_NAME="$APP_NAME" \
    APP_BUILD_DIR="$TMP_DIR/build" \
    APP_INFO_PLIST="$PLIST_FILE" \
    APP_SOURCES= SYNTH_SRC= SYNTH_RUNTIME_SRC= SYNTH_HEADERS= \
    SYNTH_JUCE_HEADERS= JUCE_MODULE_SRC= JUCE_MODULE_OBJ= \
    JUCE_C_MODULE_SRC= JUCE_C_MODULE_OBJ= 2>&1)"; then
    fail "the macOS bundle rule failed, so nothing can be read back from a bundle:
$bundle_output"
fi

BUILT_PLIST="$TMP_DIR/build/$APP_NAME.app/Contents/Info.plist"
[ -f "$BUILT_PLIST" ] || fail "the bundle rule produced no $BUILT_PLIST"
built_text="$(plutil -extract NSMicrophoneUsageDescription raw -o - "$BUILT_PLIST" 2>/dev/null || true)"
if [ "$built_text" != "$plist_text" ]; then
    fail "the built bundle does not carry the usage description the source declares:
  $PLIST_FILE: $plist_text
  $BUILT_PLIST: ${built_text:-<absent>}"
fi

echo "check-microphone-usage: OK - both platforms declare the same sentence, and the built macOS bundle carries it"
