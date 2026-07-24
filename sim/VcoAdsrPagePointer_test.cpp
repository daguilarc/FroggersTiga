// Task 7.5 prerequisite fix (design D15): SetVcoAdsrState's ADSR page pointer
// must target the same PM PageManager page (index 5, the shared engine's
// Audio=0/Marbles=1/Reverb=2/Filter=3/Drive=4/ADSR=5 layout) that the host's
// SetPageKnob(page, position, value) writes ADSR knobs to. Before this fix,
// DesktopHostIO::Init()/PagedHostIO::Init() wired the engine to
// m_pageManager.m_pages[6] -- a page never configured or written by any
// host path -- so Attack/Sustain/Release knob writes never reached the DSP
// (the envelope was silently inert). This test drives a host-param write to
// PM page 5 and asserts the engine's wired ADSR page (read back via
// FroggersEngine::GetAdsrParamForTest, a test-only accessor) sees it.
// Uses DesktopHostIO (the desktop-v2/VST host wrapper -- the production path
// that actually exposes the ADSR page to host automation) rather than
// PagedHostIO: PagedHostIO's Init() never calls PageManager::SetAllParamsTracking(),
// so a page that was never the UI-selected current page stays gated by the
// physical-knob "catch-up" tracking state and a raw SetPageKnob() write
// wouldn't take effect regardless of this fix -- that's an unrelated,
// pre-existing PagedHostIO/hardware-knob behavior, not part of task 7.5's
// scope. DesktopHostIO::Init() does call SetAllParamsTracking() (host
// automation always takes effect immediately), matching how the real VST/
// desktop-v2 host drives these knobs.
#include "DesktopHostIO.hpp"

#include <cmath>
#include <cstdio>

namespace
{
bool Near(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}
} // namespace

int main()
{
    DesktopHostIO io;
    io.m_hostKind = SimHostKind::VstV2;
    io.Init();

    const uint8_t kPmAdsrPage = 5;
    const float testValue = 0.33f;
    io.SetPageKnob(kPmAdsrPage, 0, testValue);

    const float engineSees = io.m_engine.GetAdsrParamForTest(0);
    if (!Near(engineSees, testValue))
    {
        std::printf(
            "FAIL: engine's wired ADSR page did not observe a host write to PM page %u row 0 "
            "(wrote %f, engine read %f) -- SetVcoAdsrState's page pointer is misaligned\n",
            kPmAdsrPage,
            testValue,
            engineSees);
        return 1;
    }

    // A write to a different row must not leak into row 0 -- confirms this
    // is really reading the live page, not a stale/default snapshot.
    io.SetPageKnob(kPmAdsrPage, 1, 0.77f);
    const float row0StillCorrect = io.m_engine.GetAdsrParamForTest(0);
    if (!Near(row0StillCorrect, testValue))
    {
        std::printf("FAIL: row 0 changed after writing row 1 (got %f, expected %f)\n",
                    row0StillCorrect,
                    testValue);
        return 1;
    }
    const float row1 = io.m_engine.GetAdsrParamForTest(1);
    if (!Near(row1, 0.77f))
    {
        std::printf("FAIL: engine did not see the write to row 1 (got %f)\n", row1);
        return 1;
    }

    std::printf("VcoAdsrPagePointer_test OK\n");
    return 0;
}
