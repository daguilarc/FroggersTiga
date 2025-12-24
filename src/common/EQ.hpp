#pragma once

#include "SmartGridInclude.hpp"
#include "../../External/theallelectricsmartgrid/private/src/ButterworthFilter.hpp"
#include <cmath>

struct EQ
{
    // Low band (low shelf)
    //
    BiquadSection m_lowShelf;
    float m_lowGain;
    float m_lowFreq;

    // Low-mid band (peaking)
    //
    BiquadSection m_lowMidPeak;
    float m_lowMidGain;
    float m_lowMidFreq;
    float m_lowMidQ;

    // High-mid band (peaking)
    //
    BiquadSection m_highMidPeak;
    float m_highMidGain;
    float m_highMidFreq;
    float m_highMidQ;

    // High band (high shelf)
    //
    BiquadSection m_highShelf;
    float m_highGain;
    float m_highFreq;

    float m_sampleRate;

    EQ()
        : m_lowShelf()
        , m_lowGain(1.0f)
        , m_lowFreq(100.0f)
        , m_lowMidPeak()
        , m_lowMidGain(1.0f)
        , m_lowMidFreq(500.0f)
        , m_lowMidQ(1.0f)
        , m_highMidPeak()
        , m_highMidGain(1.0f)
        , m_highMidFreq(2000.0f)
        , m_highMidQ(1.0f)
        , m_highShelf()
        , m_highGain(1.0f)
        , m_highFreq(5000.0f)
        , m_sampleRate(48000.0f)
    {
        UpdateCoefficients();
    }

    void SetSampleRate(float sampleRate)
    {
        m_sampleRate = sampleRate;
        UpdateCoefficients();
    }

    void SetLowFreq(float freqHz)
    {
        m_lowFreq = freqHz;
        UpdateLowShelf();
    }

    void SetLowGain(float gain)
    {
        m_lowGain = gain;
        UpdateLowShelf();
    }

    void SetLowMidFreq(float freqHz)
    {
        m_lowMidFreq = freqHz;
        UpdateLowMidPeak();
    }

    void SetLowMidQ(float q)
    {
        m_lowMidQ = q;
        UpdateLowMidPeak();
    }

    void SetLowMidGain(float gain)
    {
        m_lowMidGain = gain;
        UpdateLowMidPeak();
    }

    void SetHighMidFreq(float freqHz)
    {
        m_highMidFreq = freqHz;
        UpdateHighMidPeak();
    }

    void SetHighMidQ(float q)
    {
        m_highMidQ = q;
        UpdateHighMidPeak();
    }

    void SetHighMidGain(float gain)
    {
        m_highMidGain = gain;
        UpdateHighMidPeak();
    }

    void SetHighFreq(float freqHz)
    {
        m_highFreq = freqHz;
        UpdateHighShelf();
    }

    void SetHighGain(float gain)
    {
        m_highGain = gain;
        UpdateHighShelf();
    }

    void UpdateLowShelf()
    {
        float cyclesPerSample = m_lowFreq / m_sampleRate;
        float omega = 2.0f * M_PI * cyclesPerSample;
        float cosw = std::cos(omega);
        float sinw = std::sin(omega);
        float A = std::sqrt(m_lowGain);
        float S = 1.0f;
        float beta = std::sqrt(A) / S;

        float a0 = (A + 1.0f) + (A - 1.0f) * cosw + beta * sinw;
        float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw);
        float a2 = (A + 1.0f) + (A - 1.0f) * cosw - beta * sinw;
        float b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw + beta * sinw);
        float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw);
        float b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw - beta * sinw);

        m_lowShelf.m_b0 = b0 / a0;
        m_lowShelf.m_b1 = b1 / a0;
        m_lowShelf.m_b2 = b2 / a0;
        m_lowShelf.m_a1 = a1 / a0;
        m_lowShelf.m_a2 = a2 / a0;
    }

    void UpdateLowMidPeak()
    {
        float cyclesPerSample = m_lowMidFreq / m_sampleRate;
        float omega = 2.0f * M_PI * cyclesPerSample;
        float cosw = std::cos(omega);
        float sinw = std::sin(omega);
        float A = std::sqrt(m_lowMidGain);
        float alpha = sinw / (2.0f * m_lowMidQ);

        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosw;
        float a2 = 1.0f - alpha / A;
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosw;
        float b2 = 1.0f - alpha * A;

        m_lowMidPeak.m_b0 = b0 / a0;
        m_lowMidPeak.m_b1 = b1 / a0;
        m_lowMidPeak.m_b2 = b2 / a0;
        m_lowMidPeak.m_a1 = a1 / a0;
        m_lowMidPeak.m_a2 = a2 / a0;
    }

    void UpdateHighMidPeak()
    {
        float cyclesPerSample = m_highMidFreq / m_sampleRate;
        float omega = 2.0f * M_PI * cyclesPerSample;
        float cosw = std::cos(omega);
        float sinw = std::sin(omega);
        float A = std::sqrt(m_highMidGain);
        float alpha = sinw / (2.0f * m_highMidQ);

        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosw;
        float a2 = 1.0f - alpha / A;
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosw;
        float b2 = 1.0f - alpha * A;

        m_highMidPeak.m_b0 = b0 / a0;
        m_highMidPeak.m_b1 = b1 / a0;
        m_highMidPeak.m_b2 = b2 / a0;
        m_highMidPeak.m_a1 = a1 / a0;
        m_highMidPeak.m_a2 = a2 / a0;
    }

    void UpdateHighShelf()
    {
        float cyclesPerSample = m_highFreq / m_sampleRate;
        float omega = 2.0f * M_PI * cyclesPerSample;
        float cosw = std::cos(omega);
        float sinw = std::sin(omega);
        float A = std::sqrt(m_highGain);
        float S = 1.0f;
        float beta = std::sqrt(A) / S;

        float a0 = (A + 1.0f) - (A - 1.0f) * cosw + beta * sinw;
        float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw);
        float a2 = (A + 1.0f) - (A - 1.0f) * cosw - beta * sinw;
        float b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw + beta * sinw);
        float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw);
        float b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw - beta * sinw);

        m_highShelf.m_b0 = b0 / a0;
        m_highShelf.m_b1 = b1 / a0;
        m_highShelf.m_b2 = b2 / a0;
        m_highShelf.m_a1 = a1 / a0;
        m_highShelf.m_a2 = a2 / a0;
    }

    void UpdateCoefficients()
    {
        UpdateLowShelf();
        UpdateLowMidPeak();
        UpdateHighMidPeak();
        UpdateHighShelf();
    }

    float Process(float input)
    {
        // Cascade all biquad sections in series
        //
        float output = m_lowShelf.Process(input);
        output = m_lowMidPeak.Process(output);
        output = m_highMidPeak.Process(output);
        output = m_highShelf.Process(output);
        return output;
    }
};
