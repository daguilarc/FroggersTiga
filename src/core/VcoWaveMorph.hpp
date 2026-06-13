#pragma once

#include "ModMgr.hpp"

#include <cmath>
#include <cstdint>

struct VcoWaveMorph
{
    float m_knobValue = 0.0f;
    uint8_t m_modIndex = 255;
    float m_modAmount = 0.0f;

    float GetMorph(ModMgr* modMgr) const
    {
        float knob = m_knobValue;
        if (modMgr && m_modIndex != 255)
        {
            knob = modMgr->Modulate(m_knobValue, m_modIndex, m_modAmount);
        }
        if (!std::isfinite(knob))
        {
            return 0.0f;
        }
        if (knob < 0.0f)
        {
            return 0.0f;
        }
        if (knob > 1.0f)
        {
            return 1.0f;
        }
        return knob;
    }

    void SetKnob(float value)
    {
        m_knobValue = value;
    }

    void SetMod(uint8_t modIndex, float modAmount)
    {
        m_modIndex = modIndex;
        m_modAmount = modAmount;
    }
};
