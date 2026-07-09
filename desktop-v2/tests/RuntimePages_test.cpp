#include "control/MidiCvAssignmentTable.hpp"
#include "manifest/FroggersV2AppManifest.hpp"
#include "runtime/DesktopV2RuntimeProjection.hpp"
#include "runtime/FilePatchRuntimeState.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#ifndef FROGGERS_V2_REPO_ROOT
#define FROGGERS_V2_REPO_ROOT "."
#endif

namespace
{
bool contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

std::string toLowerCopy(const std::string& value)
{
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowered;
}

bool containsCaseInsensitive(const std::string& haystack, const std::string& needle)
{
    return contains(toLowerCopy(haystack), toLowerCopy(needle));
}

std::string readTextFile(const std::string& relativePath)
{
    std::ifstream in(std::string(FROGGERS_V2_REPO_ROOT) + "/" + relativePath, std::ios::binary);
    if (!in)
    {
        return {};
    }
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}
} // namespace

int main()
{
    const RuntimeControlOverlay standaloneAudio =
        runtimeControlOverlay(RuntimeProjectionContext::DesktopStandalone,
                              RuntimeControlId::HardwareAudioOutputSelector);
    const RuntimeControlOverlay hostedAudio =
        runtimeControlOverlay(RuntimeProjectionContext::PluginHosted,
                              RuntimeControlId::HardwareAudioOutputSelector);
    const RuntimeControlOverlay hostedStatus =
        runtimeControlOverlay(RuntimeProjectionContext::PluginHosted,
                              RuntimeControlId::HostedAudioStatusPanel);

    if (standaloneAudio.hidden || !standaloneAudio.interactive)
    {
        std::printf("FAIL: standalone hardware audio selector should be visible/interactive\n");
        return 1;
    }
    if (!hostedAudio.hidden)
    {
        std::printf("FAIL: hosted hardware audio selector should be hidden\n");
        return 1;
    }
    if (hostedStatus.hidden || !hostedStatus.readOnly || !hostedStatus.interactive)
    {
        std::printf("FAIL: hosted audio status panel should be read-only and visible\n");
        return 1;
    }

    const std::string mainH = readTextFile("desktop-v2/Source/MainComponent.h");
    const std::string mainCpp = readTextFile("desktop-v2/Source/MainComponent.cpp");
    const std::string hostedH = readTextFile("desktop-v2/Source/HostedMainComponentV2.h");
    const std::string hostedCpp = readTextFile("desktop-v2/Source/HostedMainComponentV2.cpp");
    const std::string audioRuntimeH = readTextFile("desktop-v2/Source/runtime/AudioRuntimePageComponent.h");
    const std::string transportLayout = readTextFile("desktop-v2/Source/ui/DesktopV2TransportLayout.hpp");

    if (!contains(mainH, "m_fileButton") || !contains(mainH, "m_audioButton") || !contains(mainH, "m_midiButton"))
    {
        std::printf("FAIL: MainComponent missing runtime page rail buttons\n");
        return 1;
    }
    if (!contains(mainCpp, "layoutRuntimePageRail"))
    {
        std::printf("FAIL: MainComponent does not lay out right-side runtime page rail\n");
        return 1;
    }
    if (!contains(mainH, "FilePatchRuntimePageComponent") || !contains(mainH, "AudioRuntimePageComponent")
        || !contains(mainH, "ControllersRuntimePageComponent"))
    {
        std::printf("FAIL: MainComponent missing runtime page adapters\n");
        return 1;
    }
    if (contains(transportLayout, "m_midiSettings") || contains(mainCpp, "showAudioSettings")
        || contains(mainCpp, "showMidiSettings"))
    {
        std::printf("FAIL: standalone still routes audio/midi through modal dialogs\n");
        return 1;
    }
    if (!contains(hostedH, "HostedRuntimeStatusPanel"))
    {
        std::printf("FAIL: hosted shell missing read-only status panel\n");
        return 1;
    }
    if (contains(hostedCpp, "AudioSettingsComponent") || contains(hostedCpp, "MidiCvSettingsComponent")
        || contains(hostedCpp, "RecordButton"))
    {
        std::printf("FAIL: hosted shell exposes standalone hardware/record controls\n");
        return 1;
    }
    if (!contains(audioRuntimeH, "AudioSettingsComponent"))
    {
        std::printf("FAIL: runtime Audio page does not adapt AudioSettingsComponent\n");
        return 1;
    }

    // 9.4: the global top-chrome oscilloscope (Packet 8) is the only scope in the app;
    // the runtime Audio page (and the settings component it adapts) must not embed a second one.
    const std::string audioRuntimeCpp = readTextFile("desktop-v2/Source/runtime/AudioRuntimePageComponent.cpp");
    const std::string audioSettingsH = readTextFile("desktop-v2/Source/AudioSettingsComponent.h");
    const std::string audioSettingsCpp = readTextFile("desktop-v2/Source/AudioSettingsComponent.cpp");
    for (const auto& [label, text] :
         {std::pair<const char*, const std::string&>{"AudioRuntimePageComponent.h", audioRuntimeH},
          std::pair<const char*, const std::string&>{"AudioRuntimePageComponent.cpp", audioRuntimeCpp},
          std::pair<const char*, const std::string&>{"AudioSettingsComponent.h", audioSettingsH},
          std::pair<const char*, const std::string&>{"AudioSettingsComponent.cpp", audioSettingsCpp}})
    {
        if (containsCaseInsensitive(text, "scope"))
        {
            std::printf("FAIL: %s embeds a duplicate oscilloscope/scope reference\n", label);
            return 1;
        }
    }
    if (!contains(mainH, "GlobalOscilloscopeDisplay") || !contains(hostedH, "GlobalOscilloscopeDisplay"))
    {
        std::printf("FAIL: standalone/hosted shell missing the single global oscilloscope\n");
        return 1;
    }

    // 9.5: the hosted (VST/AU) shell must not construct standalone runtime page adapters or
    // the persistent nav rail at all — the hosted status panel is the only projection.
    for (const char* forbidden :
         {"FilePatchRuntimePageComponent", "AudioRuntimePageComponent", "ControllersRuntimePageComponent",
          "m_fileButton", "m_audioButton", "m_midiButton"})
    {
        if (contains(hostedH, forbidden) || contains(hostedCpp, forbidden))
        {
            std::printf("FAIL: hosted shell exposes standalone runtime page control '%s'\n", forbidden);
            return 1;
        }
    }

    // 9.2: File/Patch round trip — patch identity, dirty flag, save/load/revert, mapping
    // persistence, and log all flow through FilePatchRuntimeState the same way AudioEngine
    // drives it (savePatchToFile/loadPatchFromFile/revertPatch), without needing a live
    // audio device to exercise the runtime page's state machine.
    {
        FilePatchRuntimeState state;
        if (state.dirty || state.patchIdentity != "Untitled"
            || state.controllerMappingPersistenceResult != "Not saved")
        {
            std::printf("FAIL: FilePatchRuntimeState initial state incorrect\n");
            return 1;
        }

        // Save.
        state.markSaved("MyPatch");
        state.lastSaveResult = "Saved to /tmp/MyPatch.frogv2";
        state.appendLog(state.lastSaveResult);
        state.setControllerMappingPersistenceResult("Saved with patch");
        if (state.dirty || state.patchIdentity != "MyPatch"
            || state.controllerMappingPersistenceResult != "Saved with patch")
        {
            std::printf("FAIL: FilePatchRuntimeState save did not update identity/dirty/mapping\n");
            return 1;
        }

        // Load a different patch.
        state.markSaved("OtherPatch");
        state.lastLoadResult = "Loaded /tmp/OtherPatch.frogv2";
        state.appendLog(state.lastLoadResult);
        state.setControllerMappingPersistenceResult("Loaded with patch");
        if (state.dirty || state.patchIdentity != "OtherPatch"
            || state.controllerMappingPersistenceResult != "Loaded with patch")
        {
            std::printf("FAIL: FilePatchRuntimeState load did not update identity/dirty/mapping\n");
            return 1;
        }

        // Revert.
        state.markDirty();
        state.lastRevertResult = "Reverted to last saved snapshot.";
        state.appendLog(state.lastRevertResult);
        if (!state.dirty || state.patchIdentity != "OtherPatch")
        {
            std::printf("FAIL: FilePatchRuntimeState revert did not flag dirty state\n");
            return 1;
        }

        const juce::StringArray& log = state.logMessages();
        if (log.size() != 3 || log[0] != "Saved to /tmp/MyPatch.frogv2"
            || log[1] != "Loaded /tmp/OtherPatch.frogv2" || log[2] != "Reverted to last saved snapshot.")
        {
            std::printf("FAIL: FilePatchRuntimeState log did not capture save/load/revert in order\n");
            return 1;
        }

        // Log capacity: appendLog caps at 32 entries, evicting oldest first.
        FilePatchRuntimeState capState;
        for (int i = 0; i < 40; ++i)
        {
            capState.appendLog("entry " + juce::String(i));
        }
        const juce::StringArray& capLog = capState.logMessages();
        if (capLog.size() != 32 || capLog[0] != "entry 8" || capLog[31] != "entry 39")
        {
            std::printf("FAIL: FilePatchRuntimeState log did not cap at 32 with FIFO eviction\n");
            return 1;
        }
    }

    MidiCvAssignmentTable table;
    table.markMappingsDirty();
    const std::vector<ControllerTargetMappingRow> rows = table.buildTargetMappingRows();
    if (rows.size() != froggers_v2::manifest::kControllerTargetDeclarations.size())
    {
        std::printf("FAIL: controller model row count does not match manifest targets\n");
        return 1;
    }
    if (rows[0].targetId == nullptr
        || std::string(rows[0].targetId)
               != std::string(froggers_v2::manifest::kControllerTargetDeclarations[0].stableId))
    {
        std::printf("FAIL: controller model does not expose manifest target IDs\n");
        return 1;
    }

    std::printf("PASS: RuntimePages_test\n");
    return 0;
}
