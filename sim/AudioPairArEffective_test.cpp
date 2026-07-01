#include "AudioPairArState.hpp"
#include "ModMgr.hpp"
#include "Page.hpp"
#include "V2ModTapBank.hpp"

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
    AudioPairArState state;
    state.init(44100.0f);
    state.setKnob(0, 0.5f);
    const float base = 0.5f;

    ModMgr modMgr;

    state.setModSource(0, 0);
    state.setModDepth(0, 1.0f);
    modMgr.m_externalCvActive[0] = true;
    modMgr.m_mods[0] = 0.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), 0.0f, "CC0 enabled low"))
    {
        return 1;
    }
    modMgr.m_mods[0] = 1.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), 1.0f, "CC0 enabled high"))
    {
        return 1;
    }

    modMgr.m_externalCvActive[0] = false;
    modMgr.m_mods[0] = 1.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "CC0 disabled"))
    {
        return 1;
    }

    state.setModSource(0, 1);
    modMgr.m_externalCvActive[1] = true;
    modMgr.m_mods[1] = 0.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), 0.0f, "CC1 enabled low"))
    {
        return 1;
    }
    modMgr.m_mods[1] = 1.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), 1.0f, "CC1 enabled high"))
    {
        return 1;
    }

    modMgr.m_externalCvActive[1] = false;
    modMgr.m_mods[1] = 1.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "CC1 disabled"))
    {
        return 1;
    }

    static constexpr uint8_t kInternal[] = {4, 5, 6};
    for (uint8_t modIndex : kInternal)
    {
        state.setModSource(0, modIndex);
        state.setModDepth(0, 1.0f);
        modMgr.m_mods[modIndex] = 0.0f;
        char labelLow[48];
        std::snprintf(labelLow, sizeof(labelLow), "internal %u low", modIndex);
        if (!expectNear(state.getEffectiveKnob(0, &modMgr), 0.0f, labelLow))
        {
            return 1;
        }
        modMgr.m_mods[modIndex] = 1.0f;
        char labelHigh[48];
        std::snprintf(labelHigh, sizeof(labelHigh), "internal %u high", modIndex);
        if (!expectNear(state.getEffectiveKnob(0, &modMgr), 1.0f, labelHigh))
        {
            return 1;
        }
    }

    state.setModSource(0, 255);
    modMgr.m_mods[4] = 1.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "None 255"))
    {
        return 1;
    }

    state.modSource[0] = 2;
    state.setModDepth(0, 1.0f);
    modMgr.m_mods[2] = 1.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "invalid index 2"))
    {
        return 1;
    }

    state.modSource[0] = 3;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "invalid index 3"))
    {
        return 1;
    }

    state.modSource[0] = static_cast<uint8_t>(ModMgr::x_numMods);
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "invalid index x_numMods"))
    {
        return 1;
    }

    state.modSource[0] = 100;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "invalid index 100"))
    {
        return 1;
    }

    if (state.getEffectiveKnob(99, &modMgr) != 0.0f)
    {
        std::printf("FAIL: out-of-range knob index expected 0\n");
        return 1;
    }

    if (!expectNear(state.getEffectiveKnob(0, nullptr), base, "null ModMgr"))
    {
        return 1;
    }

    float globalCrunchy = 0.0f;
    V2ModTapBank v2Taps{};
    Page audioPage;
    audioPage.m_pageId = 0;
    audioPage.m_modMgr = &modMgr;
    for (uint8_t i = 0; i < 8; ++i)
    {
        audioPage.InitParam("x", i, 0.0f);
    }
    audioPage.ConfigureV2Fuego(&globalCrunchy, 7, &v2Taps);
    state.setV2FuegoConfig(&audioPage, SimHostKind::Web);
    state.setModSource(0, 255);
    globalCrunchy = 0.0f;
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "Web Crunchy zero"))
    {
        return 1;
    }
    globalCrunchy = 1.0f;
    const float crunchyHigh = state.getEffectiveKnob(0, &modMgr);
    if (std::fabs(crunchyHigh - base) <= 0.001f)
    {
        std::printf("FAIL: Web Crunchy max expected difference from raw 0.5 got %f\n", crunchyHigh);
        return 1;
    }

    globalCrunchy = 0.0f;
    audioPage.m_parameters[7].m_knobValue = 1.0f;
    const float crispyHigh = state.getEffectiveKnob(0, &modMgr);
    if (std::fabs(crispyHigh - base) <= 0.001f)
    {
        std::printf("FAIL: Web Crispy max expected difference from raw 0.5 got %f\n", crispyHigh);
        return 1;
    }

    state.setV2FuegoConfig(&audioPage, SimHostKind::Desktop);
    globalCrunchy = 1.0f;
    audioPage.KnobUpdate(7, 1.0f, 255);
    if (!expectNear(state.getEffectiveKnob(0, &modMgr), base, "Desktop v1 no fuego on pair-AR"))
    {
        return 1;
    }

    state.setModSource(0, 4);
    state.setModDepth(0, 1.0f);
    modMgr.m_mods[4] = 0.25f;
    state.beginBlock(&modMgr);
    if (!expectNear(state.getEffectiveKnob(0), 0.25f, "beginBlock getEffectiveKnob"))
    {
        return 1;
    }

    state.init(44100.0f);
    state.setKnob(0, 0.5f);
    state.setModSource(0, 4);
    state.setModDepth(0, 1.0f);
    modMgr.m_mods[4] = 1.0f;
    state.beginBlock(&modMgr);
    state.tickSmoothers();
    if (!expectNear(state.getEffectiveSmoothed(0), 1.0f, "tickSmoothers via ModMgr"))
    {
        return 1;
    }

    std::printf("AudioPairArEffective_test OK\n");
    return 0;
}
