#pragma once

#include "ModMgr.hpp"

#include <cmath>
#include <cstddef>

inline void applyCvPresence(float* prevCv,
                            float* cvPresence,
                            ModMgr& modMgr,
                            size_t count = 4)
{
    for (size_t i = 0; i < count; i++)
    {
        float cv = modMgr.m_mods[i];
        float diff = std::fabs(cv - prevCv[i]);
        prevCv[i] = cv;
        float indicator = (0.02f < cv || 0.003f < diff) ? 1.0f : 0.0f;
        cvPresence[i] = std::max(cvPresence[i] * 0.98f, indicator);
        modMgr.m_externalCvActive[i] = 0.1f < cvPresence[i];
    }
}
