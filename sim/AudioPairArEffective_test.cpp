#include "AudioPairArState.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    AudioPairArState state;
    state.init(44100.0f);
    state.setKnob(0, 0.5f);
    state.setModSource(0, 0);
    state.setModDepth(0, 1.0f);

    float mods[ModMgr::x_numMods] = {};
    mods[0] = 0.0f;
    const float low = state.getEffectiveKnob(0, mods);
    mods[0] = 1.0f;
    const float high = state.getEffectiveKnob(0, mods);

    if (std::fabs(low - 0.0f) > 0.001f)
    {
        std::printf("FAIL: effective at mod 0 expected ~0 got %f\n", low);
        return 1;
    }
    if (std::fabs(high - 1.0f) > 0.001f)
    {
        std::printf("FAIL: effective at mod 1 expected ~1 got %f\n", high);
        return 1;
    }

    std::printf("AudioPairArEffective_test OK\n");
    return 0;
}
