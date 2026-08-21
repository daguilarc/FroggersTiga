# app/vst/CMakeLists.txt -- the VST/AU plugin skeleton. ALL JUCE code lives
# under app/vst/;
# app/'s JUCE-free core (Froggers.hpp et al.) and its own build
# (app/Makefile, check_no_juce.cpp) are untouched by this file and never
# depend on it.
#
# CMake shape adapted from the DELETED old wrappers' juce_add_plugin idiom
# (git show 0be9ab0~1:desktop/CMakeLists.txt lines 136-313 -- read, not
# resurrected): that block used juce_add_plugin(... FORMATS VST3 AU IS_SYNTH
# TRUE ...) via FetchContent'd JUCE. This file keeps the same juce_add_plugin
# shape (formats, IS_SYNTH, COPY_PLUGIN_AFTER_BUILD FALSE) but:
#   - adds JUCE via add_subdirectory($ENV{HOME}/JUCE ...), per the
#     explicit instruction (JUCE is already checked out locally), not
#     FetchContent;
#   - targets the CURRENT app core (app/Froggers.hpp,
#     synth_froggers::FroggersApp) through Sheaf's synth::Engine<App>, not
#     the old desktop/Source/ tree (deleted, unrelated to this app);
#   - declares NO input bus at all (old idiom didn't need this distinction);
#   - adds a JUCE-free-Sheaf-core custom step (below) the old idiom never
#     needed, since desktop/'s core lived in src/core, not behind Sheaf's own
#     Makefile-built static lib.
#
# Build (binding): -j2 max, always, nice'd -- this machine
# locks up above -j2.
#   cmake -S app/vst -B app/vst/build
#   cmake --build app/vst/build --config Release -- -j2

cmake_minimum_required(VERSION 3.22)
# JUCE's own top-level CMakeLists.txt declares `LANGUAGES C CXX` (some of its
# dependencies -- e.g. VST3 SDK helper code -- compile .c files); omitting C
# here left CMAKE_C_COMPILE_OBJECT unset and the generate step failed.
project(FroggersVst VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macOS deployment target")

set(FROGGERS_JUCE_DIR "$ENV{HOME}/JUCE" CACHE PATH
    "Path to a local JUCE checkout, added via add_subdirectory (not FetchContent)")
if(NOT EXISTS "${FROGGERS_JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${FROGGERS_JUCE_DIR} (set -DFROGGERS_JUCE_DIR=... "
                         "or check out JUCE there) -- see app/vst/CMakeLists.txt's own header comment")
endif()
add_subdirectory("${FROGGERS_JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")

set(APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(SHEAF_SYNTH_DIR "${APP_DIR}/../External/Sheaf/projects/synth")
set(SHEAF_LIB "${SHEAF_SYNTH_DIR}/build/libsynth.a")

# --- Sheaf's JUCE-free static core -----------------------------------------
# app/Makefile's own SHEAF_LIB rule delegates to Sheaf's Makefile the same
# way (`$(MAKE) -C $(SHEAF_SYNTH_DIR) build`), every time, and lets Sheaf's
# own build decide whether anything actually needs recompiling.
#
# This USED to be an
# execute_process() at CONFIGURE time, specifically to avoid ever running
# concurrently with the `cmake --build ... -- -j2` step below and multiplying
# this machine's effective parallelism past the hard -j2 cap. The problem
# that traded away: `cmake --build` alone (no reconfigure) could link a
# STALE libsynth.a -- a Sheaf source edit with no app/vst/ reconfigure in
# between would never be seen. Fixed here with a build-time
# add_custom_target instead, WITHOUT reintroducing the over-parallelism risk:
# add_dependencies() below makes FroggersVst (and therefore everything that
# links or depends on it, including FroggersVstSmokeTest) wait on this
# target before compiling ANYTHING, so the outer build graph's second -j2
# slot has no other job available to run concurrently with the nested
# `nice make -j2` while it executes -- the two -j2's are serialized by the
# dependency edge, not by configure-vs-build timing. (add_custom_target's
# COMMAND always reruns when the target is built, exactly like the old
# execute_process did every configure -- it is Sheaf's own Makefile that
# decides per invocation whether anything is actually stale, same
# "every time, let Sheaf decide" contract as app/Makefile's SHEAF_LIB rule.)
find_program(FROGGERS_NICE_EXE nice)
if(FROGGERS_NICE_EXE)
    set(FROGGERS_SHEAF_LIB_BUILD_COMMAND "${FROGGERS_NICE_EXE}" -n 10 make -j2 build)
else()
    set(FROGGERS_SHEAF_LIB_BUILD_COMMAND make -j2 build)
endif()
add_custom_target(FroggersSheafLibsynth ALL
    COMMAND ${FROGGERS_SHEAF_LIB_BUILD_COMMAND}
    WORKING_DIRECTORY "${SHEAF_SYNTH_DIR}"
    BYPRODUCTS "${SHEAF_LIB}"
    COMMENT "app/vst: building Sheaf's libsynth.a (nice make -j2 build in ${SHEAF_SYNTH_DIR})"
    VERBATIM
)

# Stop-and-ask gate (deleted idiom's own convention,
# desktop/CMakeLists.txt:143-146): instrument/generator identity must stay
# IS_SYNTH TRUE. Named FROGGERS_VST_PLUGIN_IS_SYNTH (not
# FROGGERS_VST_IS_SYNTH) so this CMake-script-only variable can't be
# confused with desktop-v2/CMakeLists.txt:300,340's FROGGERS_VST_IS_SYNTH,
# a real C++ preprocessor macro for unrelated targets there (different
# namespace, no actual collision -- this is a readability-only rename).
set(FROGGERS_VST_PLUGIN_IS_SYNTH TRUE)
if(NOT FROGGERS_VST_PLUGIN_IS_SYNTH)
    message(FATAL_ERROR "FroggersVst must build with IS_SYNTH TRUE -- it is an instrument/generator plugin, not an effect")
endif()

juce_add_plugin(FroggersVst
    PRODUCT_NAME "Frogg3rs"
    COMPANY_NAME "frogg3rs"
    BUNDLE_ID "dev.sheaf.Frogg3rs.vst"
    PLUGIN_MANUFACTURER_CODE Shef
    PLUGIN_CODE Fr3g
    FORMATS VST3 AU
    IS_SYNTH TRUE
    # "a synth that declares MIDI input accepted is fine; no
    # note wiring" -- FroggersPluginProcessor::acceptsMidi() agrees (TRUE),
    # and processBlock() ignores the MIDI buffer entirely.
    NEEDS_MIDI_INPUT TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# FroggersVst (the shared-code target every format
# wrapper links, and what FroggersVstSmokeTest itself links) cannot start
# compiling until libsynth.a has been (re)built -- see
# FroggersSheafLibsynth's own comment above for why this also keeps total
# concurrent build parallelism at the -j2 cap.
add_dependencies(FroggersVst FroggersSheafLibsynth)

# Operator documentation ships with the plugin (froggers-sheaf-runtime-app
# spec, "Operator documentation ships with the app"): the manual and quick
# dictionary are embedded from the repository's single copy at build time,
# so no second checked-in copy exists to drift. Same source-of-truth
# approach desktop/CMakeLists.txt uses for SIM_MANUAL.md/QUICK_DICT.md/
# LICENSE via FROGGERS_ASSET_SOURCES (that tree is frozen -- read for the
# pattern, not touched) -- here the two docs are copied straight into each
# built plugin bundle's Contents/Resources after the format target links,
# since juce_add_plugin has no built-in "bundle this file" option the way
# juce_add_gui_app's ICON_BIG has for the standalone app.
get_filename_component(FROGGERS_REPO_ROOT "${APP_DIR}/.." ABSOLUTE)
set(FROGGERS_DOC_SOURCES
    "${FROGGERS_REPO_ROOT}/MANUAL.md"
    "${FROGGERS_REPO_ROOT}/QUICK_DICT.md"
)
foreach(FROGGERS_VST_FORMAT VST3 AU)
    if(TARGET FroggersVst_${FROGGERS_VST_FORMAT})
        add_custom_command(TARGET FroggersVst_${FROGGERS_VST_FORMAT} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_CONTENT_DIR:FroggersVst_${FROGGERS_VST_FORMAT}>/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${FROGGERS_DOC_SOURCES}
                "$<TARGET_BUNDLE_CONTENT_DIR:FroggersVst_${FROGGERS_VST_FORMAT}>/Resources"
            COMMENT "app/vst: bundling MANUAL.md/QUICK_DICT.md into ${FROGGERS_VST_FORMAT} Resources"
            VERBATIM
        )
    endif()
endforeach()

target_sources(FroggersVst PRIVATE
    FroggersPluginProcessor.cpp
    # The real portable-surface editor. Needs
    # SHEAF_SYNTH_DIR/juce on the include path (below) for
    # PortableJuceBackend.hpp -- runtime/ and include/ alone (the
    # original set) never pulled that directory in.
    FroggersPluginEditor.cpp
    # Only the tiny data-root helper (juce_core dependency, no
    # window/device/MIDI-connection machinery) -- see
    # FroggersPluginProcessor.hpp's header comment on why reusing this one
    # file is not "launcher-code duplication."
    "${SHEAF_SYNTH_DIR}/runtime/HostDataPaths.cpp"
)

target_include_directories(FroggersVst PRIVATE
    "${APP_DIR}"
    "${SHEAF_SYNTH_DIR}/include"
    "${SHEAF_SYNTH_DIR}/runtime"
    # synth_juce::PortableComponent (PortableJuceBackend.hpp), the
    # render-host seam FroggersPluginEditor.cpp builds on -- see that file's
    # own header comment for the full trace.
    "${SHEAF_SYNTH_DIR}/juce"
)

target_compile_features(FroggersVst PUBLIC cxx_std_20)

target_link_libraries(FroggersVst
    PUBLIC
        juce::juce_audio_utils
        juce::juce_audio_processors
    PRIVATE
        "${SHEAF_LIB}"
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(FroggersVst PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    # This plugin has never shipped a VST2 build and FORMATS never requests
    # one -- JUCE's juce_audio_plugin_client_VST3.cpp hard-errors by default
    # (JUCE_VST3_CAN_REPLACE_VST2, on unless disabled) warning about VST2/VST3
    # parameter-ID migration that cannot apply here. Per that file's own
    # comment: "If you have released neither a VST2 or VST3 version of the
    # plugin, consider only releasing a VST3 version and disabling
    # JUCE_VST3_CAN_REPLACE_VST2."
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# --- processBlock smoke test (CTest) -----------------------------------
enable_testing()

add_executable(FroggersVstSmokeTest FroggersVstSmokeTest.cpp)
target_include_directories(FroggersVstSmokeTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${APP_DIR}"
    "${SHEAF_SYNTH_DIR}/include"
    "${SHEAF_SYNTH_DIR}/runtime"
)
target_compile_features(FroggersVstSmokeTest PRIVATE cxx_std_20)
# Links the plugin's own shared-code target (juce_add_plugin's FroggersVst
# static library, the same object code every format wrapper links) so the
# test drives the REAL compiled FroggersPluginProcessor, not a second copy.
target_link_libraries(FroggersVstSmokeTest PRIVATE FroggersVst)

add_test(NAME FroggersVstSmokeTest COMMAND FroggersVstSmokeTest)

# --- DAW-external transport/tempo/surface-row tests (CTest) -----------
# Same "drive the real compiled processor" shape as FroggersVstSmokeTest
# above (links FroggersVst, the shared-code target every format wrapper
# links), plus app/FroggersUiSurface.hpp directly for the plugin-mode
# surface-row assertions -- that header is JUCE-free itself, but
# this test binary already links JUCE transitively via FroggersVst, so no
# separate target is needed for it.
add_executable(FroggersVstHostTests FroggersVstHostTests.cpp)
target_include_directories(FroggersVstHostTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${APP_DIR}"
    "${SHEAF_SYNTH_DIR}/include"
    "${SHEAF_SYNTH_DIR}/runtime"
)
target_compile_features(FroggersVstHostTests PRIVATE cxx_std_20)
target_link_libraries(FroggersVstHostTests PRIVATE FroggersVst)

add_test(NAME FroggersVstHostTests COMMAND FroggersVstHostTests)

# --- Editor construction/destruction (CTest, own binary) ---------------
# Deliberately ISOLATED from FroggersVstHostTests above: this is the one
# place in the app/vst/ suite that constructs a real juce::Component
# (FroggersPluginEditor), and a headless CTest process has no guarantee a
# JUCE GUI Component can be constructed/shown safely -- see
# FroggersVstEditorTest.cpp's own header comment. Keeping this experiment in
# its own process means a hard crash here reports as exactly one failed
# CTest entry and never takes FroggersVstHostTests's other (already solid,
# GUI-free) cases down with it.
add_executable(FroggersVstEditorTest FroggersVstEditorTest.cpp)
target_include_directories(FroggersVstEditorTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${APP_DIR}"
    "${SHEAF_SYNTH_DIR}/include"
    "${SHEAF_SYNTH_DIR}/runtime"
    "${SHEAF_SYNTH_DIR}/juce"
)
target_compile_features(FroggersVstEditorTest PRIVATE cxx_std_20)
target_link_libraries(FroggersVstEditorTest PRIVATE FroggersVst)

add_test(NAME FroggersVstEditorTest COMMAND FroggersVstEditorTest)
