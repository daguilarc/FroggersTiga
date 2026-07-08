#include "control/MidiCvAssignmentTable.hpp"
#include "manifest/FroggersV2AppManifest.hpp"
#include "runtime/DesktopV2RuntimeProjection.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#ifndef FROGGERS_V2_REPO_ROOT
#define FROGGERS_V2_REPO_ROOT "."
#endif

namespace
{
bool contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
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
