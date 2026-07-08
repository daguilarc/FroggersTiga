#pragma once

#include "PermanentModTapRack.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

struct V2EnvelopeFollowerBank
{
    static constexpr size_t kNumTaps = 5;
    static constexpr uint8_t kFirstModIndex = 3;

    float m_levels[kNumTaps]{};
    float m_attackCoeff = 0.05f;
    float m_releaseCoeff = 0.01f;

    void setSampleRate(float sampleRate)
    {
        const float sr = sampleRate > 0.0f ? sampleRate : 44100.0f;
        const float attackSec = 0.01f;
        const float releaseSec = 0.05f;
        m_attackCoeff = 1.0f - std::exp(-1.0f / (attackSec * sr));
        m_releaseCoeff = 1.0f - std::exp(-1.0f / (releaseSec * sr));
    }

    void Process(float v1, float v2, float v3, PermanentModTapRack& taps)
    {
        const float targets[kNumTaps] = {
            std::fabs(v1),
            std::fabs(v2),
            std::fabs(v3),
            std::fabs(v1 + v2) * 0.5f,
            std::fabs(v2 + v3) * 0.5f,
        };

        for (size_t i = 0; i < kNumTaps; ++i)
        {
            const float target = std::min(std::max(targets[i], 0.0f), 1.0f);
            const float coeff = (target > m_levels[i]) ? m_attackCoeff : m_releaseCoeff;
            m_levels[i] += (target - m_levels[i]) * coeff;
            taps.SetTap(static_cast<uint8_t>(kFirstModIndex + i), m_levels[i]);
        }
    }
};
