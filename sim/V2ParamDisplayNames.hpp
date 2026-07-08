#pragma once

#include "ParamDisplayNames.hpp"

#include <cstdint>

namespace V2ParamDisplayNames
{
constexpr uint8_t kV2NumHostPages = 7;
constexpr uint8_t kV2ExpandedNumRows = 10;

inline uint8_t CrispyRowForPage(uint8_t hostPage)
{
    static constexpr uint8_t kCrispyRow[kV2NumHostPages] = {7, 9, 9, 9, 9, 9, 6};
    if (hostPage >= kV2NumHostPages)
    {
        return kV2ExpandedNumRows;
    }
    return kCrispyRow[hostPage];
}

const char* forHostPage(uint8_t hostPage);
const char* forHostPageRow(uint8_t hostPage, uint8_t row);
} // namespace V2ParamDisplayNames
