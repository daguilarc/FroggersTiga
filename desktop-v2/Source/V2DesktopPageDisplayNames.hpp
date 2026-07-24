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
//   0 Audio, 1 Reverb, 2 Filter, 3 Drive, 4 Delay, 5 Envelope.
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
//
// Task 7.5 (operator 2026-07-24) widens the fork again: page 5's carousel/product
// label is renamed "Pair-AR" -> "Envelope" and its per-VCO row labels are expanded
// to full words (kEnvelopeRowLabels below), matching design.md's "ASR Envelope"
// section (desktop-v2-sheaf-runtime-harmonization/design.md line 113: "Full ADSR
// (decay knee) -- ASR only for Envelope"). This table still exposes 7 rows per
// VCO (Attack, Release) x3 + Crispy) -- it does NOT add Sustain rows. Attempting
// that surfaced a shared-engine gap: src/core/VcoAdsrState.hpp's Hold stage holds
// at a hardcoded 1.0f and apply() takes no sustain-level argument at all (the
// 2026-06-30 archived spec, openspec/changes/archive/2026-07-01-desktop-v2-ux-and-
// sequencer/specs/desktop-v2-adsr-page/spec.md, deliberately removed
// Stage::Sustain). Adding a real per-VCO Sustain control needs a shared-engine DSP
// change (new sustain-level state + knob wiring through VcoAdsrState/FroggersEngine/
// DesktopHostIO/PagedHostIO), which is exactly the open "Envelope engine" question
// gating desktop-v2-unified-parameter-layout/design.md's U5 section (Packet 6 sustain
// spike) -- out of scope here; see the increment-3 report for the full trace. The
// shared table's page 6 (sim/V2ParamDisplayNames.hpp, used by web/wasm V2 + v1) is
// untouched and still says "Pair-AR" -- V2PageAuthorityForkParity_test.cpp's
// byte-identical check now excludes this page from that comparison (same precedent
// as task 7.4's Audio exclusion for the Cross-coupler fork).
namespace V2DesktopPageDisplayNames
{
constexpr uint8_t kV2NumHostPages = 6;
constexpr uint8_t kV2ExpandedNumRows = 10;

inline uint8_t CrispyRowForPage(uint8_t hostPage)
{
    // Indexed by desktop-v2 UI page: 0 Audio, 1 Reverb, 2 Filter, 3 Drive, 4 Delay,
    // 5 Envelope. Audio's Crispy moved from row 7 to row 6 when the Cross-coupler row
    // was removed (task 7.4); the expanded FX pages keep Crispy at row 9; Envelope
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

// Task 7.5: full-word per-VCO labels (design.md's "Operator-visible labels use
// full words (no 'Attk'/'Rel')" requirement), replacing the former "Atk1"/"Rel1"
// abbreviations. Still Attack/Release only -- see the file-header note above for
// why Sustain rows are not added here.
inline constexpr std::array<const char*, 7> kEnvelopeRowLabels{{
    "Attack VCO1", "Release VCO1", "Attack VCO2", "Release VCO2", "Attack VCO3",
    "Release VCO3", "Crispy",
}};

inline constexpr std::array<std::array<const char*, 3>, 4> kExpansionTailRowLabels{{
    {"Mod depth", "Hold", "Crispy"},
    {"Comb/Peak", "Scoop", "Crispy"},
    {"Blend", "Phase", "Crispy"},
    {"Color", "Halo", "Crispy"},
}};

// Audio (UI page 0) only: 7 rows post-D11 (3 pitch + 3 PM + Crispy). The
// Cross-coupler row is gone; PM1/PM2/PM3/Crispy each moved up one UI row from
// their pre-7.4 position. (Task 7.8: the VCO1/2/3 waveform-morph controls are
// a separate VcoMorph axis, not a row in this PageKnob-indexed grid -- they
// are labeled "Shape" in manifest/FroggersV2AppManifest.hpp's
// formatInventoryDisplayName(), the display-name authority for that axis.)
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
        return "Envelope";
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
        if (row >= kEnvelopeRowLabels.size())
        {
            return "";
        }
        return kEnvelopeRowLabels[row];
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
