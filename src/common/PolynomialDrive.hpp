#pragma once

#include "SmartGridInclude.hpp"
#include "RuntimeParam.hpp"

struct PolynomialDrive
{
    RuntimeParam m_gain;
    RuntimeParam m_coefs[5];
    WaveTable const* m_sinTable;

    PolynomialDrive()
        : m_gain()
        , m_coefs{}
        , m_sinTable(&WaveTable::GetSine())
    {
    }

    float Process(float input)
    {
        float input2 = input * input;
        float input3 = input2 * input;
        float input4 = input3 * input;
        float input5 = input3 * input2;
        return m_gain.Process() * (input * m_coefs[0].Process() + input2 * m_coefs[1].Process() + input3 * m_coefs[2].Process() + input4 * m_coefs[3].Process() + input5 * m_coefs[4].Process());
    }

    void SetGain(float gain)
    {
        float computedGain = PhaseUtils::ExpParam::Compute(1.0, 5.0, gain);
        m_gain.SetTarget(computedGain);
    }

    void SetCoefs(float coefsKnob)
    {
        float computedGain = m_gain.m_target;
        coefsKnob = PhaseUtils::ZeroedExpParam::Compute(30.0, coefsKnob);

        // Use a space-filling curve to map a single knob to 5 coefficients
        // Using sin/cos with different frequencies and phases to explore parameter space
        //
        float phase = coefsKnob;
        float phase0 = phase * 1.0f;
        phase0 = phase0 - std::floor(phase0);
        m_coefs[0].SetTarget(1.0 + 10.0f * m_sinTable->Evaluate(phase0));

        float phase1 = phase * 1.618f + 0.25f * (computedGain - 1);
        phase1 = phase1 - std::floor(phase1);
        m_coefs[1].SetTarget(10.0f * m_sinTable->Evaluate(phase1));

        float phase2 = phase * 2.718f;
        phase2 = phase2 - std::floor(phase2);
        m_coefs[2].SetTarget(10.0f * m_sinTable->Evaluate(phase2));

        float phase3 = phase * 3.141f + 0.25f * (computedGain - 1);
        phase3 = phase3 - std::floor(phase3);
        m_coefs[3].SetTarget(10.0f * m_sinTable->Evaluate(phase3));

        float phase4 = phase * 4.669f;
        phase4 = phase4 - std::floor(phase4);
        m_coefs[4].SetTarget(10.0f * m_sinTable->Evaluate(phase4));
    }
};

struct Oversampler2x
{
    float m_prevInput;
    bool m_firstSample;
    OPLowPassFilter m_antiAlias;

    Oversampler2x()
        : m_prevInput(0.0f)
        , m_firstSample(true)
        , m_antiAlias()
    {
        m_antiAlias.SetAlphaFromNatFreq(0.4f);
    }

    template<typename ProcessFunc>
    float Process(float input, ProcessFunc processFunc)
    {
        float output;

        if (m_firstSample)
        {
            // First sample: just process it twice
            //
            float output1 = processFunc(input);
            float output2 = processFunc(input);
            m_antiAlias.Process(output1);
            float filtered2 = m_antiAlias.Process(output2);
            output = filtered2;
            m_firstSample = false;
        }
        else
        {
            // Upsample: linear interpolation between previous and current
            //
            float interpolated = (m_prevInput + input) * 0.5f;

            // Process at 2x rate: first the interpolated sample, then current
            //
            float output1 = processFunc(interpolated);
            float output2 = processFunc(input);

            // Anti-alias filter before downsampling
            //
            m_antiAlias.Process(output1);
            float filtered2 = m_antiAlias.Process(output2);

            // Downsample: take second sample
            //
            output = filtered2;
        }

        m_prevInput = input;
        return output;
    }
};

struct DigitalReorganizer
{
    uint8_t m_flip;
    uint8_t m_hashBits;

    DigitalReorganizer()
        : m_flip(0)
    {
    }

    float Process(float input)
    {
        float inputUp = (input + 1) * 128;
        uint8_t inputInt = std::round(inputUp);
        float inputRemainder = inputUp - inputInt;

        inputInt ^= m_flip;
        uint8_t mask = (1 << m_hashBits) - 1;
        uint8_t lowerBits = inputInt & mask;

        lowerBits ^= (lowerBits << 3) & mask;
        lowerBits ^= (lowerBits >> 5) & mask;
        lowerBits ^= (lowerBits << 1) & mask;

        inputInt = (inputInt & ~mask) | lowerBits;

        return (static_cast<float>(inputInt) + inputRemainder) / 128 - 1;
    }

    void SetFlip(float flipKnob)
    {
        m_flip = static_cast<uint8_t>(flipKnob * 255);
    }

    void SetHash(float hashKnob)
    {
        m_hashBits = static_cast<uint8_t>(std::round(hashKnob * 8));
    }
};

struct FrogBlock
{
    PolynomialDrive m_polynomialDrive;
    WaveTable const* m_sinTable;
    SampleRateReducer m_sampleRateReducer1;
    SampleRateReducer m_sampleRateReducer2;
    DigitalReorganizer m_digitalReorganizer;
    Oversampler2x m_oversampler;
    float m_mix;

    FrogBlock()
        : m_polynomialDrive()
        , m_sinTable(&WaveTable::GetSine())
        , m_sampleRateReducer1()
        , m_sampleRateReducer2()
        , m_digitalReorganizer()
        , m_oversampler()
        , m_mix(0)
    {
    }

    float Process(float input)
    {
        // Process chain up to digital reorganizer with 2x oversampling
        //
        float output = m_oversampler.Process(input, [this](float in) -> float
        {
            float out = m_polynomialDrive.Process(in);
            float sinIn = out / 4;
            sinIn = sinIn - std::floor(sinIn);
            return m_sinTable->Evaluate(sinIn);
        });

        output = m_digitalReorganizer.Process(output);
        output = m_sampleRateReducer1.Process(output);
        output = m_sampleRateReducer2.Process(output);
        output = output * m_mix + input * (1 - m_mix);
        return output;
    }
};