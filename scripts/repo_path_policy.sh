#!/usr/bin/env bash
# Shared path classifications for local hygiene scripts.
# This file intentionally does not run git commands.

REPO_REQUIRED_IGNORES=(
  sim/build/
  desktop/build/
  desktop/build-vst-test/
  desktop/dist/
  wasm/build/
  web/dist/
)

repo_policy_is_firmware_excluded() {
  case "$1" in
    External/*|src/FroggersTiga/*|src/common/*|src/mk/*|src/Blink/*|src/TestControl/*|MANUAL.md)
      return 0
      ;;
  esac
  return 1
}

repo_policy_is_publication_output() {
  case "$1" in
    docs/*|web/public/*)
      return 0
      ;;
  esac
  return 1
}

repo_policy_is_openspec_ephemeral() {
  case "$1" in
    openspec/.cache/*|openspec/.sessions/*|openspec/*/.cache/*|openspec/*/.sessions/*)
      return 0
      ;;
  esac
  return 1
}

repo_policy_is_local_cache_or_build_output() {
  local path="$1"

  case "$path" in
    .emsdk/*|node-v*/*|node-v*.tar.gz|Rack-SDK/*|vcv/Rack-SDK/*)
      return 0
      ;;
    sim/build/*|desktop/build/*|desktop/build-vst-test/*|desktop/dist/*|wasm/build/*|web/dist/*)
      return 0
      ;;
    vcv/build/*|vcv/dist/*|vcv/dep/*|desktop/FroggersTigaPlugin_artefacts/*)
      return 0
      ;;
  esac

  repo_policy_is_openspec_ephemeral "$path"
}

repo_policy_is_local_only_product_surface() {
  case "$1" in
    vcv/*|desktop/Source/PluginEditor.cpp|desktop/Source/PluginEditor.h|desktop/Source/PluginProcessor.cpp|desktop/Source/PluginProcessor.h)
      return 0
      ;;
  esac
  return 1
}

repo_policy_is_prohibited_public_artifact() {
  local path="$1"

  if repo_policy_is_firmware_excluded "$path" || repo_policy_is_publication_output "$path"; then
    return 1
  fi

  if repo_policy_is_local_cache_or_build_output "$path"; then
    return 0
  fi

  case "$path" in
    sim/CMakeCache.txt|sim/CMakeFiles/*|sim/cmake_install.cmake|sim/Makefile|sim/CTestTestfile.cmake)
      return 0
      ;;
    sim/*.o|sim/*.o.d|sim/*_test)
      return 0
      ;;
    desktop/CMakeCache.txt|desktop/CMakeFiles/*|desktop/cmake_install.cmake|desktop/Makefile)
      return 0
      ;;
    desktop/*.o|desktop/**/*.o|desktop/*.o.d)
      return 0
      ;;
    wasm/CMakeCache.txt|wasm/CMakeFiles/*|wasm/cmake_install.cmake|wasm/Makefile|wasm/*.o|wasm/*.wasm)
      return 0
      ;;
    web/node_modules/*)
      return 0
      ;;
    vcv/CMakeCache.txt|vcv/CMakeFiles/*|vcv/cmake_install.cmake|vcv/Makefile|vcv/*.o)
      return 0
      ;;
  esac

  return 1
}
