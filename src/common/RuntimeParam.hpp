#pragma once

#include "SmartGridInclude.hpp"

struct RuntimeParam
{
    float m_target;
    OPLowPassFilter m_filter;

    RuntimeParam()
        : m_target(0)
    {
        m_filter.SetAlphaFromNatFreq(1000.0 / 48000.0);
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