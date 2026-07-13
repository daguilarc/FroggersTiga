#pragma once

#include "ModMgr.hpp"
#include "SimModSource.hpp"

#include <algorithm>
#include <cstdint>

// Manifest permanent rack: VCO 1+2, VCO 2+3, VCO 1+3, VCO 1 EF, VCO 2 EF, VCO 3 EF,
// VCO 1+2 EF, VCO 2+3 EF, LFO 1, LFO 2, LFO 3, Random S&H 1, Random S&H 2,
// External Audio (audio rate), External Audio (envelope follower).
struct PermanentModTapRack
{
    static constexpr uint8_t kFirstIndex = 0;
    static constexpr uint8_t kLastIndex = 14;
    static constexpr size_t kNumTaps = 15;

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
        SetTap(11, modMgr.m_mods[5]);
        SetTap(12, modMgr.m_mods[6]);
    }
};

inline float GetSimCvOut(SimHostKind hostKind,
                         const ModMgr& modMgr,
                         const PermanentModTapRack& taps,
                         size_t modIndex)
{
    if (IsV2SimHostKind(hostKind))
    {
        if (modIndex <= PermanentModTapRack::kLastIndex)
        {
            return taps.GetTap(static_cast<uint8_t>(modIndex));
        }
        return 0.0f;
    }
    if (modIndex < ModMgr::x_numMods)
    {
        return modMgr.m_mods[modIndex];
    }
    return 0.0f;
}
