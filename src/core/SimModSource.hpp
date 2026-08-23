#pragma once

#include <cstdint>

inline bool IsPermanentModSourceIndex(uint8_t laneIndex)
{
    return laneIndex <= 14;
}

inline bool IsSimAssignableModIndex(uint8_t modIndex)
{
    return modIndex == 0 || modIndex == 1 || modIndex == 4 || modIndex == 5 || modIndex == 6;
}

inline bool IsValidSimModAssignment(uint8_t modIndex)
{
    return modIndex == 255 || IsSimAssignableModIndex(modIndex);
}
