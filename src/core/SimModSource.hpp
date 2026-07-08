#pragma once

#include "CvMidiBridge.hpp"
#include "RGen.hpp"

#include <cstddef>
#include <cstdint>

enum class SimHostKind : uint8_t
{
    Desktop,
    Web,
    Vst,
    Vcv,
    DesktopV2,
    VstV2
};

enum class SimModSource : uint8_t
{
    None = 255,
    MidiCc1 = 0,
    MidiCc2 = 1,
    VcoFeat = 4,
    Marbles1 = 5,
    Marbles2 = 6
};

inline bool IsV2SimHostKind(SimHostKind hostKind)
{
    return hostKind == SimHostKind::DesktopV2 || hostKind == SimHostKind::VstV2;
}

inline bool UsesV2Fuego(SimHostKind hostKind)
{
    return IsV2SimHostKind(hostKind) || hostKind == SimHostKind::Web;
}

inline bool IsPermanentModSourceIndex(uint8_t laneIndex)
{
    return laneIndex <= 14;
}

inline bool IsSimAssignableModIndex(uint8_t modIndex, SimHostKind hostKind)
{
    if (IsV2SimHostKind(hostKind))
    {
        return IsPermanentModSourceIndex(modIndex);
    }
    return modIndex == 0 || modIndex == 1 || modIndex == 4 || modIndex == 5 || modIndex == 6;
}

inline bool IsSimAssignableModIndex(uint8_t modIndex)
{
    return IsSimAssignableModIndex(modIndex, SimHostKind::Desktop);
}

inline bool IsValidSimModAssignment(uint8_t modIndex, SimHostKind hostKind)
{
    return modIndex == 255 || IsSimAssignableModIndex(modIndex, hostKind);
}

inline bool IsValidSimModAssignment(uint8_t modIndex)
{
    return IsValidSimModAssignment(modIndex, SimHostKind::Desktop);
}

inline bool IsSimModSourceAvailable(uint8_t modIndex,
                                    const CvMidiBridge& bridge,
                                    SimHostKind hostKind)
{
    if (IsV2SimHostKind(hostKind))
    {
        return IsPermanentModSourceIndex(modIndex);
    }
    if (!IsSimAssignableModIndex(modIndex, hostKind))
    {
        return false;
    }
    if (modIndex == 0 || modIndex == 1)
    {
        if (hostKind == SimHostKind::Vcv || hostKind == SimHostKind::Vst)
        {
            return false;
        }
        return bridge.isCcModIndexEnabled(modIndex);
    }
    return true;
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

inline uint8_t DrawAssignableModLane(RGen& rgen,
                                     const CvMidiBridge& bridge,
                                     SimHostKind hostKind)
{
    if (rgen.UniGen() < 0.5f)
    {
        return 255;
    }

    static constexpr uint8_t kPoolAll[] = {0, 1, 4, 5, 6};
    static constexpr uint8_t kPoolInternal[] = {4, 5, 6};
    static constexpr uint8_t kPoolV2[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    const uint8_t* pool = kPoolAll;
    size_t poolSize = 5;
    if (IsV2SimHostKind(hostKind))
    {
        pool = kPoolV2;
        poolSize = 13;
    }
    else if (hostKind == SimHostKind::Vcv || hostKind == SimHostKind::Vst)
    {
        pool = kPoolInternal;
        poolSize = 3;
    }

    uint8_t available[13];
    uint8_t count = 0;
    for (size_t i = 0; i < poolSize; i++)
    {
        const uint8_t idx = pool[i];
        if (IsSimModSourceAvailable(idx, bridge, hostKind))
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
