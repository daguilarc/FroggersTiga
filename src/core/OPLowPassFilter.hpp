#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>

struct OPLowPassFilter
{
    static constexpr float x_maxCutoff = 0.499f;

    float m_alpha;
    float m_output;

    OPLowPassFilter()
        : m_alpha(0.0f)
        , m_output(0.0f)
    {
    }

    float Process(float input)
    {
        m_output = m_alpha * input + (1.0f - m_alpha) * m_output;
        return m_output;
    }

    void SetAlphaFromNatFreq(float cyclesPerSample)
    {
        cyclesPerSample = std::min(x_maxCutoff, cyclesPerSample);
        assert(cyclesPerSample > 0.0f);
        static constexpr float x_twoPi = 6.28318530717958647692f;
        m_alpha = 1.0f - std::exp(-x_twoPi * cyclesPerSample);
    }
};
