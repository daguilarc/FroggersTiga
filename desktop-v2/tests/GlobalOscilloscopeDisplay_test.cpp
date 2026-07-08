#include "ui/DesktopV2ScopeUiTimer.hpp"
#include "ui/GlobalOscilloscopeDisplay.hpp"
#include "manifest/FroggersV2AppManifest.hpp"

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
    if (desktop_v2::kDesktopV2ScopeUiTimerHz != 15)
    {
        std::printf("FAIL: kDesktopV2ScopeUiTimerHz expected 15 got %d\n",
                    desktop_v2::kDesktopV2ScopeUiTimerHz);
        return 1;
    }

    if (froggers_v2::manifest::kOscilloscopeTaps.size() != 3)
    {
        std::printf("FAIL: expected three manifest oscilloscope taps\n");
        return 1;
    }

    const std::string mainH = readTextFile("desktop-v2/Source/MainComponent.h");
    const std::string hostedH = readTextFile("desktop-v2/Source/HostedMainComponentV2.h");
    const std::string globalCpp = readTextFile("desktop-v2/Source/ui/GlobalOscilloscopeDisplay.cpp");
    const std::string transportLayout = readTextFile("desktop-v2/Source/ui/DesktopV2TransportLayout.hpp");

    if (!contains(mainH, "GlobalOscilloscopeDisplay m_globalOscilloscope"))
    {
        std::printf("FAIL: MainComponent does not own GlobalOscilloscopeDisplay\n");
        return 1;
    }
    if (!contains(hostedH, "GlobalOscilloscopeDisplay m_globalOscilloscope"))
    {
        std::printf("FAIL: HostedMainComponentV2 does not own GlobalOscilloscopeDisplay\n");
        return 1;
    }
    if (!contains(globalCpp, "kOscilloscopeTaps"))
    {
        std::printf("FAIL: GlobalOscilloscopeDisplay does not read manifest oscilloscope taps\n");
        return 1;
    }
    if (!contains(globalCpp, "kDesktopV2ScopeUiTimerHz"))
    {
        std::printf("FAIL: GlobalOscilloscopeDisplay does not use named scope UI timer\n");
        return 1;
    }
    if (!contains(globalCpp, "setSourceGroup"))
    {
        std::printf("FAIL: GlobalOscilloscopeDisplay missing source-group switching hook\n");
        return 1;
    }
    if (!contains(transportLayout, "scope.setBounds"))
    {
        std::printf("FAIL: transport layout does not place scope in transport band\n");
        return 1;
    }

    GlobalOscilloscopeDisplay scope;
    scope.setSourceGroup(GlobalOscilloscopeSourceGroup::Lfo);
    if (scope.sourceGroup() != GlobalOscilloscopeSourceGroup::Lfo)
    {
        std::printf("FAIL: source group setter did not update active group\n");
        return 1;
    }

    std::printf("PASS: GlobalOscilloscopeDisplay_test\n");
    return 0;
}
