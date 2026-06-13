#pragma once

#include "RGen.hpp"

#include <cstdint>

enum class SimModSource : uint8_t
{
    None = 255,
    Midi = 0,
    VcoFeat = 4,
    Marbles1 = 5,
    Marbles2 = 6
};

inline bool IsSimAssignableModIndex(uint8_t modIndex)
{
    return modIndex == 0 || modIndex == 4 || modIndex == 5 || modIndex == 6;
}

inline bool IsValidSimModAssignment(uint8_t modIndex)
{
    return modIndex == 255 || IsSimAssignableModIndex(modIndex);
}

inline SimModSource CoreIndexToSimModSource(uint8_t modIndex)
{
    switch (modIndex)
    {
        case 0:
            return SimModSource::Midi;
        case 4:
            return SimModSource::VcoFeat;
        case 5:
            return SimModSource::Marbles1;
        case 6:
            return SimModSource::Marbles2;
        default:
            return SimModSource::None;
    }
}

inline uint8_t SimModSourceToCoreIndex(SimModSource source)
{
    return static_cast<uint8_t>(source);
}

inline uint8_t PickSimRandomModIndex(RGen& rgen)
{
    if (rgen.UniGen() < 0.5f)
    {
        return 255;
    }
    static constexpr uint8_t kPool[] = {0, 4, 5, 6};
    return kPool[rgen.RangeGen(4)];
}
