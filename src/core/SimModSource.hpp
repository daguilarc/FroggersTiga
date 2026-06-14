#pragma once

#include "CvMidiBridge.hpp"
#include "RGen.hpp"

#include <cstdint>

enum class SimModSource : uint8_t
{
    None = 255,
    MidiCc1 = 0,
    MidiCc2 = 1,
    VcoFeat = 4,
    Marbles1 = 5,
    Marbles2 = 6
};

inline bool IsSimAssignableModIndex(uint8_t modIndex)
{
    return modIndex == 0 || modIndex == 1 || modIndex == 4 || modIndex == 5 || modIndex == 6;
}

inline bool IsValidSimModAssignment(uint8_t modIndex)
{
    return modIndex == 255 || IsSimAssignableModIndex(modIndex);
}

inline bool IsSimModSourceAvailable(uint8_t modIndex, const CvMidiBridge& bridge)
{
    if (modIndex == 0 || modIndex == 1)
    {
        return bridge.isCcModIndexEnabled(modIndex);
    }
    return IsSimAssignableModIndex(modIndex);
}

inline SimModSource CoreIndexToSimModSource(uint8_t modIndex)
{
    switch (modIndex)
    {
        case 0:
            return SimModSource::MidiCc1;
        case 1:
            return SimModSource::MidiCc2;
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

inline uint8_t PickSimRandomModIndex(RGen& rgen, const CvMidiBridge& bridge)
{
    if (rgen.UniGen() < 0.5f)
    {
        return 255;
    }
    static constexpr uint8_t kPool[] = {0, 1, 4, 5, 6};
    uint8_t available[5];
    uint8_t count = 0;
    for (uint8_t idx : kPool)
    {
        if (IsSimModSourceAvailable(idx, bridge))
        {
            available[count++] = idx;
        }
    }
    if (count == 0)
    {
        return 255;
    }
    return available[rgen.RangeGen(count)];
}
