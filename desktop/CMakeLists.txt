cmake_minimum_required(VERSION 3.22)
project(FroggersTigaDesktop VERSION 1.0.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
set(DESKTOP_V2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../desktop-v2")
set(V2_FONT_REGULAR "${DESKTOP_V2_DIR}/Assets/IBMPlexSans-Regular.ttf")
set(V2_FONT_SEMIBOLD "${DESKTOP_V2_DIR}/Assets/IBMPlexSans-SemiBold.ttf")
set(FROGGERS_ASSET_SOURCES
    Resources/Icon.png
    "${REPO_ROOT}/SIM_MANUAL.md"
    "${REPO_ROOT}/QUICK_DICT.md"
    "${REPO_ROOT}/LICENSE"
)
if(EXISTS "${V2_FONT_REGULAR}" AND EXISTS "${V2_FONT_SEMIBOLD}")
    list(APPEND FROGGERS_ASSET_SOURCES "${V2_FONT_REGULAR}" "${V2_FONT_SEMIBOLD}")
    set(FROGGERS_V2_BUNDLED_IBM_PLEX 1)
endif()

juce_add_binary_data(FroggersTigaAssets
    SOURCES
        ${FROGGERS_ASSET_SOURCES}
)

juce_add_gui_app(FroggersTigaDesktop
    PRODUCT_NAME "FroggersTiga"
    COMPANY_NAME "FroggersTiga"
    BUNDLE_ID "com.froggers.tiga.desktop"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icon.png"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "FroggersTiga needs audio input access for external ring-mod (mic, line-in, or interface)."
)

target_sources(FroggersTigaDesktop PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/SubModulePanel.cpp
    Source/GlobalStrip.cpp
    Source/ModRackPanel.cpp
    Source/ModModuleBox.cpp
    Source/CvScopeDisplay.cpp
    Source/WaveMorphButton.cpp
    Source/PatchCableOverlay.cpp
    Source/MidiSettingsComponent.cpp
    Source/AudioSettingsComponent.cpp
    Source/AudioEngine.cpp
    Source/AudioRecorder.cpp
    Source/RecordButton.cpp
    Source/RecordExportCluster.cpp
    Source/HelpDocsDialog.cpp
    Source/AppMenuBar.cpp
    Source/InputEnvelopeIndicator.cpp
)

target_include_directories(FroggersTigaDesktop PRIVATE
    "${CORE_DIR}"
    "${SIM_DIR}"
    Source
)

target_compile_definitions(FroggersTigaDesktop PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:FroggersTigaDesktop,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)

# MSVC does not define M_PI unless _USE_MATH_DEFINES is set before <cmath>.
if(MSVC)
    target_compile_definitions(FroggersTigaDesktop PRIVATE _USE_MATH_DEFINES)
endif()

target_link_libraries(FroggersTigaDesktop PRIVATE
    FroggersTigaAssets
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_extra
)

enable_testing()

add_executable(AudioRecorder_test
    tests/AudioRecorder_test.cpp
    Source/AudioRecorder.cpp
    ../sim/OwnedAllocationHooks.cpp
)
target_include_directories(AudioRecorder_test PRIVATE
    "${CORE_DIR}"
    "${SIM_DIR}"
    Source
)
target_compile_definitions(AudioRecorder_test PRIVATE
    FROGGERS_RECORDER_TESTING=1
    FROGGERS_OWNED_ALLOCATION_INSTRUMENT=1
)
add_test(NAME AudioRecorder_test COMMAND AudioRecorder_test)

set(FROGGERS_UI_SOURCES
    Source/MainComponent.cpp
    Source/SubModulePanel.cpp
    Source/GlobalStrip.cpp
    Source/ModRackPanel.cpp
    Source/ModModuleBox.cpp
    Source/CvScopeDisplay.cpp
    Source/WaveMorphButton.cpp
    Source/PatchCableOverlay.cpp
    Source/MidiSettingsComponent.cpp
    Source/AudioSettingsComponent.cpp
    Source/AudioEngine.cpp
    Source/AudioRecorder.cpp
    Source/RecordButton.cpp
    Source/RecordExportCluster.cpp
    Source/HelpDocsDialog.cpp
    Source/InputEnvelopeIndicator.cpp
)

