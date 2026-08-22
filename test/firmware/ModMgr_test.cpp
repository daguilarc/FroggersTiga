#include "ModMgr.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    ModMgr mgr;
    mgr.m_externalCvActive[0] = true;
    mgr.m_mods[4] = 1.0f;
    const float base = 0.5f;

    if (mgr.Modulate(base, 255, 1.0f) != base)
    {
        std::printf("FAIL: Modulate index 255 should return knob unchanged\n");
        return 1;
    }

    if (mgr.Modulate(base, -1, 1.0f) != base)
    {
        std::printf("FAIL: Modulate negative index should return knob unchanged\n");
        return 1;
    }

    if (mgr.Modulate(base, static_cast<int>(ModMgr::x_numMods), 1.0f) != base)
    {
        std::printf("FAIL: Modulate index == x_numMods should return knob unchanged\n");
        return 1;
    }

    if (mgr.Modulate(base, 100, 1.0f) != base)
    {
        std::printf("FAIL: Modulate out-of-range index should return knob unchanged\n");
        return 1;
    }

    const float modulated = mgr.Modulate(base, 4, 1.0f);
    if (std::fabs(modulated - 1.0f) > 0.001f)
    {
        std::printf("FAIL: Modulate valid index 4 expected ~1 got %f\n", modulated);
        return 1;
    }

    mgr.m_externalCvActive[0] = false;
    if (mgr.Modulate(base, 0, 1.0f) != base)
    {
        std::printf("FAIL: Modulate index 0 with inactive external CV should return knob unchanged\n");
        return 1;
    }

    std::printf("ModMgr_test OK\n");
    return 0;
}
