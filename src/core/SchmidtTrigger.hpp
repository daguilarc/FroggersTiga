#pragma once

struct SchmidtTrigger
{
    float m_highThreshold;
    float m_lowThreshold;
    bool m_state;

    SchmidtTrigger(float highThreshold, float lowThreshold)
        : m_highThreshold(highThreshold)
        , m_lowThreshold(lowThreshold)
        , m_state(false)
    {
    }

    bool Process(float input)
    {
        bool risingEdge = false;

        if (input > m_highThreshold)
        {
            if (!m_state)
            {
                risingEdge = true;
            }
            m_state = true;
        }
        else if (input < m_lowThreshold)
        {
            m_state = false;
        }

        return risingEdge;
    }

    void Reset(float initialValue)
    {
        m_state = initialValue > m_highThreshold;
    }
};

