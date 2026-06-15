#include "DelayState.hpp"
#include "ModMgr.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    DelayState delay;
    delay.init(44100.0f);
    delay.setKnob(0, 0.5f);
    delay.setKnob(7, 0.0f);
    delay.setModSource(7, 4);
    delay.setModDepth(7, 1.0f);

    ModMgr modMgr;
    modMgr.m_externalCvActive[0] = true;
    modMgr.m_externalCvActive[1] = true;
    modMgr.m_externalCvActive[2] = true;
    modMgr.m_externalCvActive[3] = true;
    modMgr.m_mods[4] = 0.0f;
    delay.beginBlock(&modMgr);
    const float crispyLow = delay.getEffectiveKnob(7);
    modMgr.m_mods[4] = 1.0f;
    const float crispyHigh = delay.getEffectiveKnob(7);

    if (std::fabs(crispyLow - 0.0f) > 0.001f)
    {
        std::printf("FAIL: Delay Crispy effective at mod 0 expected ~0 got %f\n", crispyLow);
        return 1;
    }
    if (std::fabs(crispyHigh - 1.0f) > 0.001f)
    {
        std::printf("FAIL: Delay Crispy effective at mod 1 expected ~1 got %f\n", crispyHigh);
        return 1;
    }

    modMgr.m_mods[4] = 0.0f;
    const float row0LowCrispy = delay.getEffectiveKnob(0);
    modMgr.m_mods[4] = 1.0f;
    const float row0HighCrispy = delay.getEffectiveKnob(0);

    if (std::fabs(row0LowCrispy - row0HighCrispy) < 0.001f)
    {
        std::printf("FAIL: row 0 effective should differ when Crispy mod sweeps (got %f vs %f)\n",
                    row0LowCrispy,
                    row0HighCrispy);
        return 1;
    }

    std::printf("DelayCrispyMod_test OK\n");
    return 0;
}
