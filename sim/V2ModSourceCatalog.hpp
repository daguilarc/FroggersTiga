#pragma once

#include "SimModSource.hpp"

#include <cstdint>

inline const char* V2ModSourceLabel(uint8_t modIndex)
{
    switch (modIndex)
    {
        case 7:
            return "VCO1 EF";
        case 8:
            return "VCO2 EF";
        case 9:
            return "VCO3 EF";
        case 10:
            return "VCO1+VCO2 EF";
        case 11:
            return "VCO2+VCO3 EF";
        case 12:
            return "VCO1+VCO2+VCO3 EF";
        case 13:
            return "Random S&H 1";
        case 14:
            return "Random S&H 2";
        default:
            return "";
    }
}

inline bool IsV2ModSourceAvailable(uint8_t modIndex)
{
    return IsV2ModSourceIndex(modIndex);
}

inline bool IsV2SimAssignableModIndex(uint8_t modIndex)
{
    return IsV2ModSourceIndex(modIndex);
}
