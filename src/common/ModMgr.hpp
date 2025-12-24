#pragma once

struct ModMgr
{
    static constexpr size_t x_numMods = 7;
    float m_mods[x_numMods];

    ModMgr()
    {
        for (size_t i = 0; i < x_numMods; i++)
        {
            m_mods[i] = 0.0f;
        }
    }  

    float Modulate(float knobValue, int index, float amount)
    {
        return std::min(std::max(knobValue * (1.0f - amount) + m_mods[index] * amount, 0.0f), 1.0f);
    }
};