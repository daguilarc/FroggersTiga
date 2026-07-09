#include "ui/DesktopV2ScopeUiTimer.hpp"
#include "ui/GlobalOscilloscopeDisplay.hpp"
#include "ui/CvScopeDisplay.h"
#include "manifest/FroggersV2AppManifest.hpp"

#include <cstdio>
#include <cstring>
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

// Mirrors GlobalOscilloscopeDisplay's internal stableId -> permanent
// modulation-source-lane resolution so the test can independently verify
// the default traces are bound to the manifest-declared oscilloscope taps
// (and not some parallel/hardcoded table).
uint8_t permanentModIndexForStableId(const char* stableId)
{
    using froggers_v2::manifest::kPermanentModulationSources;

    if (stableId == nullptr || stableId[0] == '\0')
    {
        return UINT8_MAX;
    }
    for (uint8_t i = 0; i < kPermanentModulationSources.size(); ++i)
    {
        const char* candidate = kPermanentModulationSources[i].stableId;
        if (candidate != nullptr && std::strcmp(candidate, stableId) == 0)
        {
            return i;
        }
    }
    constexpr const char* kOscilloscopePrefix = "oscilloscope_";
    if (std::strncmp(stableId, kOscilloscopePrefix, std::strlen(kOscilloscopePrefix)) == 0)
    {
        return permanentModIndexForStableId(stableId + std::strlen(kOscilloscopePrefix));
    }
    return UINT8_MAX;
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

    // 8.2: default group is three color-coded VCO traces, bound to the
    // manifest-declared oscilloscope taps (not a hardcoded parallel table).
    GlobalOscilloscopeDisplay defaultScope;
    if (defaultScope.sourceGroup() != GlobalOscilloscopeSourceGroup::VcoDefault)
    {
        std::printf("FAIL: default source group is not VcoDefault\n");
        return 1;
    }
    if (defaultScope.traceCount() != froggers_v2::manifest::kOscilloscopeTaps.size())
    {
        std::printf("FAIL: default trace count %zu does not match manifest tap count %zu\n",
                    defaultScope.traceCount(),
                    froggers_v2::manifest::kOscilloscopeTaps.size());
        return 1;
    }
    for (size_t i = 0; i < defaultScope.traceCount(); ++i)
    {
        const auto& tap = froggers_v2::manifest::kOscilloscopeTaps[i];
        const uint8_t expectedModIndex = permanentModIndexForStableId(tap.stableId);
        if (expectedModIndex == UINT8_MAX)
        {
            std::printf("FAIL: manifest tap '%s' does not resolve to a permanent mod source\n",
                        tap.stableId);
            return 1;
        }
        if (defaultScope.traceModIndex(i) != expectedModIndex)
        {
            std::printf("FAIL: trace %zu bound to mod index %u, expected manifest-derived %u\n",
                        i,
                        defaultScope.traceModIndex(i),
                        expectedModIndex);
            return 1;
        }
    }

    // 8.3: fixed-capacity buffers. The VCO envelope-follower group has five
    // manifest sources (vco_1_ef, vco_2_ef, vco_3_ef, vco_12_ef, vco_23_ef),
    // which exceeds CvScopeDisplay::kMaxTraces (4). Binding must clamp to
    // the fixed-capacity ceiling rather than grow to fit.
    defaultScope.setSourceGroup(GlobalOscilloscopeSourceGroup::VcoEnvelopeFollower);
    if (defaultScope.traceCount() > CvScopeDisplay::kMaxTraces)
    {
        std::printf("FAIL: trace count %zu exceeds fixed capacity %zu\n",
                    defaultScope.traceCount(),
                    CvScopeDisplay::kMaxTraces);
        return 1;
    }
    if (defaultScope.traceCount() != CvScopeDisplay::kMaxTraces)
    {
        std::printf("FAIL: expected envelope-follower group to fill fixed capacity %zu, got %zu\n",
                    CvScopeDisplay::kMaxTraces,
                    defaultScope.traceCount());
        return 1;
    }

    std::printf("PASS: GlobalOscilloscopeDisplay_test\n");
    return 0;
}
