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

inline const char* forHostPage(uint8_t hostPage)
{
    if (hostPage == 6)
    {
        return "Pair-AR";
    }
    return ParamDisplayNames::forHostPage(hostPage);
}

inline const char* forHostPageRow(uint8_t hostPage, uint8_t row)
{
    if (hostPage == 0)
    {
        return ParamDisplayNames::forHostPageRow(0, row);
    }

    if (hostPage == 6)
    {
        static const char* kPairArRows[7] = {
            "Atk1", "Rel1", "Atk2", "Rel2", "Atk3", "Rel3", "Crispy",
        };
        if (row >= 7)
        {
            return "";
        }
        return kPairArRows[row];
    }

        if (hostPage >= 1 && hostPage <= 5)
        {
            static const char* kExpansionTail[5][3] = {
                {"Spread", "Bias", "Crispy"},
                {"Mod depth", "Hold", "Crispy"},
                {"Comb/Peak", "Scoop", "Crispy"},
                {"Blend", "Phase", "Crispy"},
                {"Color", "Halo", "Crispy"},
            };
            if (row < 7)
            {
                return ParamDisplayNames::forHostPageRow(hostPage, row);
            }
            if (row >= 7 && row <= 9)
            {
                return kExpansionTail[hostPage - 1][row - 7];
            }
            return "";
        }

    return "";
}
} // namespace V2ParamDisplayNames
