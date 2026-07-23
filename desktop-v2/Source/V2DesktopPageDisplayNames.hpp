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
//
// Task 7.4 (D11/D12/D14, operator 2026-07-23) widened the fork: the Cross-coupler
// row is also removed from this table's Audio page only. The shared table's Audio
// page keeps the Cross-coupler row (the web V2 host + v1 desktop are unaffected —
// XCPL stays a real, functioning control there). The underlying shared engine XCPL
// parameter slot is NOT deleted anywhere (index-stability: deleting it would shift
// PM1A/PM2A/OLVL indices for Daisy/v1) — it is simply neutralized in DSP and hidden
// from this UI-facing table's Audio row grid. See
// desktop-v2/Source/HostParameterRoutingV2.hpp's engineRowForUiRow() for the row
// remap this requires (Audio UI rows 3+ skip over the still-allocated engine XCPL
// slot at engine row 3).
namespace V2DesktopPageDisplayNames
{
constexpr uint8_t kV2NumHostPages = 6;
constexpr uint8_t kV2ExpandedNumRows = 10;

inline uint8_t CrispyRowForPage(uint8_t hostPage)
{
    // Indexed by desktop-v2 UI page: 0 Audio, 1 Reverb, 2 Filter, 3 Drive, 4 Delay,
    // 5 Pair-AR. Audio's Crispy moved from row 7 to row 6 when the Cross-coupler row
    // was removed (task 7.4); the expanded FX pages keep Crispy at row 9; Pair-AR
    // at row 6.
    static constexpr uint8_t kCrispyRow[kV2NumHostPages] = {6, 9, 9, 9, 9, 6};
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

// Audio (UI page 0) only: 7 rows post-D11 (3 pitch + 3 PM + Crispy). The
// Cross-coupler row is gone; PM1/PM2/PM3/Crispy each moved up one UI row from
// their pre-7.4 position. (Task 7.8 will label VCO1/2/3 morphs "Shape" -- not
// done here, per operator scope note.)
inline constexpr std::array<const char*, 7> kAudioRowLabels{{
    "VCO1", "VCO2", "VCO3", "Phase mod 1", "Phase mod 2", "Phase mod 3", "Crispy",
}};

inline constexpr std::array<std::array<const char*, 8>, 4> kHostRowGrid{{
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
        if (row >= kAudioRowLabels.size())
        {
            return "";
        }
        return kAudioRowLabels[row];
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
        const uint8_t gridPage = static_cast<uint8_t>(hostPage - 1);
        if (row < 7)
        {
            if (gridPage >= kHostRowGrid.size() || row >= kHostRowGrid[gridPage].size())
            {
                return "";
            }
            return kHostRowGrid[gridPage][row];
        }
        if (row >= 7 && row <= 9)
        {
            return kExpansionTailRowLabels[gridPage][row - 7];
        }
        return "";
    }

    return "";
}
} // namespace V2DesktopPageDisplayNames
