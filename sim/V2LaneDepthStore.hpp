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
