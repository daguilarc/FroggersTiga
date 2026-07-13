#pragma once

#include "CvScopeDisplay.h"

#include <array>
#include <cstddef>
#include <cstdint>

// UI-thread authority for permanent mod-lane CV histories (lanes 0..14).
// Shells push GetCvOut once per tick; GlobalOscilloscopeDisplay and detail
// underlays consume these rings without a second host sample loop.
class CvLaneHistoryStore
{
public:
    static constexpr size_t kNumLanes = 15;
    static constexpr size_t kBufferSize = CvScopeDisplay::kBufferSize;

    void pushLaneSample(uint8_t lane, float v01)
    {
        if (lane >= kNumLanes)
        {
            return;
        }
        const float clamped = v01 < 0.0f ? 0.0f : (v01 > 1.0f ? 1.0f : v01);
        m_samples[lane][m_writeIndex[lane]] = clamped;
        m_writeIndex[lane] = (m_writeIndex[lane] + 1) % kBufferSize;
        m_latest[lane] = clamped;
    }

    float latest(uint8_t lane) const
    {
        if (lane >= kNumLanes)
        {
            return 0.0f;
        }
        return m_latest[lane];
    }

    const float* samples(uint8_t lane) const
    {
        if (lane >= kNumLanes)
        {
            return nullptr;
        }
        return m_samples[lane].data();
    }

    size_t writeIndex(uint8_t lane) const
    {
        if (lane >= kNumLanes)
        {
            return 0;
        }
        return m_writeIndex[lane];
    }

    size_t bufferSize() const { return kBufferSize; }

private:
    std::array<std::array<float, kBufferSize>, kNumLanes> m_samples{};
    std::array<size_t, kNumLanes> m_writeIndex{};
    std::array<float, kNumLanes> m_latest{};
};
