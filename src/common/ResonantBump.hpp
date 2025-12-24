#pragma once

#include "SmartGridInclude.hpp"
#include "../../External/theallelectricsmartgrid/private/src/ButterworthFilter.hpp"
#include <cmath>

struct ResonantBump
{
    BiquadSection m_biquad;
    
    float m_freq;
    float m_height;
    float m_width;

    ResonantBump()
        : m_biquad()
        , m_freq(1000.0f)
        , m_height(1.0f)
        , m_width(1.0f)
    {
        UpdateCoefficients();
    }
    
    void SetFreq(float freq)
    {
        m_freq = freq;
        UpdateCoefficients();
    }

    void SetHeight(float height)
    {
        m_height = height;
        UpdateCoefficients();
    }

    void SetWidth(float width)
    {
        m_width = width;
        UpdateCoefficients();
    }

    void UpdateCoefficients()
    {
        // Standard peaking EQ biquad
        // When height = 1.0 (0dB), filter is transparent
        //
        float omega = 2.0f * M_PI * m_freq;
        float cosw = std::cos(omega);
        float sinw = std::sin(omega);
        
        // A = sqrt(linear gain), height is the linear gain
        //
        float A = std::sqrt(m_height);
        
        // Q controls width, higher Q = narrower
        //
        float Q = m_width;
        float alpha = sinw / (2.0f * Q);

        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosw;
        float a2 = 1.0f - alpha / A;
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosw;
        float b2 = 1.0f - alpha * A;

        m_biquad.m_b0 = b0 / a0;
        m_biquad.m_b1 = b1 / a0;
        m_biquad.m_b2 = b2 / a0;
        m_biquad.m_a1 = a1 / a0;
        m_biquad.m_a2 = a2 / a0;
    }

    float Process(float input)
    {
        return m_biquad.Process(input);
    }
};
