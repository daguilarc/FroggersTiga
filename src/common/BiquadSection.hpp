#pragma once

#include <cmath>

struct BiquadSection
{
    float m_b0;
    float m_b1;
    float m_b2;
    float m_a1;
    float m_a2;
    float m_x1;
    float m_x2;
    float m_y1;
    float m_y2;

    BiquadSection()
        : m_b0(1.0f)
        , m_b1(0.0f)
        , m_b2(0.0f)
        , m_a1(0.0f)
        , m_a2(0.0f)
        , m_x1(0.0f)
        , m_x2(0.0f)
        , m_y1(0.0f)
        , m_y2(0.0f)
    {
    }

    float Process(float input)
    {
        float output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
        m_x2 = m_x1;
        m_x1 = input;
        m_y2 = m_y1;
        m_y1 = output;
        return output;
    }

    void Reset()
    {
        m_x1 = 0.0f;
        m_x2 = 0.0f;
        m_y1 = 0.0f;
        m_y2 = 0.0f;
    }

    void SetCoefficients(float cosw, float sinw, float q, bool isHighPass = false)
    {
        float alpha = sinw / (2.0f * q);
        float a0 = 1.0f + alpha;
        float a2 = 1.0f - alpha;

        float b0;
        float b1;
        float b2;
        if (isHighPass)
        {
            b0 = (1.0f + cosw) * 0.5f;
            b1 = -(1.0f + cosw);
            b2 = (1.0f + cosw) * 0.5f;
        }
        else
        {
            b0 = (1.0f - cosw) * 0.5f;
            b1 = 1.0f - cosw;
            b2 = (1.0f - cosw) * 0.5f;
        }

        m_b0 = b0 / a0;
        m_b1 = b1 / a0;
        m_b2 = b2 / a0;
        m_a1 = -2.0f * cosw / a0;
        m_a2 = a2 / a0;
    }
};
