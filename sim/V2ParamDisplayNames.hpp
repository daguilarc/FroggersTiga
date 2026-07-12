#pragma once

#include "ParamDisplayNames.hpp"

#include <array>
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

// Single authority for V2 product page/row display names. Callers below
// sim/ (DelayState, PagedHostIO, wasm bindings) and above it
// (desktop-v2's manifest, which delegates here) both need this data;
// it lives here so neither depends on the other in the wrong direction.
inline constexpr std::array<const char*, 6> kLegacyHostPageLabels{{
    "Audio", "Random S&H", "Reverb", "Filter", "Drive", "Delay",
}};

inline constexpr std::array<const char*, 7> kPairArRowLabels{{
    "Atk1", "Rel1", "Atk2", "Rel2", "Atk3", "Rel3", "Crispy",
}};

inline constexpr std::array<std::array<const char*, 3>, 5> kExpansionTailRowLabels{{
    {"Spread", "Bias", "Crispy"},
    {"Mod depth", "Hold", "Crispy"},
    {"Comb/Peak", "Scoop", "Crispy"},
    {"Blend", "Phase", "Crispy"},
    {"Color", "Halo", "Crispy"},
}};

inline constexpr std::array<std::array<const char*, 8>, 6> kLegacyHostRowGrid{{
    {"VCO1", "VCO2", "VCO3", "Cross-coupler", "Phase mod 1", "Phase mod 2", "Phase mod 3", "Crispy"},
    {"Step chance", "Deja vu 1", "Bag size 1", "Slew 1", "Deja vu 2", "Bag size 2", "Slew 2", "Crispy"},
    {"Wet/dry", "Room size", "Decay", "Pre-delay", "Damping", "Stereo width", "Diffusion", "Crispy"},
    {"Comb offset", "Peak freq", "Peak gain", "Peak Q", "Comb delay", "Comb feedback", "Comb LP", "Crispy"},
    {"Drive", "Shape", "SRR 1", "SRR 2", "XOR", "Bit depth", "Fuzz", "Crispy"},
    {"Delay time", "Send", "Feedback", "Stereo width", "Detune", "Mod depth", "Wet mix", "Crispy"},
}};

inline const char* forHostPage(uint8_t hostPage)
{
    if (hostPage == 6)
    {
        return "Pair-AR";
    }
    if (hostPage >= kLegacyHostPageLabels.size())
    {
        return "";
    }
    return kLegacyHostPageLabels[hostPage];
}

inline const char* forHostPageRow(uint8_t hostPage, uint8_t row)
{
    if (hostPage == 0)
    {
        if (row >= kLegacyHostRowGrid[0].size())
        {
            return "";
        }
        return kLegacyHostRowGrid[0][row];
    }

    if (hostPage == 6)
    {
        if (row >= kPairArRowLabels.size())
        {
            return "";
        }
        return kPairArRowLabels[row];
    }

    if (hostPage >= 1 && hostPage <= 5)
    {
        if (row < 7)
        {
            if (hostPage >= kLegacyHostRowGrid.size() || row >= kLegacyHostRowGrid[hostPage].size())
            {
                return "";
            }
            return kLegacyHostRowGrid[hostPage][row];
        }
        if (row >= 7 && row <= 9)
        {
            return kExpansionTailRowLabels[hostPage - 1][row - 7];
        }
        return "";
    }

    return "";
}
} // namespace V2ParamDisplayNames
