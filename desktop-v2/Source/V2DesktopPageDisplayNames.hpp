#pragma once

#include <array>
#include <cstdint>

// Desktop-v2-owned page/row label authority (design decision D10).
//
// This is a DELIBERATE FORK of the shared sim/V2ParamDisplayNames.hpp. The shared
// table still carries the Random S&H module page at index 1 (7 pages total) because
// the web/wasm V2 host and v1 desktop both consume it and both still ship that page;
// those products are out of scope for this change. Desktop-v2 deleted the Random S&H
// module page and its bag parameters (Step chance / Deja vu / Bag size / Slew /
// Spread / Bias), keeping Random S&H 1/2 only as modulation lanes, so it needs its own
// 6-page table:
//   0 Audio, 1 Reverb, 2 Filter, 3 Drive, 4 Delay, 5 Pair-AR.
//
// The two tables are reconciled during the operator's next task (wasm/web V2 host
// migration, D10). Until then:
//   * Do NOT point web/v1/shared-engine code at this header.
//   * Do NOT edit sim/V2ParamDisplayNames.hpp to match — the divergence is intentional.
//   * Engine-facing, PageManager-indexed callers (DesktopHostIO/PagedHostIO V2 fuego
//     config, V2EngineSetup, DelayState) MUST keep using the shared authority: the
//     engine PageManager still contains the Marbles page at PM index 1, so its page
//     indices are PM-aligned with the 7-page shared table, not this UI-indexed one.
namespace V2DesktopPageDisplayNames
{
constexpr uint8_t kV2NumHostPages = 6;
constexpr uint8_t kV2ExpandedNumRows = 10;

inline uint8_t CrispyRowForPage(uint8_t hostPage)
{
    // Indexed by desktop-v2 UI page: 0 Audio, 1 Reverb, 2 Filter, 3 Drive, 4 Delay,
    // 5 Pair-AR. Audio keeps its in-page Crispy at row 7; the expanded FX pages at
    // row 9; Pair-AR at row 6.
    static constexpr uint8_t kCrispyRow[kV2NumHostPages] = {7, 9, 9, 9, 9, 6};
    if (hostPage >= kV2NumHostPages)
    {
        return kV2ExpandedNumRows;
    }
    return kCrispyRow[hostPage];
}

inline constexpr std::array<const char*, 5> kHostPageLabels{{
    "Audio", "Reverb", "Filter", "Drive", "Delay",
}};

inline constexpr std::array<const char*, 7> kPairArRowLabels{{
    "Atk1", "Rel1", "Atk2", "Rel2", "Atk3", "Rel3", "Crispy",
}};

inline constexpr std::array<std::array<const char*, 3>, 4> kExpansionTailRowLabels{{
    {"Mod depth", "Hold", "Crispy"},
    {"Comb/Peak", "Scoop", "Crispy"},
    {"Blend", "Phase", "Crispy"},
    {"Color", "Halo", "Crispy"},
}};

inline constexpr std::array<std::array<const char*, 8>, 5> kHostRowGrid{{
    {"VCO1", "VCO2", "VCO3", "Cross-coupler", "Phase mod 1", "Phase mod 2", "Phase mod 3", "Crispy"},
    {"Wet/dry", "Room size", "Decay", "Pre-delay", "Damping", "Stereo width", "Diffusion", "Crispy"},
    {"Comb offset", "Peak freq", "Peak gain", "Peak Q", "Comb delay", "Comb feedback", "Comb LP", "Crispy"},
    {"Drive", "Shape", "SRR 1", "SRR 2", "XOR", "Bit depth", "Fuzz", "Crispy"},
    {"Delay time", "Send", "Feedback", "Stereo width", "Detune", "Mod depth", "Wet mix", "Crispy"},
}};

inline const char* forHostPage(uint8_t hostPage)
{
    if (hostPage == 5)
    {
        return "Pair-AR";
    }
    if (hostPage >= kHostPageLabels.size())
    {
        return "";
    }
    return kHostPageLabels[hostPage];
}

inline const char* forHostPageRow(uint8_t hostPage, uint8_t row)
{
    if (hostPage == 0)
    {
        if (row >= kHostRowGrid[0].size())
        {
            return "";
        }
        return kHostRowGrid[0][row];
    }

    if (hostPage == 5)
    {
        if (row >= kPairArRowLabels.size())
        {
            return "";
        }
        return kPairArRowLabels[row];
    }

    if (hostPage >= 1 && hostPage <= 4)
    {
        if (row < 7)
        {
            if (hostPage >= kHostRowGrid.size() || row >= kHostRowGrid[hostPage].size())
            {
                return "";
            }
            return kHostRowGrid[hostPage][row];
        }
        if (row >= 7 && row <= 9)
        {
            return kExpansionTailRowLabels[hostPage - 1][row - 7];
        }
        return "";
    }

    return "";
}
} // namespace V2DesktopPageDisplayNames
