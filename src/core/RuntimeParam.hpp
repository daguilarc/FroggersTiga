#pragma once

#include "OPLowPassFilter.hpp"

struct RuntimeParam
{
    float m_target;
    OPLowPassFilter m_filter;

    RuntimeParam()
        : m_target(0)
    {
        m_filter.SetAlphaFromNatFreq(1000.0f / 44100.0f);
    }

    void SetSmoothingRate(float sampleRate)
    {
        m_filter.SetAlphaFromNatFreq(1000.0f / sampleRate);
    }

    void SetTarget(float target)
    {
        m_target = target;
    }

    float Process()
    {
        return m_filter.Process(m_target);
    }
};