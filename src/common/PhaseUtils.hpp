#pragma once

#include <cmath>

namespace PhaseUtils
{
struct ExpParam
{
    float m_baseParam;
    float m_expParam;
    float m_max;
    float m_base;
    float m_factor;

    ExpParam()
        : ExpParam(2.0f)
    {
    }

    ExpParam(float base)
        : m_baseParam(0.0f)
        , m_expParam(1.0f)
        , m_max(base)
        , m_base(base)
        , m_factor(1.0f)
    {
    }

    explicit ExpParam(float min, float max)
        : m_baseParam(0.0f)
        , m_expParam(min)
        , m_max(max)
        , m_base(max / min)
        , m_factor(min)
    {
    }

    float Update(float value)
    {
        if (m_baseParam != value)
        {
            m_baseParam = value;
            m_expParam = m_factor * std::pow(m_base, value);
        }

        return m_expParam;
    }

    static float Compute(float min, float max, float value)
    {
        return min * std::pow(max / min, value);
    }

    float Update(float min, float max, float value)
    {
        if (m_baseParam != value || m_max != max || m_factor != min)
        {
            m_factor = min;
            m_max = max;
            m_base = max / min;
            m_expParam = m_factor * std::pow(m_base, value);
            m_baseParam = value;
        }

        return m_expParam;
    }
};

struct ZeroedExpParam
{
    float m_base;
    float m_expParam;
    float m_baseParam;

    ZeroedExpParam(float base)
        : m_base(base)
        , m_expParam(0.0f)
        , m_baseParam(0.0f)
    {
    }

    ZeroedExpParam()
        : ZeroedExpParam(20.0f)
    {
    }

    static float Compute(float base, float value)
    {
        return (std::pow(base, value) - 1.0f) / (base - 1.0f);
    }

    float Update(float value)
    {
        if (m_baseParam != value)
        {
            m_baseParam = value;
            m_expParam = (std::pow(m_base, value) - 1.0f) / (m_base - 1.0f);
        }

        return m_expParam;
    }
};
}
