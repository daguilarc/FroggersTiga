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
    Source/ui/EncoderRingComponent.cpp
    Source/ui/ModSourceCell.cpp
    Source/ui/SubmodulePagePanel.cpp
    Source/ui/AdsrPagePanel.cpp
    Source/ui/PageCarouselComponent.cpp
    Source/ui/VcoEfScopeDisplay.cpp
    Source/ui/GlobalStripV2.cpp
    Source/ui/PerformanceBandV2.cpp
    Source/ui/SequencerPanelComponent.cpp
    Source/ui/CvScopeDisplay.cpp
    Source/ui/DesktopV2LookAndFeel.cpp
    "${V1_DESKTOP_DIR}/Source/AudioSettingsComponent.cpp"
    Source/control/MidiCvAssignmentTable.cpp
    Source/MidiCvSettingsComponent.cpp
    "${V1_DESKTOP_DIR}/Source/AudioRecorder.cpp"
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
