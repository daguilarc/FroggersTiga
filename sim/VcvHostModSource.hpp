#pragma once

#include "RGen.hpp"

#include <cstdint>

inline uint8_t PickVcvRandomModIndex(RGen& rgen)
{
    if (rgen.UniGen() < 0.5f)
    {
        return 255;
    }
    static constexpr uint8_t kPool[] = {4, 5, 6};
    return kPool[rgen.RangeGen(3)];
}
