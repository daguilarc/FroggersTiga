#include "ModMgr.hpp"
#include "Page.hpp"
#include "PermanentModTapRack.hpp"
#include "V2FuegoStack.hpp"
#include "V2LaneDepthStore.hpp"

#include <cmath>
#include <cstdio>

static bool expectNear(float actual, float expected, const char* label)
{
    if (std::fabs(actual - expected) > 0.001f)
    {
        std::printf("FAIL: %s expected ~%f got %f\n", label, expected, actual);
        return false;
    }
    return true;
}

int main()
{
    // --- Single lane: additive attenuverter ---
    {
        V2LaneDepthStore depths;
        PermanentModTapRack taps;
        Page page{};
        page.m_pageId = 0;
        page.ConfigureV2Fuego(
            nullptr,
            9,
            &taps,
            V2FuegoFns{&ApplyV2FuegoOpaque, &V2FuegoStack::ApplyGlobal, &V2FuegoStack::ApplyMusicalRow},
            &depths);
        page.m_parameters[3].m_knobValue = 0.5f;

        taps.SetTap(3, 1.0f);
        depths.Set(0, 3, 3, 0.4f);
        if (!expectNear(page.GetParam(3), 0.9f, "single lane positive depth"))
        {
            return 1;
        }

        depths.Set(0, 3, 3, -0.4f);
        if (!expectNear(page.GetParam(3), 0.1f, "single lane negative depth"))
        {
            return 1;
        }
    }

    // --- Multi-lane additive: sum then clamp ---
    {
        V2LaneDepthStore depths;
        PermanentModTapRack taps;
        Page page{};
        page.m_pageId = 0;
        page.ConfigureV2Fuego(
            nullptr,
            9,
            &taps,
            V2FuegoFns{&ApplyV2FuegoOpaque, &V2FuegoStack::ApplyGlobal, &V2FuegoStack::ApplyMusicalRow},
            &depths);
        page.m_parameters[2].m_knobValue = 0.5f;

        taps.SetTap(0, 1.0f);
        taps.SetTap(1, 1.0f);

        depths.Set(0, 2, 0, 0.2f);
        depths.Set(0, 2, 1, 0.2f);
        if (!expectNear(page.GetParam(2), 0.9f, "multi-lane additive sum"))
        {
            return 1;
        }

        depths.Set(0, 2, 0, 0.8f);
        depths.Set(0, 2, 1, 0.8f);
        if (!expectNear(page.GetParam(2), 1.0f, "multi-lane sum clamps to 1.0"))
        {
            return 1;
        }

        depths.Set(0, 2, 0, -0.8f);
        depths.Set(0, 2, 1, -0.8f);
        if (!expectNear(page.GetParam(2), 0.0f, "multi-lane sum clamps to 0.0"))
        {
            return 1;
        }
    }

    // --- Zero-depth lanes contribute nothing ---
    {
        V2LaneDepthStore depths;
        PermanentModTapRack taps;
        Page page{};
        page.m_pageId = 0;
        page.ConfigureV2Fuego(
            nullptr,
            9,
            &taps,
            V2FuegoFns{&ApplyV2FuegoOpaque, &V2FuegoStack::ApplyGlobal, &V2FuegoStack::ApplyMusicalRow},
            &depths);
        page.m_parameters[5].m_knobValue = 0.37f;

        for (uint8_t lane = 0; lane < PermanentModTapRack::kNumTaps; lane++)
        {
            taps.SetTap(lane, 1.0f);
        }
        // depths left at default (0) for every lane.
        if (!expectNear(page.GetParam(5), 0.37f, "zero-depth lanes contribute nothing"))
        {
            return 1;
        }
    }

    // --- Legacy untouched: m_useV2Fuego == false still uses ModMgr crossfade ---
    {
        ModMgr modMgr;
        Page page{};
        page.m_pageId = 0;
        page.m_modMgr = &modMgr;
        page.m_parameters[1].m_knobValue = 0.3f;
        page.m_parameters[1].m_modIndex = 4;
        page.m_parameters[1].m_modAmount = 0.5f;
        modMgr.m_mods[4] = 0.9f;

        const float expected = modMgr.Modulate(0.3f, 4, 0.5f);
        if (!expectNear(page.GetParam(1), expected, "legacy crossfade unchanged"))
        {
            return 1;
        }
    }

    std::printf("PASS: V2LaneDepthApply_test\n");
    return 0;
}
