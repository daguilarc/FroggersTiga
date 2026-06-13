cmake_minimum_required(VERSION 3.22)
project(FroggersTigaDesktop VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/core")
set(SIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sim")
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

juce_add_binary_data(FroggersTigaAssets
    SOURCES
        Resources/Icon.png
        "${REPO_ROOT}/SIM_MANUAL.md"
        "${REPO_ROOT}/QUICK_DICT.md"
        "${REPO_ROOT}/LICENSE"
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

target_link_libraries(FroggersTigaDesktop PRIVATE
    FroggersTigaAssets
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_gui_extra
)
