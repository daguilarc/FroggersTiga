cmake_minimum_required(VERSION 3.22)
project(FroggersTigaDesktopV2 VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_DESKTOP_V2 "Build FroggersTiga Desktop V2 app" OFF)

if(NOT BUILD_DESKTOP_V2)
    message(STATUS "BUILD_DESKTOP_V2 is OFF; skipping FroggersTigaDesktopV2")
    return()
endif()

include(FetchContent)
set(FROGGERS_JUCE_SOURCE_DIR "" CACHE PATH "Optional local JUCE source checkout/cache for offline desktop configuration")
if(FROGGERS_JUCE_SOURCE_DIR)
    set(FETCHCONTENT_SOURCE_DIR_JUCE "${FROGGERS_JUCE_SOURCE_DIR}" CACHE PATH "Use local JUCE source" FORCE)
endif()

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/core")
set(SIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sim")
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(V1_DESKTOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../desktop")

# --- Sheaf runtime vendor slice --------------------------------------------
# openspec/changes/desktop-v2-sheaf-runtime-harmonization packet 2 (tasks 2.1-2.3).
# Vendored in-tree (copied, not submoduled/fetched) from Sheaf pinned at
# commit c1810393; see
# openspec/changes/desktop-v2-sheaf-runtime-harmonization/sheaf-adoption-inventory.md
# for the full source->destination table. No network FetchContent of Sheaf.
set(SHEAF_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/External/Sheaf")

add_library(SheafRuntimeVendor STATIC
    "${SHEAF_VENDOR_DIR}/src/ButtonGrid.cpp"
    "${SHEAF_VENDOR_DIR}/src/DspWavetable.cpp"
    "${SHEAF_VENDOR_DIR}/src/MidiConfigBlocks.cpp"
    "${SHEAF_VENDOR_DIR}/src/MidiConfigViewModel.cpp"
    "${SHEAF_VENDOR_DIR}/src/MidiController.cpp"
    "${SHEAF_VENDOR_DIR}/src/MidiDevicePoller.cpp"
    "${SHEAF_VENDOR_DIR}/src/MidiReconcile.cpp"
    "${SHEAF_VENDOR_DIR}/src/Modules.cpp"
    "${SHEAF_VENDOR_DIR}/src/ParameterModulation.cpp"
    "${SHEAF_VENDOR_DIR}/src/PatchPersistence.cpp"
    "${SHEAF_VENDOR_DIR}/runtime/HostDataPaths.cpp"
)
target_include_directories(SheafRuntimeVendor PUBLIC
    "${SHEAF_VENDOR_DIR}"
    "${SHEAF_VENDOR_DIR}/include"
    "${SHEAF_VENDOR_DIR}/runtime"
    "${SHEAF_VENDOR_DIR}/juce"
)
# Sheaf's own build (runtime/juce_build.mk) compiles this slice at -std=c++20
# (Engine.hpp uses concepts; PortableUIBuilders/GangedRandomLfoVisualizer use
# std::span). Scoped to this vendored target only; the rest of desktop-v2
# stays on its existing CMAKE_CXX_STANDARD.
set_target_properties(SheafRuntimeVendor PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(SheafRuntimeVendor PUBLIC juce::juce_core)

set(V2_FONT_REGULAR "${CMAKE_CURRENT_SOURCE_DIR}/Assets/IBMPlexSans-Regular.ttf")
set(V2_FONT_SEMIBOLD "${CMAKE_CURRENT_SOURCE_DIR}/Assets/IBMPlexSans-SemiBold.ttf")
set(FROGGERS_V2_ASSET_SOURCES
    "${V1_DESKTOP_DIR}/Resources/Icon.png"
    "${REPO_ROOT}/SIM_MANUAL.md"
    "${REPO_ROOT}/QUICK_DICT.md"
    "${REPO_ROOT}/LICENSE"
)
if(EXISTS "${V2_FONT_REGULAR}" AND EXISTS "${V2_FONT_SEMIBOLD}")
    list(APPEND FROGGERS_V2_ASSET_SOURCES "${V2_FONT_REGULAR}" "${V2_FONT_SEMIBOLD}")
    set(FROGGERS_V2_BUNDLED_IBM_PLEX 1)
else()
    message(STATUS "IBM Plex Sans not bundled; DesktopV2LookAndFeel uses system sans fallback")
endif()

juce_add_binary_data(FroggersTigaV2Assets
    SOURCES
        ${FROGGERS_V2_ASSET_SOURCES}
)

juce_add_gui_app(FroggersTigaDesktopV2
    PRODUCT_NAME "FroggersTigaV2"
    COMPANY_NAME "FroggersTiga"
    BUNDLE_ID "com.froggers.tiga.desktop.v2"
    ICON_BIG "${V1_DESKTOP_DIR}/Resources/Icon.png"
    ICON_SMALL "${V1_DESKTOP_DIR}/Resources/Icon.png"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "FroggersTiga needs audio input access for external ring-mod (mic, line-in, or interface)."
)

target_sources(FroggersTigaDesktopV2 PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/DesktopV2HostCallbacks.cpp
    Source/AudioEngine.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/control/FroggersV2HostBridge.cpp
    Source/control/FroggersV2AppCoreFacade.cpp
    Source/runtime/DesktopV2RuntimeProjection.cpp
    Source/runtime/FilePatchRuntimeState.cpp
    Source/runtime/DesktopV2AudioStateProjection.cpp
    Source/runtime/FilePatchRuntimePageComponent.cpp
    Source/runtime/AudioRuntimePageComponent.cpp
    Source/runtime/ControllersRuntimePageComponent.cpp
    Source/runtime/HostedRuntimeStatusPanel.cpp
    Source/ui/EncoderRingComponent.cpp
    Source/ui/SubmodulePagePanel.cpp
    Source/ui/AdsrPagePanel.cpp
    Source/ui/PageCarouselComponent.cpp
    Source/ui/GlobalOscilloscopeDisplay.cpp
    Source/ui/GlobalStripV2.cpp
    Source/ui/PerformanceBandV2.cpp
    Source/ui/CvScopeDisplay.cpp
    Source/ui/DesktopV2LookAndFeel.cpp
    Source/AudioSettingsComponent.cpp
    Source/control/MidiCvAssignmentTable.cpp
    Source/MidiCvSettingsComponent.cpp
    "${V1_DESKTOP_DIR}/Source/AudioRecorder.cpp"
    "${V1_DESKTOP_DIR}/Source/RecordButton.cpp"
)

target_include_directories(FroggersTigaDesktopV2 PRIVATE
    Source
    Source/ui
    "${V1_DESKTOP_DIR}/Source"
    "${CORE_DIR}"
    "${SIM_DIR}"
)

target_compile_definitions(FroggersTigaDesktopV2 PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:FroggersTigaDesktopV2,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)
if(FROGGERS_V2_BUNDLED_IBM_PLEX)
    target_compile_definitions(FroggersTigaDesktopV2 PRIVATE FROGGERS_V2_BUNDLED_IBM_PLEX=1)
endif()

if(MSVC)
    target_compile_definitions(FroggersTigaDesktopV2 PRIVATE _USE_MATH_DEFINES)
endif()

target_link_libraries(FroggersTigaDesktopV2 PRIVATE
    FroggersTigaV2Assets
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_extra
)

if(APPLE)
    set(_DESKTOP_V2_STALE_BUNDLE "${CMAKE_CURRENT_BINARY_DIR}/FroggersTigaDesktopV2_artefacts/FroggersTigaV2.app")
    add_custom_command(TARGET FroggersTigaDesktopV2 POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DRELEASE_BUNDLE="$<TARGET_BUNDLE_DIR:FroggersTigaDesktopV2>"
            -DSTALE_BUNDLE="${_DESKTOP_V2_STALE_BUNDLE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RemoveStaleDesktopV2Bundle.cmake"
        COMMENT "Remove stale root-level FroggersTigaV2.app when not current Release output"
    )
endif()


# --- Runtime-hosted entry target (Packet 10 shell cutover, additive step) --
# Coexists with FroggersTigaDesktopV2 above: boots FroggersApp
# (Source/FroggersApp.hpp) under the vendored Sheaf Runtime shell
# (External/Sheaf/runtime/Shell.hpp) instead of MainComponent/Main.cpp. Does
# NOT retire the legacy target or its sources -- see MainRuntime.cpp's doc
# comment. Source list mirrors FroggersApp_test's already-proven minimal
# link set below (the DSP/control-core/manifest sources FroggersTigaDesktopV2
# compiles, minus Main.cpp/MainComponent.cpp and the JUCE-widget-only files
# the portable Sheaf surface doesn't use -- DesktopV2HostCallbacks,
# PageCarouselComponent, GlobalStripV2, PerformanceBandV2,
# SequencerPanelComponent, EncoderRingComponent, SubmodulePagePanel,
# AdsrPagePanel, CvScopeDisplay, DesktopV2LookAndFeel, and the MainComponent-
# specific runtime page components).
juce_add_gui_app(FroggersTigaDesktopV2Runtime
    PRODUCT_NAME "FroggersTigaRuntime"
    COMPANY_NAME "FroggersTiga"
    BUNDLE_ID "com.froggers.tiga.desktop.v2.runtime"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "FroggersTiga needs audio input access for external ring-mod (mic, line-in, or interface)."
)

target_sources(FroggersTigaDesktopV2Runtime PRIVATE
    Source/MainRuntime.cpp
    Source/FroggersAppCore.cpp
    Source/control/FroggersV2AppCoreFacade.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/control/FroggersV2HostBridge.cpp
    Source/AudioEngine.cpp
    Source/AudioSettingsComponent.cpp
    Source/MidiCvSettingsComponent.cpp
    Source/control/MidiCvAssignmentTable.cpp
    Source/runtime/FilePatchRuntimeState.cpp
    Source/runtime/DesktopV2AudioStateProjection.cpp
    "${V1_DESKTOP_DIR}/Source/AudioRecorder.cpp"
)

target_include_directories(FroggersTigaDesktopV2Runtime PRIVATE
    Source
    "${V1_DESKTOP_DIR}/Source"
    "${CORE_DIR}"
    "${SIM_DIR}"
)

if(MSVC)
    target_compile_definitions(FroggersTigaDesktopV2Runtime PRIVATE _USE_MATH_DEFINES)
endif()

target_link_libraries(FroggersTigaDesktopV2Runtime PRIVATE
    SheafRuntimeVendor
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_extra
)

# Sheaf's vendored Runtime/Engine headers require C++20 (concepts, std::span)
# -- same reason FroggersApp_test/FroggersAppSurface_test below are pinned to
# CXX_STANDARD 20 rather than this project's default CMAKE_CXX_STANDARD (17).
set_target_properties(FroggersTigaDesktopV2Runtime PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

enable_testing()

add_executable(ControlCoreBridge_test
    tests/ControlCoreBridge_test.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/control/FroggersV2HostBridge.cpp
)
target_include_directories(ControlCoreBridge_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
add_test(NAME ControlCoreBridge_test COMMAND ControlCoreBridge_test)

add_executable(MidiCvAssignment_test
    tests/MidiCvAssignment_test.cpp
    Source/control/MidiCvAssignmentTable.cpp
)
target_include_directories(MidiCvAssignment_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
add_test(NAME MidiCvAssignment_test COMMAND MidiCvAssignment_test)

add_executable(ExternalMidiMod_test
    tests/ExternalMidiMod_test.cpp
    Source/control/FroggersV2ControlCore.cpp
)
target_include_directories(ExternalMidiMod_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
add_test(NAME ExternalMidiMod_test COMMAND ExternalMidiMod_test)

add_executable(FroggersV2ManifestSnapshot
    tools/FroggersV2ManifestSnapshot.cpp
)
target_include_directories(FroggersV2ManifestSnapshot PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)

set(FROGGERS_V2_MANIFEST_OUTPUT_DIR "${REPO_ROOT}/build/manifest")
add_custom_target(FroggersV2ManifestArtifacts
    COMMAND FroggersV2ManifestSnapshot "${FROGGERS_V2_MANIFEST_OUTPUT_DIR}"
    BYPRODUCTS
        "${FROGGERS_V2_MANIFEST_OUTPUT_DIR}/froggers-v2-manifest.snapshot.json"
        "${FROGGERS_V2_MANIFEST_OUTPUT_DIR}/froggers-v2-manifest-report.md"
    DEPENDS FroggersV2ManifestSnapshot
    COMMENT "Generating Froggers v2 manifest reviewer artifacts"
)

add_executable(HostParameterProcessorV2_test
    tests/HostParameterProcessorV2_test.cpp
    Source/control/FroggersV2ControlCore.cpp
)
target_include_directories(HostParameterProcessorV2_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_compile_definitions(HostParameterProcessorV2_test PRIVATE
    FROGGERS_VST_IS_SYNTH=1
)
add_test(NAME HostParameterProcessorV2_test COMMAND HostParameterProcessorV2_test)

add_executable(FroggersV2Manifest_test
    tests/FroggersV2Manifest_test.cpp
)
target_include_directories(FroggersV2Manifest_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
add_test(NAME FroggersV2Manifest_test COMMAND FroggersV2Manifest_test)

add_executable(FroggersV2ProjectionValidators_test
    tests/FroggersV2ProjectionValidators_test.cpp
    Source/control/MidiCvAssignmentTable.cpp
)
target_include_directories(FroggersV2ProjectionValidators_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_compile_definitions(FroggersV2ProjectionValidators_test PRIVATE
    FROGGERS_V2_REPO_ROOT="${REPO_ROOT}"
)
add_test(NAME FroggersV2ProjectionValidators_test COMMAND FroggersV2ProjectionValidators_test)

# Task 4.4 regression: desktop-v2 preset load silently drops the deleted Random
# S&H module-page axes back to Marbles Sheaf-style defaults (shared engine keeps
# the Marbles page at PM index 1; shared SimPresetSnapshot is left unchanged).
add_executable(PresetRandomPageDrop_test
    tests/PresetRandomPageDrop_test.cpp
)
target_include_directories(PresetRandomPageDrop_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_compile_definitions(PresetRandomPageDrop_test PRIVATE
    FROGGERS_VST_IS_SYNTH=1
)
add_test(NAME PresetRandomPageDrop_test COMMAND PresetRandomPageDrop_test)

# I1 drift-guard: the D10 page-label fork (V2DesktopPageDisplayNames vs shared
# V2ParamDisplayNames) must agree on every non-Random page until the web-v2
# reconciliation.
add_executable(V2PageAuthorityForkParity_test
    tests/V2PageAuthorityForkParity_test.cpp
)
target_include_directories(V2PageAuthorityForkParity_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
add_test(NAME V2PageAuthorityForkParity_test COMMAND V2PageAuthorityForkParity_test)

add_executable(GlobalOscilloscopeDisplay_test
    tests/GlobalOscilloscopeDisplay_test.cpp
    Source/ui/GlobalOscilloscopeDisplay.cpp
    Source/ui/CvScopeDisplay.cpp
)
target_include_directories(GlobalOscilloscopeDisplay_test PRIVATE
    Source
    Source/ui
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_compile_definitions(GlobalOscilloscopeDisplay_test PRIVATE
    FROGGERS_V2_REPO_ROOT="${REPO_ROOT}"
)
target_link_libraries(GlobalOscilloscopeDisplay_test PRIVATE juce::juce_gui_basics)
add_test(NAME GlobalOscilloscopeDisplay_test COMMAND GlobalOscilloscopeDisplay_test)

set(DESKTOP_V2_CAROUSEL_TEST_SOURCES
    Source/DesktopV2HostCallbacks.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/control/FroggersV2HostBridge.cpp
    Source/ui/PageCarouselComponent.cpp
    Source/ui/SubmodulePagePanel.cpp
    Source/ui/AdsrPagePanel.cpp
    Source/ui/EncoderRingComponent.cpp
    Source/ui/DesktopV2LookAndFeel.cpp
)

add_executable(CallbackLifetime_test
    tests/CallbackLifetime_test.cpp
    ${DESKTOP_V2_CAROUSEL_TEST_SOURCES}
)
target_include_directories(CallbackLifetime_test PRIVATE
    Source
    Source/ui
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(CallbackLifetime_test PRIVATE juce::juce_gui_basics)
add_test(NAME CallbackLifetime_test COMMAND CallbackLifetime_test)

set(DESKTOP_V2_LAYOUT_TEST_SOURCES
    ${DESKTOP_V2_CAROUSEL_TEST_SOURCES}
    Source/ui/GlobalStripV2.cpp
    Source/ui/GlobalOscilloscopeDisplay.cpp
    Source/ui/CvScopeDisplay.cpp
    Source/ui/PerformanceBandV2.cpp
)

add_executable(LayoutBounds_test
    tests/LayoutBounds_test.cpp
    ${DESKTOP_V2_LAYOUT_TEST_SOURCES}
)
target_include_directories(LayoutBounds_test PRIVATE
    Source
    Source/ui
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(LayoutBounds_test PRIVATE juce::juce_gui_basics)
add_test(NAME LayoutBounds_test COMMAND LayoutBounds_test)

add_executable(GlobalControlParity_test
    tests/GlobalControlParity_test.cpp
    Source/ui/GlobalStripV2.cpp
    Source/ui/EncoderRingComponent.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/control/FroggersV2HostBridge.cpp
)
target_include_directories(GlobalControlParity_test PRIVATE
    Source
    Source/ui
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(GlobalControlParity_test PRIVATE juce::juce_gui_basics)
add_test(NAME GlobalControlParity_test COMMAND GlobalControlParity_test)

add_executable(ModSourceGrid_test
    tests/ModSourceGrid_test.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/ui/EncoderRingComponent.cpp
)
target_include_directories(ModSourceGrid_test PRIVATE
    Source
    Source/ui
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(ModSourceGrid_test PRIVATE juce::juce_gui_basics)
add_test(NAME ModSourceGrid_test COMMAND ModSourceGrid_test)

set(FROGGERS_V2_FACADE_TEST_SOURCES
    tests/FroggersV2AppCoreFacade_test.cpp
    Source/control/FroggersV2AppCoreFacade.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/control/FroggersV2HostBridge.cpp
    Source/AudioEngine.cpp
    Source/AudioSettingsComponent.cpp
    Source/MidiCvSettingsComponent.cpp
    Source/control/MidiCvAssignmentTable.cpp
    Source/runtime/FilePatchRuntimeState.cpp
    Source/runtime/DesktopV2AudioStateProjection.cpp
    "${V1_DESKTOP_DIR}/Source/AudioRecorder.cpp"
)

add_executable(FroggersV2AppCoreFacade_test ${FROGGERS_V2_FACADE_TEST_SOURCES})
target_include_directories(FroggersV2AppCoreFacade_test PRIVATE
    Source
    "${V1_DESKTOP_DIR}/Source"
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(FroggersV2AppCoreFacade_test PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_extra
)
add_test(NAME FroggersV2AppCoreFacade_test COMMAND FroggersV2AppCoreFacade_test)

add_executable(RuntimePages_test
    tests/RuntimePages_test.cpp
    Source/runtime/DesktopV2RuntimeProjection.cpp
    Source/runtime/FilePatchRuntimeState.cpp
    Source/control/MidiCvAssignmentTable.cpp
)
target_include_directories(RuntimePages_test PRIVATE
    Source
    "${V1_DESKTOP_DIR}/Source"
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_compile_definitions(RuntimePages_test PRIVATE
    FROGGERS_V2_REPO_ROOT="${REPO_ROOT}"
)
target_link_libraries(RuntimePages_test PRIVATE juce::juce_gui_basics)
add_test(NAME RuntimePages_test COMMAND RuntimePages_test)

add_executable(BootSmoke_test tests/BootSmoke_test.cpp)
add_test(NAME BootSmoke_test COMMAND BootSmoke_test)

# RuntimeBootSmoke_test -- Packet 10 shell cutover. Same fork/exec/poll-alive
# pattern as BootSmoke_test above, pointed at the new
# FroggersTigaDesktopV2Runtime binary instead of the legacy one; proves
# FroggersApp boots under the vendored Sheaf Runtime shell (see
# MainRuntime.cpp / RuntimeBootSmoke_test.cpp doc comments).
add_executable(RuntimeBootSmoke_test tests/RuntimeBootSmoke_test.cpp)
add_test(NAME RuntimeBootSmoke_test COMMAND RuntimeBootSmoke_test)

# SheafVendorSmoke_test -- packet 2 compile gate (tasks 2.1-2.3). Proves the
# vendored synth::Engine / GridManager / RuntimeUIState / ScopeWriter /
# ScopeVisualizer / GangedRandomLfoVisualizer, plus the JUCE-facing runtime
# headers (Runtime.hpp/Shell.hpp/MainPane.hpp/PortableJuceBackend.hpp/
# RuntimePagesJuce.hpp/MidiHandlers.hpp), compile and link against this
# product's own JUCE modules. Does not instantiate SYNTH_RUNTIME_MAIN /
# ShellApplication<AppType> as a running app -- that cutover is tasks.md
# section 10, a later packet.
add_executable(SheafVendorSmoke_test tests/SheafVendorSmoke_test.cpp)
target_link_libraries(SheafVendorSmoke_test PRIVATE
    SheafRuntimeVendor
    juce::juce_gui_extra
    juce::juce_audio_devices
)
set_target_properties(SheafVendorSmoke_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
add_test(NAME SheafVendorSmoke_test COMMAND SheafVendorSmoke_test)

# FroggersApp_test -- packet 3 (tasks.md 3.1-3.3). FroggersApp/FroggersAppCore
# (Source/FroggersApp.hpp, Source/FroggersAppCore.hpp/.cpp) satisfy the
# vendored synth::SynthApplication concept and delegate ProcessBlock to the
# existing AudioEngine + FroggersV2ControlCore/facade path (design D2; no DSP
# rewrite). Built at CXX_STANDARD 20 like SheafVendorSmoke_test, since
# synth/AppContext.hpp transitively requires std::span (ParameterModulation.hpp).
add_executable(FroggersApp_test
    tests/FroggersApp_test.cpp
    Source/FroggersAppCore.cpp
    Source/control/FroggersV2AppCoreFacade.cpp
    Source/control/FroggersV2ControlCore.cpp
    Source/control/FroggersV2HostBridge.cpp
    Source/AudioEngine.cpp
    Source/AudioSettingsComponent.cpp
    Source/MidiCvSettingsComponent.cpp
    Source/control/MidiCvAssignmentTable.cpp
    Source/runtime/FilePatchRuntimeState.cpp
    Source/runtime/DesktopV2AudioStateProjection.cpp
    "${V1_DESKTOP_DIR}/Source/AudioRecorder.cpp"
)
target_include_directories(FroggersApp_test PRIVATE
    Source
    "${V1_DESKTOP_DIR}/Source"
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(FroggersApp_test PRIVATE
    SheafRuntimeVendor
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_extra
)
set_target_properties(FroggersApp_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
add_test(NAME FroggersApp_test COMMAND FroggersApp_test)

# FroggersScopePanels_test -- packet 5 (tasks.md 5.1-5.4). Dual-panel smoke
# coverage for Source/ui/FroggersScopePanels.hpp: the vendored Sheaf
# ScopeVisualizer/ScopeWriter/GangedRandomLfoVisualizer machinery (the one
# scope model this product reuses -- no parallel scope stack), fed with
# synthetic sample data. JUCE-free / no AudioEngine: only manifest headers
# (Source, CORE_DIR, SIM_DIR) + the vendored Sheaf headers (via
# SheafRuntimeVendor's PUBLIC include dirs).
add_executable(FroggersScopePanels_test tests/FroggersScopePanels_test.cpp)
target_include_directories(FroggersScopePanels_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(FroggersScopePanels_test PRIVATE
    SheafRuntimeVendor
)
set_target_properties(FroggersScopePanels_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
add_test(NAME FroggersScopePanels_test COMMAND FroggersScopePanels_test)

# FroggersAppSurface_test -- packet 5 (tasks.md 5.1-5.4), extended by packet 7
# increment 1 (tasks.md 7.2). Portable-visualizer presence coverage for
# Source/ui/FroggersAppSurface.hpp: proves the surface's actual
# synth::ui::Surface::BuildTree() exposes the two dual ScopeVisualizer panels
# and the GangedRandomLfoVisualizer on the Random S&H mod-depth cells, plus
# (packet 7 inc 1) the 6-tab module selector and the active module's 4x4
# grid (Audio ported fully). FroggersV2ControlCore.cpp is compiled in here
# because the surface now owns one JUCE-free control-core instance to read
# the Audio module's real params/labels from -- the same authority
# SubmodulePagePanel reads today (ExternalMidiMod_test above proves this .cpp
# already builds JUCE-free). Still no AudioEngine, same as
# FroggersScopePanels_test above.
add_executable(FroggersAppSurface_test
    tests/FroggersAppSurface_test.cpp
    Source/control/FroggersV2ControlCore.cpp
)
target_include_directories(FroggersAppSurface_test PRIVATE
    Source
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(FroggersAppSurface_test PRIVATE
    SheafRuntimeVendor
)
set_target_properties(FroggersAppSurface_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
add_test(NAME FroggersAppSurface_test COMMAND FroggersAppSurface_test)
