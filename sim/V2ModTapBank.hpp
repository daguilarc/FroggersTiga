#pragma once

#include "ModMgr.hpp"
#include "SimModSource.hpp"

#include <algorithm>
#include <cstdint>

struct V2ModTapBank
{
    static constexpr uint8_t kFirstIndex = 7;
    static constexpr uint8_t kLastIndex = 14;
    static constexpr size_t kNumTaps = 8;

    float m_taps[kNumTaps]{};

    float GetTap(uint8_t modIndex) const
    {
        if (modIndex < kFirstIndex || modIndex > kLastIndex)
        {
            return 0.0f;
        }
        return m_taps[modIndex - kFirstIndex];
    }

    void SetTap(uint8_t modIndex, float value)
    {
        if (modIndex < kFirstIndex || modIndex > kLastIndex)
        {
            return;
        }
        m_taps[modIndex - kFirstIndex] = std::min(std::max(value, 0.0f), 1.0f);
    }

    void SyncMarblesFromModMgr(const ModMgr& modMgr)
    {
        SetTap(13, modMgr.m_mods[5]);
        SetTap(14, modMgr.m_mods[6]);
    }
};

inline float GetSimCvOut(SimHostKind hostKind,
                         const ModMgr& modMgr,
                         const V2ModTapBank& taps,
                         size_t modIndex)
{
    if (modIndex < ModMgr::x_numMods)
    {
        return modMgr.m_mods[modIndex];
    }
    if (IsV2SimHostKind(hostKind))
    {
        return taps.GetTap(static_cast<uint8_t>(modIndex));
    }
    return 0.0f;
}
