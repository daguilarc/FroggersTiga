#include "ModLedBrightness.hpp"

#include <cmath>
#include <cstdio>

namespace
{
bool expectNear(float actual, float expected, const char* label)
{
    if (std::fabs(actual - expected) > 0.0001f)
    {
        std::printf("FAIL: %s expected %f got %f\n", label, expected, actual);
        return false;
    }
    return true;
}
} // namespace

int main()
{
    if (!expectNear(ModLedDisplayBrightness(-0.1f, true), 0.f, "negative active CV"))
    {
        return 1;
    }
    if (!expectNear(ModLedDisplayBrightness(0.f, true), 0.f, "zero active CV"))
    {
        return 1;
    }
    if (!expectNear(ModLedDisplayBrightness(0.275f, true), 0.25f, "mid active CV"))
    {
        return 1;
    }
    if (!expectNear(ModLedDisplayBrightness(0.55f, true), 1.f, "reference active CV"))
    {
        return 1;
    }
    if (!expectNear(ModLedDisplayBrightness(0.8f, false), 0.f, "inactive transport"))
    {
        return 1;
    }
    if (!expectNear(ModLedDisplayBrightness(1.f, true), 1.f, "full-scale active CV"))
    {
        return 1;
    }
    if (!expectNear(ModLedDisplayBrightness(1.2f, true), 1.f, "above-range active CV"))
    {
        return 1;
    }

    std::printf("ModLedBrightness_test OK\n");
    return 0;
}
