#pragma once

#include "SmartGridInclude.hpp"

struct Comb
{
    OPLowPassFilter m_filter;
    static constexpr size_t x_size = 8192;
    float m_delayLine[x_size];
    size_t m_index;
    size_t m_delaySamples;
    float m_feedback;
    float m_output;
    TanhSaturator<true> m_saturator;

    Comb()
        : m_filter()
        , m_delayLine{0.0f}
        , m_index(0)
        , m_delaySamples(0)
        , m_feedback(0.0f)
        , m_output(0.0f)
        , m_saturator(0.5)
    {
    }

    void SetFeedback(float feedback)
    {
        m_feedback = feedback;
    }

    void SetCutoffAlpha(float cutoff)
    {
        m_filter.m_alpha = cutoff;
    }

    float Process(float input)
    {
        float output = input + m_feedback * m_saturator.Process(m_filter.Process(m_delayLine[(m_index + x_size - m_delaySamples) % x_size]));
        m_output = output;
        m_delayLine[m_index] = output;
        m_index = (m_index + 1) % x_size;
        return output;
    }

    static float GetDelaySamples(float freq)
    {
        return 1.0 / freq;
    }

    static float GetFeedback(float knob)
    {
        if (knob < 0.5f)
        {
            return -1.1 * PhaseUtils::ZeroedExpParam::Compute(0.25, 2 * (0.5f - knob));
        }
        else
        {
            return 1.1 * PhaseUtils::ZeroedExpParam::Compute(0.25, 2 * (knob - 0.5f));
        }
    }
};

struct PureDelay
{
    static constexpr size_t x_size = 8192;
    float m_delayLine[x_size];
    size_t m_index;
    float m_delaySamples;

    PureDelay()
        : m_delayLine{0.0f}
        , m_index(0)
        , m_delaySamples(0.0f)
    {
    }

    void SetDelaySamples(float freq)
    {
        m_delaySamples = 1.0 / freq;
    }

    float Process(float input)
    {
        m_delayLine[m_index] = input;
        float lowExact = m_index + x_size - m_delaySamples;
        float frac = lowExact - std::floor(lowExact);
        size_t idx0 = static_cast<size_t>(lowExact) % x_size;
        size_t idx1 = (idx0 + 1) % x_size;
        float output = m_delayLine[idx0] * (1.0f - frac) + m_delayLine[idx1] * frac;
        m_index = (m_index + 1) % x_size;
        return output;
    }
};