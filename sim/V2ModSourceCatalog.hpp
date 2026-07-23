#pragma once

#include "SimModSource.hpp"

#include <cstdint>

// Permanent modulation rack lanes 0-14 align with froggers_v2::manifest::kPermanentModulationSources.
inline const char* permanentModSourceDisplayName(uint8_t laneIndex)
{
    switch (laneIndex)
    {
        case 0:
            return "VCO 1+2";
        case 1:
            return "VCO 2+3";
        case 2:
            return "VCO 1+3";
        case 3:
            return "VCO 1 EF";
        case 4:
            return "VCO 2 EF";
        case 5:
            return "VCO 3 EF";
        case 6:
            return "VCO 1+2 EF";
        case 7:
            return "VCO 2+3 EF";
        case 8:
            return "LFO EF 1";
        case 9:
            return "LFO EF 2";
        case 10:
            return "LFO EF 3";
        case 11:
            return "Random S&H 1";
        case 12:
            return "Random S&H 2";
        case 13:
            return "External Audio (audio rate)";
        case 14:
            return "External Audio (envelope follower)";
        default:
            return "";
    }
}

inline bool IsV2ModSourceAvailable(uint8_t laneIndex)
{
    return IsPermanentModSourceIndex(laneIndex);
}

inline bool IsV2SimAssignableModIndex(uint8_t laneIndex)
{
    return IsPermanentModSourceIndex(laneIndex);
}
