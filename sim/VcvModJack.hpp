#pragma once

#include "ModMgr.hpp"

#include <algorithm>
#include <cstdint>

inline float vcvInternalEffective(float base, uint8_t modIndex, float depth, const ModMgr& modMgr)
{
    if (modIndex == 255 || depth <= 0.f)
    {
        return base;
    }
    return modMgr.Modulate(base, static_cast<int>(modIndex), depth);
}

inline float applyVcvSectionCv(float base,
                               uint8_t modIndex,
                               float depth,
                               const ModMgr& modMgr,
                               bool jackConnected,
                               float modVoltage)
{
    const float internal = vcvInternalEffective(base, modIndex, depth, modMgr);
    if (!jackConnected)
    {
        return internal;
    }
    return std::clamp(internal + modVoltage / 10.f, 0.f, 1.f);
}
