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
    Source/ui/ModLanePicker.cpp
    Source/ui/SubmodulePagePanel.cpp
    Source/ui/AdsrPagePanel.cpp
    Source/ui/PageCarouselComponent.cpp
    Source/ui/GlobalOscilloscopeDisplay.cpp
    Source/ui/GlobalStripV2.cpp
    Source/ui/CenterGlobalClusterV2.cpp
    Source/ui/PerformanceBandV2.cpp
    Source/ui/SequencerPanelComponent.cpp
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
    Source/ui/CenterGlobalClusterV2.cpp
    Source/ui/SubmodulePagePanel.cpp
    Source/ui/AdsrPagePanel.cpp
    Source/ui/EncoderRingComponent.cpp
    Source/ui/ModLanePicker.cpp
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
    Source/ui/SequencerPanelComponent.cpp
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
    Source/ui/CenterGlobalClusterV2.cpp
    Source/ui/EncoderRingComponent.cpp
    Source/control/FroggersV2ControlCore.cpp
)
target_include_directories(GlobalControlParity_test PRIVATE
    Source
    Source/ui
    "${CORE_DIR}"
    "${SIM_DIR}"
)
target_link_libraries(GlobalControlParity_test PRIVATE juce::juce_gui_basics)
add_test(NAME GlobalControlParity_test COMMAND GlobalControlParity_test)

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
