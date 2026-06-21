#pragma once

#include <algorithm>
#include <cmath>

constexpr float kModLedFullBrightnessCv = 0.55f;

inline float ModLedDisplayBrightness(float cv01, bool active)
{
    if (!active)
    {
        return 0.f;
    }

    const float clamped = std::min(std::max(cv01, 0.f), 1.f);
    if (clamped >= kModLedFullBrightnessCv)
    {
        return 1.f;
    }

    const float normalized = clamped / kModLedFullBrightnessCv;
    return normalized * normalized;
}
