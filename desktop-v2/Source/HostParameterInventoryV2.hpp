#pragma once

#include "V2DesktopPageDisplayNames.hpp"
#include "V2ParamDisplayNames.hpp"
#include "SequencerState.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// VST v2 host parameter inventory (OpenSpec §6.2). Dual stableId + displayName per entry.
// kCount = 120: 142 (pre-Random-deletion) minus 20 removed Random S&H module-page axes
// (10 page knobs + 10 page mod depths on the former UI page 1) minus 2 removed Cross-coupler
// axes (1 page knob + 1 page mod depth on Audio row 3, task 7.4 / D11 / D14, V2-hosts-only).
// Random S&H 1/2 survive as modulation lanes only; the shared engine Marbles page is retained
// (not host-exposed) so the Random S&H mod sources still run at their default knob values
// (Sheaf-style defaults). The shared engine XCPL param slot is likewise retained (not
// host-exposed here) for Daisy/v1 index stability -- see FroggersEngine.hpp's
// m_simIndependentPm flag and HostParameterRoutingV2.hpp's engineRowForUiRow().
// Desktop-v2 page labels now come from V2DesktopPageDisplayNames (the D10 fork, widened by
// D11/D14 to also drop Audio's Cross-coupler row), not the shared 7-page V2ParamDisplayNames
// used by the web/wasm V2 host and v1.
namespace HostParameterInventoryV2
{
constexpr uint8_t kNumUiPages = 6;
constexpr uint8_t kDelayUiPage = 4;
constexpr uint8_t kAdsrUiPage = 5;
// The shared engine PageManager still carries the Marbles page at PM index 1
// (Audio 0, Marbles 1, Reverb 2, Filter 3, Drive 4, ADSR 5), so Pair-AR/Envelope
// remains PM page 5 even though its desktop-v2 UI page dropped from 6 to 5.
constexpr uint8_t kPmAdsrPage = 5;
constexpr uint8_t kMorphCount = 3;
constexpr uint8_t kSequencerCount = 5;
constexpr uint8_t kGestureLaneCount = 2;

constexpr uint8_t rowsForUiPage(uint8_t page)
{
    if (page == 0)
    {
        // Task 7.4 (D11/D14): Cross-coupler row removed (3 pitch + 3 PM + Crispy).
        return 7;
    }
    if (page == kAdsrUiPage)
    {
        // Task 7.5 (D15): per-VCO Attack/Sustain/Release triplets x3 + Crispy.
        return 10;
    }
    return 10;
}

constexpr size_t kPageRowCount = []() constexpr -> size_t {
    size_t total = 0;
    for (uint8_t page = 0; page < kNumUiPages; ++page)
    {
        total += rowsForUiPage(page);
    }
    return total;
}();

constexpr size_t kPageKnobCount = kPageRowCount;
constexpr size_t kPageModDepthCount = kPageRowCount;
constexpr size_t kGlobalCrunchyCount = 1;
constexpr size_t kMorphKnobCount = kMorphCount;
constexpr size_t kSceneBlendCount = 1;
constexpr size_t kGestureWeightCount = kGestureLaneCount;

constexpr size_t kCount = kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount
                        + kSequencerCount + kSceneBlendCount + kGestureWeightCount;

enum class Axis : uint8_t
{
    PageKnob = 0,
    PageModDepth = 1,
    GlobalCrunchy = 2,
    VcoMorph = 3,
    Sequencer = 4,
    SceneBlend = 5,
    GestureWeight = 6,
};

enum class Id : uint16_t
{
    Count = static_cast<uint16_t>(kCount)
};

struct Descriptor
{
    Id id;
    Axis axis;
    uint8_t page;
    uint8_t row;
    uint8_t index;
    float minNorm;
    float maxNorm;
    float defaultNorm;
    const char* stableId;
    const char* displayName;
};

constexpr float modDepthDefault()
{
    return 0.5f;
}

// Inverse of ExpParam::Compute(20, 20000, norm) at 30 Hz: log(30/20) / log(20000/20).
constexpr float audioVcoFrequencyDefaultNorm()
{
    return 0.058697f;
}

constexpr float vcoMorphDefault(uint8_t index)
{
    switch (index)
    {
        case 0:
            return 0.0f;
        case 1:
            return 1.0f;
        case 2:
            return 0.5f;
        default:
            return 0.0f;
    }
}

constexpr float pageKnobDefault(uint8_t page, uint8_t row)
{
    if (page == 0 && row <= 2)
    {
        return audioVcoFrequencyDefaultNorm();
    }
    if (row == V2DesktopPageDisplayNames::CrispyRowForPage(page))
    {
        return 0.0f;
    }
    if (page == kAdsrUiPage)
    {
        // Task 7.5 (D15): per-VCO triplet row order (Attack, Sustain,
        // Release) x3 -- rows 0-8; row 9 (Crispy) is handled by the
        // CrispyRowForPage check above and never reaches here.
        switch (row % 3)
        {
            case 0:
                return 0.05f;
            case 1:
                return 0.8f;
            default:
                return 0.2f;
        }
    }
    return 0.5f;
}

constexpr size_t axisOffset(Axis axis)
{
    switch (axis)
    {
        case Axis::PageKnob:
            return 0;
        case Axis::PageModDepth:
            return kPageKnobCount;
        case Axis::GlobalCrunchy:
            return kPageKnobCount + kPageModDepthCount;
        case Axis::VcoMorph:
            return kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount;
        case Axis::Sequencer:
            return kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount;
        case Axis::SceneBlend:
            return kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount
                   + kSequencerCount;
        case Axis::GestureWeight:
            return kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount
                   + kSequencerCount + kSceneBlendCount;
    }
    return 0;
}

constexpr bool decodePageRowIndex(size_t index, uint8_t& pageOut, uint8_t& rowOut)
{
    size_t cursor = 0;
    for (uint8_t page = 0; page < kNumUiPages; ++page)
    {
        const size_t rows = rowsForUiPage(page);
        if (index < cursor + rows)
        {
            pageOut = page;
            rowOut = static_cast<uint8_t>(index - cursor);
            return true;
        }
        cursor += rows;
    }
    return false;
}

struct RuntimeDescriptor : Descriptor
{
    char idBuffer[48];
    char nameBuffer[96];
};

RuntimeDescriptor buildDescriptorAt(size_t index);
bool validateInventory();
} // namespace HostParameterInventoryV2
