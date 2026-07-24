// MainRuntime -- Runtime-hosted entry point for FroggersApp (Packet 10 shell
// cutover, additive/decision-free step). Boots FroggersApp
// (Source/FroggersApp.hpp) under the vendored Sheaf Runtime shell
// (External/Sheaf/runtime/Shell.hpp's SYNTH_RUNTIME_MAIN), coexisting with
// the untouched legacy MainComponent/Main.cpp entry (FroggersTigaDesktopV2
// target). Does not retire that target or port its transport/global-command
// chrome -- FroggersAppSurface's chrome_placeholder region stands in for it
// until that decision-gated work lands. See
// desktop-v2/tests/RuntimeBootSmoke_test.cpp for the boot proof.

#include "runtime/Shell.hpp"

#include "FroggersApp.hpp"

SYNTH_RUNTIME_MAIN(FroggersApp)
