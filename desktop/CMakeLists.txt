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

option(BUILD_VST "Build FroggersTiga VST3/AU plugin (local-only; sources not in public repo)" OFF)

if(BUILD_VST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginProcessor.cpp")
    enable_testing()
    # Task 5.9: instrument/generator identity — IS_SYNTH must remain TRUE.
    set(FROGGERS_PLUGIN_IS_SYNTH TRUE)
    if(NOT FROGGERS_PLUGIN_IS_SYNTH)
        message(FATAL_ERROR "FroggersTigaPlugin requires IS_SYNTH TRUE (task 5.9)")
    endif()

    juce_add_plugin(FroggersTigaPlugin
        PRODUCT_NAME "FroggersTiga"
        COMPANY_NAME "FroggersTiga"
        BUNDLE_ID "com.froggers.tiga.plugin"
        PLUGIN_MANUFACTURER_CODE FrTi
        PLUGIN_CODE FrTg
        FORMATS VST3 AU
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
    )

    target_sources(FroggersTigaPlugin PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/HostParameterRegistry.cpp
        ${FROGGERS_UI_SOURCES}
    )

    target_include_directories(FroggersTigaPlugin PRIVATE
        "${CORE_DIR}"
        "${SIM_DIR}"
        Source
    )

    target_compile_definitions(FroggersTigaPlugin PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    if(MSVC)
        target_compile_definitions(FroggersTigaPlugin PRIVATE _USE_MATH_DEFINES)
    endif()

    target_link_libraries(FroggersTigaPlugin PRIVATE
        FroggersTigaAssets
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )

    add_executable(HostParameterRegistry_test tests/HostParameterRegistry_test.cpp)
    target_include_directories(HostParameterRegistry_test PRIVATE
        "${CORE_DIR}"
        "${SIM_DIR}"
        Source
    )
    add_test(NAME HostParameterRegistry_test COMMAND HostParameterRegistry_test)

    add_executable(HostParameterProcessor_test tests/HostParameterProcessor_test.cpp)
    target_include_directories(HostParameterProcessor_test PRIVATE
        "${CORE_DIR}"
        "${SIM_DIR}"
        Source
    )
    target_compile_definitions(HostParameterProcessor_test PRIVATE
        FROGGERS_VST_IS_SYNTH=1
        FROGGERS_EXPECT_HOST_PARAM_COUNT=107
    )
    add_test(NAME HostParameterProcessor_test COMMAND HostParameterProcessor_test)
endif()

set(DESKTOP_V2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../desktop-v2")
set(FROGGERS_V2_UI_SOURCES
    "${DESKTOP_V2_DIR}/Source/AudioEngine.cpp"
    "${DESKTOP_V2_DIR}/Source/DesktopV2HostCallbacks.cpp"
    "${DESKTOP_V2_DIR}/Source/control/FroggersV2ControlCore.cpp"
    "${DESKTOP_V2_DIR}/Source/control/FroggersV2HostBridge.cpp"
    "${DESKTOP_V2_DIR}/Source/control/FroggersV2AppCoreFacade.cpp"
    "${DESKTOP_V2_DIR}/Source/runtime/FilePatchRuntimeState.cpp"
    "${DESKTOP_V2_DIR}/Source/runtime/DesktopV2AudioStateProjection.cpp"
    "${DESKTOP_V2_DIR}/Source/control/MidiCvAssignmentTable.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/EncoderRingComponent.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/ModLanePicker.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/SubmodulePagePanel.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/AdsrPagePanel.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/PageCarouselComponent.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/GlobalOscilloscopeDisplay.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/GlobalStripV2.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/PerformanceBandV2.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/SequencerPanelComponent.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/CvScopeDisplay.cpp"
    "${DESKTOP_V2_DIR}/Source/ui/DesktopV2LookAndFeel.cpp"
)

option(BUILD_VST_V2 "Build FroggersTiga VST3/AU v2 plugin (local-only)" OFF)

if(BUILD_VST_V2 AND EXISTS "${DESKTOP_V2_DIR}/Source/PluginProcessorV2.cpp")
    set(FROGGERS_PLUGIN_V2_IS_SYNTH TRUE)
    if(NOT FROGGERS_PLUGIN_V2_IS_SYNTH)
        message(FATAL_ERROR "FroggersTigaPluginV2 requires IS_SYNTH TRUE")
    endif()

    juce_add_plugin(FroggersTigaPluginV2
        PRODUCT_NAME "FroggersTigaV2"
        COMPANY_NAME "FroggersTiga"
        BUNDLE_ID "com.froggers.tiga.plugin.v2"
        PLUGIN_MANUFACTURER_CODE FrTi
        PLUGIN_CODE FrT2
        FORMATS VST3 AU
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
    )

    target_sources(FroggersTigaPluginV2 PRIVATE
        "${DESKTOP_V2_DIR}/Source/PluginProcessorV2.cpp"
        "${DESKTOP_V2_DIR}/Source/PluginEditorV2.cpp"
        "${DESKTOP_V2_DIR}/Source/HostedMainComponentV2.cpp"
        "${DESKTOP_V2_DIR}/Source/HostParameterRegistryV2.cpp"
        "${DESKTOP_V2_DIR}/Source/MidiCvSettingsComponent.cpp"
        Source/AudioRecorder.cpp
        Source/AudioSettingsComponent.cpp
        ${FROGGERS_V2_UI_SOURCES}
    )

    target_include_directories(FroggersTigaPluginV2 PRIVATE
        "${CORE_DIR}"
        "${SIM_DIR}"
        "${DESKTOP_V2_DIR}/Source"
        "${DESKTOP_V2_DIR}/Source/ui"
        Source
    )

    target_compile_definitions(FroggersTigaPluginV2 PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    if(FROGGERS_V2_BUNDLED_IBM_PLEX)
        target_compile_definitions(FroggersTigaPluginV2 PRIVATE FROGGERS_V2_BUNDLED_IBM_PLEX=1)
    endif()

    if(MSVC)
        target_compile_definitions(FroggersTigaPluginV2 PRIVATE _USE_MATH_DEFINES)
    endif()

    target_link_libraries(FroggersTigaPluginV2 PRIVATE
        FroggersTigaAssets
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )

    add_executable(HostParameterProcessorV2_test
        "${DESKTOP_V2_DIR}/tests/HostParameterProcessorV2_test.cpp"
        "${DESKTOP_V2_DIR}/Source/control/FroggersV2ControlCore.cpp"
        "${DESKTOP_V2_DIR}/Source/control/FroggersV2HostBridge.cpp"
    )
    target_include_directories(HostParameterProcessorV2_test PRIVATE
        "${CORE_DIR}"
        "${SIM_DIR}"
        "${DESKTOP_V2_DIR}/Source"
        Source
    )
    target_compile_definitions(HostParameterProcessorV2_test PRIVATE
        FROGGERS_VST_IS_SYNTH=1
        FROGGERS_EXPECT_HOST_PARAM_COUNT_V2=142
    )
    add_test(NAME HostParameterProcessorV2_test COMMAND HostParameterProcessorV2_test)
endif()
