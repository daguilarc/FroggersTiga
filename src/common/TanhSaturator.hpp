#pragma once

#include <algorithm>
#include <cmath>

template<bool Normalize>
struct TanhSaturator
{
    float m_inputGain;
    float m_tanhGain;

    TanhSaturator()
        : m_inputGain(1.0f)
        , m_tanhGain(Saturate(1.0f))
    {
    }

    TanhSaturator(float gain)
        : m_inputGain(1.0f)
        , m_tanhGain(Saturate(1.0f))
    {
        SetInputGain(gain);
    }

    static float Saturate(float input)
    {
        float input2 = input * input;
        float output = input * (27.0f + input2) / (27.0f + 9.0f * input2);
        return std::max(-1.0f, std::min(1.0f, output));
    }

    void SetInputGain(float gain)
    {
        m_inputGain = gain;
        m_tanhGain = Saturate(m_inputGain);
    }

    float Process(float input)
    {
        float output = Saturate(m_inputGain * input);
        if constexpr (Normalize)
        {
            if (std::abs(m_tanhGain) < 1.0e-6f)
            {
                return output;
            }

            return output / m_tanhGain;
        }
        else
        {
            return output;
        }
    }
};
