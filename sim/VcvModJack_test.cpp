#include "ModMgr.hpp"
#include "VcvModJack.hpp"

#include <cmath>
#include <cstdio>

static bool expectNear(float actual, float expected, const char* label)
{
    if (std::fabs(actual - expected) > 0.001f)
    {
        std::printf("FAIL: %s expected %f got %f\n", label, expected, actual);
        return false;
    }
    return true;
}

int main()
{
    ModMgr modMgr;
    modMgr.m_mods[4] = 0.8f;

    const float disconnected =
        applyVcvSectionCv(0.5f, 4, 1.0f, modMgr, false, 0.f);
    if (!expectNear(disconnected, 0.8f, "disconnected internal route"))
    {
        return 1;
    }

    const float positiveNoRoute = applyVcvSectionCv(0.25f, 255, 0.f, modMgr, true, 5.f);
    if (!expectNear(positiveNoRoute, 0.75f, "+5V base 0.25 no route"))
    {
        return 1;
    }

    modMgr.m_mods[5] = 0.4f;
    const float internalPlusExternal =
        applyVcvSectionCv(0.0f, 5, 1.0f, modMgr, true, 5.f);
    if (!expectNear(internalPlusExternal, 0.9f, "internal 0.4 + 5V"))
    {
        return 1;
    }

    modMgr.m_mods[4] = 1.0f;
    const float clampHigh = applyVcvSectionCv(0.5f, 4, 1.0f, modMgr, true, 5.f);
    if (!expectNear(clampHigh, 1.0f, "clamp high"))
    {
        return 1;
    }

    modMgr.m_mods[4] = 0.0f;
    const float clampLow = applyVcvSectionCv(0.1f, 255, 0.f, modMgr, true, -2.f);
    if (!expectNear(clampLow, 0.0f, "clamp low"))
    {
        return 1;
    }

    std::printf("VcvModJack_test OK\n");
    return 0;
}
