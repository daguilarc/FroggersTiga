#pragma once

#include <cmath>

struct SampleRateReducer
{
    float m_freq;
    float m_phase;
    float m_output;

    SampleRateReducer()
        : m_freq(0.0f)
        , m_phase(0.0f)
        , m_output(0.0f)
    {
    }

    void SetFreq(float freq)
    {
        m_freq = freq;
    }

    float Process(float input)
    {
        if (m_freq >= 1.0f)
        {
            return input;
        }

        if (m_freq <= 0.0f)
        {
            return m_output;
        }

        m_phase += m_freq;
        if (m_phase >= 1.0f)
        {
            m_phase = m_phase - std::floor(m_phase);
            m_output = input;
        }

        return m_output;
    }
};
