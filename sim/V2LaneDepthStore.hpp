#pragma once

// Single authority for the V2-fuego additive multi-lane modulation blend.
// The control core (later packets) and the engine (Page::GetPreFuegoValue)
// both call ApplyV2LaneMod so the formula is defined in exactly one place.

#include "Page.hpp"
#include "Parameter.hpp"
#include "PermanentModTapRack.hpp"

#include <algorithm>
#include <cstdint>
#include <cstddef>

struct V2LaneDepthStore
{
    static constexpr size_t kNumLanes = PermanentModTapRack::kNumTaps;

    float m_depths[PageManager::x_numPages][Parameter::x_numParameters][kNumLanes]{};

    float Get(uint8_t page, uint8_t position, uint8_t lane) const
    {
        if (page >= PageManager::x_numPages || position >= Parameter::x_numParameters
            || lane >= kNumLanes)
        {
            return 0.0f;
        }
        return m_depths[page][position][lane];
    }

    void Set(uint8_t page, uint8_t position, uint8_t lane, float value)
    {
        if (page >= PageManager::x_numPages || position >= Parameter::x_numParameters
            || lane >= kNumLanes)
        {
            return;
        }
        m_depths[page][position][lane] = std::min(std::max(value, -1.0f), 1.0f);
    }

    void Clear()
    {
        float* begin = &m_depths[0][0][0];
        constexpr size_t kTotal = PageManager::x_numPages * Parameter::x_numParameters * kNumLanes;
        std::fill(begin, begin + kTotal, 0.0f);
    }
};

// out = clamp( knob + Sum_{lane=0..14} ( tap[lane] * depth[page][position][lane] ), 0, 1 )
inline float ApplyV2LaneMod(float knob,
                            uint8_t page,
                            uint8_t position,
                            const V2LaneDepthStore& depths,
                            const PermanentModTapRack& taps)
{
    float sum = knob;
    for (uint8_t lane = 0; lane < V2LaneDepthStore::kNumLanes; lane++)
    {
        sum += taps.GetTap(lane) * depths.Get(page, position, lane);
    }
    return std::min(std::max(sum, 0.0f), 1.0f);
}

// Type-erased entry point matching Page.hpp's V2FuegoApplyFn signature.
// Page.hpp cannot see the complete PermanentModTapRack/V2LaneDepthStore types
// (firmware never puts sim/ on its include path), so it calls through this
// function pointer with void* instead. Callers that DO have the real types
// (DesktopHostIO, PagedHostIO, sim tests) pass &ApplyV2FuegoOpaque to
// Page::ConfigureV2Fuego. laneDepthsVoid non-null selects the multi-lane
// additive sum; null falls back to the single-source crossfade this codebase
// used before Packet 15's multi-lane model (still exercised by callers that
// configure V2Fuego without a depth store).
inline float ApplyV2FuegoOpaque(float knobValue,
                                uint8_t modIndex,
                                float modAmount,
                                uint8_t pageId,
                                uint8_t position,
                                const void* tapsVoid,
                                const void* laneDepthsVoid)
{
    const auto* taps = static_cast<const PermanentModTapRack*>(tapsVoid);
    const auto* depths = static_cast<const V2LaneDepthStore*>(laneDepthsVoid);
    if (depths != nullptr)
    {
        return ApplyV2LaneMod(knobValue, pageId, position, *depths, *taps);
    }
    if (modIndex <= PermanentModTapRack::kLastIndex)
    {
        const float tap = taps->GetTap(modIndex);
        return std::min(std::max(knobValue * (1.0f - modAmount) + tap * modAmount, 0.0f), 1.0f);
    }
    return knobValue;
}
