#pragma once

#include "AudioPairArState.hpp"
#include "DelayState.hpp"
#include "ParamDisplayNames.hpp"

#include <cstddef>
#include <cstdint>

// Independent semantic inventory for VST/AU host parameters (task 5.1).
// Stable IDs, ranges, and defaults are authoritative here; the registry must match exactly.
namespace HostParameterInventory
{
constexpr uint8_t kNumPageManagerPages = 5;
constexpr uint8_t kNumRows = ParamDisplayNames::kNumRows;
constexpr uint8_t kDelayPage = ParamDisplayNames::kDelayHostPage;
constexpr uint8_t kPairArCount = AudioPairArState::kCount;
constexpr uint8_t kMorphCount = 3;

constexpr size_t kPageKnobCount = static_cast<size_t>(kNumPageManagerPages) * kNumRows;
constexpr size_t kPageModDepthCount = kPageKnobCount;
// v1 host contract: the delay host page exposes a fixed 8-row grid, matching the
// authored kDescriptors table below (delay_row0..delay_row7). This is intentionally
// decoupled from the shared engine's DelayState::kNumRows, which grew to 10 rows in
// commit 4e3d0a3 for the richer engine / desktop-v2 without adding v1 host parameters
// for the extra rows. Wiring this to DelayState::kNumRows over-counted kCount to 111
// and left indices 107..110 as zero-initialized (nullptr stableKey) phantom descriptors.
constexpr size_t kDelayHostRowCount = 8;
constexpr size_t kDelayKnobCount = kDelayHostRowCount;
constexpr size_t kDelayModDepthCount = kDelayHostRowCount;
constexpr size_t kPairArKnobCount = kPairArCount;
constexpr size_t kPairArModDepthCount = kPairArCount;
constexpr size_t kMorphKnobCount = kMorphCount;

constexpr size_t kCount = kPageKnobCount + kPageModDepthCount + kDelayKnobCount + kDelayModDepthCount
                        + kPairArKnobCount + kPairArModDepthCount + kMorphKnobCount;

enum class Axis : uint8_t
{
    PageKnob = 0,
    PageModDepth = 1,
    DelayKnob = 2,
    DelayModDepth = 3,
    PairArKnob = 4,
    PairArModDepth = 5,
    VcoMorph = 6
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
    const char* stableKey;
};

constexpr bool isCrispyRow(uint8_t row)
{
    return row == ParamDisplayNames::kCrispyRow;
}

constexpr float modDepthDefault()
{
    return 0.5f;
}

constexpr float morphDefault()
{
    return 0.0f;
}

constexpr float kPage0KnobDefaults[7] = {0.35f, 0.4f, 0.45f, 0.5f, 0.0f, 0.0f, 0.4f};
constexpr float kPage1KnobDefaults[7] = {1.0f, 0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f};
constexpr float kPage2KnobDefaults[7] = {0.2f, 0.4f, 0.5f, 0.1f, 0.6f, 0.2f, 0.2f};
constexpr float kPage3KnobDefaults[7] = {0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f};
constexpr float kPage4KnobDefaults[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

constexpr float pageKnobDefault(uint8_t page, uint8_t row)
{
    if (isCrispyRow(row))
    {
        return 0.0f;
    }
    switch (page)
    {
        case 0:
            return row < 7 ? kPage0KnobDefaults[row] : 0.0f;
        case 1:
            return row < 7 ? kPage1KnobDefaults[row] : 0.0f;
        case 2:
            return row < 7 ? kPage2KnobDefaults[row] : 0.0f;
        case 3:
            return row < 7 ? kPage3KnobDefaults[row] : 0.0f;
        case 4:
            return row < 7 ? kPage4KnobDefaults[row] : 0.0f;
        default:
            return 0.5f;
    }
}

constexpr float delayKnobDefault(uint8_t row)
{
    return isCrispyRow(row) ? 0.0f : 0.5f;
}

constexpr size_t axisOffset(Axis axis)
{
    switch (axis)
    {
        case Axis::PageKnob:
            return 0;
        case Axis::PageModDepth:
            return kPageKnobCount;
        case Axis::DelayKnob:
            return kPageKnobCount + kPageModDepthCount;
        case Axis::DelayModDepth:
            return kPageKnobCount + kPageModDepthCount + kDelayKnobCount;
        case Axis::PairArKnob:
            return kPageKnobCount + kPageModDepthCount + kDelayKnobCount + kDelayModDepthCount;
        case Axis::PairArModDepth:
            return kPageKnobCount + kPageModDepthCount + kDelayKnobCount + kDelayModDepthCount
                   + kPairArKnobCount;
        case Axis::VcoMorph:
            return kPageKnobCount + kPageModDepthCount + kDelayKnobCount + kDelayModDepthCount
                   + kPairArKnobCount + kPairArModDepthCount;
    }
    return 0;
}

constexpr Id makeId(Axis axis, uint8_t page, uint8_t row, uint8_t index)
{
    size_t local = 0;
    switch (axis)
    {
        case Axis::PageKnob:
        case Axis::PageModDepth:
            local = static_cast<size_t>(page) * kNumRows + row;
            break;
        case Axis::DelayKnob:
        case Axis::DelayModDepth:
            local = row;
            break;
        case Axis::PairArKnob:
        case Axis::PairArModDepth:
        case Axis::VcoMorph:
            local = index;
            break;
    }
    return static_cast<Id>(axisOffset(axis) + local);
}

constexpr Descriptor makeDescriptor(Axis axis,
                                    uint8_t page,
                                    uint8_t row,
                                    uint8_t index,
                                    float defaultNorm,
                                    const char* stableKey)
{
    return Descriptor{
        makeId(axis, page, row, index),
        axis,
        page,
        row,
        index,
        0.0f,
        1.0f,
        defaultNorm,
        stableKey,
    };
}

inline constexpr Descriptor kDescriptors[kCount] = {
    makeDescriptor(Axis::PageKnob, 0, 0, 0, pageKnobDefault(0, 0), "page0_row0_knob"),
    makeDescriptor(Axis::PageKnob, 0, 1, 0, pageKnobDefault(0, 1), "page0_row1_knob"),
    makeDescriptor(Axis::PageKnob, 0, 2, 0, pageKnobDefault(0, 2), "page0_row2_knob"),
    makeDescriptor(Axis::PageKnob, 0, 3, 0, pageKnobDefault(0, 3), "page0_row3_knob"),
    makeDescriptor(Axis::PageKnob, 0, 4, 0, pageKnobDefault(0, 4), "page0_row4_knob"),
    makeDescriptor(Axis::PageKnob, 0, 5, 0, pageKnobDefault(0, 5), "page0_row5_knob"),
    makeDescriptor(Axis::PageKnob, 0, 6, 0, pageKnobDefault(0, 6), "page0_row6_knob"),
    makeDescriptor(Axis::PageKnob, 0, 7, 0, pageKnobDefault(0, 7), "page0_row7_knob"),
    makeDescriptor(Axis::PageKnob, 1, 0, 0, pageKnobDefault(1, 0), "page1_row0_knob"),
    makeDescriptor(Axis::PageKnob, 1, 1, 0, pageKnobDefault(1, 1), "page1_row1_knob"),
    makeDescriptor(Axis::PageKnob, 1, 2, 0, pageKnobDefault(1, 2), "page1_row2_knob"),
    makeDescriptor(Axis::PageKnob, 1, 3, 0, pageKnobDefault(1, 3), "page1_row3_knob"),
    makeDescriptor(Axis::PageKnob, 1, 4, 0, pageKnobDefault(1, 4), "page1_row4_knob"),
    makeDescriptor(Axis::PageKnob, 1, 5, 0, pageKnobDefault(1, 5), "page1_row5_knob"),
    makeDescriptor(Axis::PageKnob, 1, 6, 0, pageKnobDefault(1, 6), "page1_row6_knob"),
    makeDescriptor(Axis::PageKnob, 1, 7, 0, pageKnobDefault(1, 7), "page1_row7_knob"),
    makeDescriptor(Axis::PageKnob, 2, 0, 0, pageKnobDefault(2, 0), "page2_row0_knob"),
    makeDescriptor(Axis::PageKnob, 2, 1, 0, pageKnobDefault(2, 1), "page2_row1_knob"),
    makeDescriptor(Axis::PageKnob, 2, 2, 0, pageKnobDefault(2, 2), "page2_row2_knob"),
    makeDescriptor(Axis::PageKnob, 2, 3, 0, pageKnobDefault(2, 3), "page2_row3_knob"),
    makeDescriptor(Axis::PageKnob, 2, 4, 0, pageKnobDefault(2, 4), "page2_row4_knob"),
    makeDescriptor(Axis::PageKnob, 2, 5, 0, pageKnobDefault(2, 5), "page2_row5_knob"),
    makeDescriptor(Axis::PageKnob, 2, 6, 0, pageKnobDefault(2, 6), "page2_row6_knob"),
    makeDescriptor(Axis::PageKnob, 2, 7, 0, pageKnobDefault(2, 7), "page2_row7_knob"),
    makeDescriptor(Axis::PageKnob, 3, 0, 0, pageKnobDefault(3, 0), "page3_row0_knob"),
    makeDescriptor(Axis::PageKnob, 3, 1, 0, pageKnobDefault(3, 1), "page3_row1_knob"),
    makeDescriptor(Axis::PageKnob, 3, 2, 0, pageKnobDefault(3, 2), "page3_row2_knob"),
    makeDescriptor(Axis::PageKnob, 3, 3, 0, pageKnobDefault(3, 3), "page3_row3_knob"),
    makeDescriptor(Axis::PageKnob, 3, 4, 0, pageKnobDefault(3, 4), "page3_row4_knob"),
    makeDescriptor(Axis::PageKnob, 3, 5, 0, pageKnobDefault(3, 5), "page3_row5_knob"),
    makeDescriptor(Axis::PageKnob, 3, 6, 0, pageKnobDefault(3, 6), "page3_row6_knob"),
    makeDescriptor(Axis::PageKnob, 3, 7, 0, pageKnobDefault(3, 7), "page3_row7_knob"),
    makeDescriptor(Axis::PageKnob, 4, 0, 0, pageKnobDefault(4, 0), "page4_row0_knob"),
    makeDescriptor(Axis::PageKnob, 4, 1, 0, pageKnobDefault(4, 1), "page4_row1_knob"),
    makeDescriptor(Axis::PageKnob, 4, 2, 0, pageKnobDefault(4, 2), "page4_row2_knob"),
    makeDescriptor(Axis::PageKnob, 4, 3, 0, pageKnobDefault(4, 3), "page4_row3_knob"),
    makeDescriptor(Axis::PageKnob, 4, 4, 0, pageKnobDefault(4, 4), "page4_row4_knob"),
    makeDescriptor(Axis::PageKnob, 4, 5, 0, pageKnobDefault(4, 5), "page4_row5_knob"),
    makeDescriptor(Axis::PageKnob, 4, 6, 0, pageKnobDefault(4, 6), "page4_row6_knob"),
    makeDescriptor(Axis::PageKnob, 4, 7, 0, pageKnobDefault(4, 7), "page4_row7_knob"),

    makeDescriptor(Axis::PageModDepth, 0, 0, 0, modDepthDefault(), "page0_row0_depth"),
    makeDescriptor(Axis::PageModDepth, 0, 1, 0, modDepthDefault(), "page0_row1_depth"),
    makeDescriptor(Axis::PageModDepth, 0, 2, 0, modDepthDefault(), "page0_row2_depth"),
    makeDescriptor(Axis::PageModDepth, 0, 3, 0, modDepthDefault(), "page0_row3_depth"),
    makeDescriptor(Axis::PageModDepth, 0, 4, 0, modDepthDefault(), "page0_row4_depth"),
    makeDescriptor(Axis::PageModDepth, 0, 5, 0, modDepthDefault(), "page0_row5_depth"),
    makeDescriptor(Axis::PageModDepth, 0, 6, 0, modDepthDefault(), "page0_row6_depth"),
    makeDescriptor(Axis::PageModDepth, 0, 7, 0, modDepthDefault(), "page0_row7_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 0, 0, modDepthDefault(), "page1_row0_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 1, 0, modDepthDefault(), "page1_row1_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 2, 0, modDepthDefault(), "page1_row2_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 3, 0, modDepthDefault(), "page1_row3_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 4, 0, modDepthDefault(), "page1_row4_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 5, 0, modDepthDefault(), "page1_row5_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 6, 0, modDepthDefault(), "page1_row6_depth"),
    makeDescriptor(Axis::PageModDepth, 1, 7, 0, modDepthDefault(), "page1_row7_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 0, 0, modDepthDefault(), "page2_row0_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 1, 0, modDepthDefault(), "page2_row1_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 2, 0, modDepthDefault(), "page2_row2_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 3, 0, modDepthDefault(), "page2_row3_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 4, 0, modDepthDefault(), "page2_row4_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 5, 0, modDepthDefault(), "page2_row5_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 6, 0, modDepthDefault(), "page2_row6_depth"),
    makeDescriptor(Axis::PageModDepth, 2, 7, 0, modDepthDefault(), "page2_row7_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 0, 0, modDepthDefault(), "page3_row0_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 1, 0, modDepthDefault(), "page3_row1_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 2, 0, modDepthDefault(), "page3_row2_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 3, 0, modDepthDefault(), "page3_row3_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 4, 0, modDepthDefault(), "page3_row4_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 5, 0, modDepthDefault(), "page3_row5_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 6, 0, modDepthDefault(), "page3_row6_depth"),
    makeDescriptor(Axis::PageModDepth, 3, 7, 0, modDepthDefault(), "page3_row7_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 0, 0, modDepthDefault(), "page4_row0_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 1, 0, modDepthDefault(), "page4_row1_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 2, 0, modDepthDefault(), "page4_row2_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 3, 0, modDepthDefault(), "page4_row3_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 4, 0, modDepthDefault(), "page4_row4_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 5, 0, modDepthDefault(), "page4_row5_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 6, 0, modDepthDefault(), "page4_row6_depth"),
    makeDescriptor(Axis::PageModDepth, 4, 7, 0, modDepthDefault(), "page4_row7_depth"),

    makeDescriptor(Axis::DelayKnob, kDelayPage, 0, 0, delayKnobDefault(0), "delay_row0_knob"),
    makeDescriptor(Axis::DelayKnob, kDelayPage, 1, 0, delayKnobDefault(1), "delay_row1_knob"),
    makeDescriptor(Axis::DelayKnob, kDelayPage, 2, 0, delayKnobDefault(2), "delay_row2_knob"),
    makeDescriptor(Axis::DelayKnob, kDelayPage, 3, 0, delayKnobDefault(3), "delay_row3_knob"),
    makeDescriptor(Axis::DelayKnob, kDelayPage, 4, 0, delayKnobDefault(4), "delay_row4_knob"),
    makeDescriptor(Axis::DelayKnob, kDelayPage, 5, 0, delayKnobDefault(5), "delay_row5_knob"),
    makeDescriptor(Axis::DelayKnob, kDelayPage, 6, 0, delayKnobDefault(6), "delay_row6_knob"),
    makeDescriptor(Axis::DelayKnob, kDelayPage, 7, 0, delayKnobDefault(7), "delay_row7_knob"),

    makeDescriptor(Axis::DelayModDepth, kDelayPage, 0, 0, modDepthDefault(), "delay_row0_depth"),
    makeDescriptor(Axis::DelayModDepth, kDelayPage, 1, 0, modDepthDefault(), "delay_row1_depth"),
    makeDescriptor(Axis::DelayModDepth, kDelayPage, 2, 0, modDepthDefault(), "delay_row2_depth"),
    makeDescriptor(Axis::DelayModDepth, kDelayPage, 3, 0, modDepthDefault(), "delay_row3_depth"),
    makeDescriptor(Axis::DelayModDepth, kDelayPage, 4, 0, modDepthDefault(), "delay_row4_depth"),
    makeDescriptor(Axis::DelayModDepth, kDelayPage, 5, 0, modDepthDefault(), "delay_row5_depth"),
    makeDescriptor(Axis::DelayModDepth, kDelayPage, 6, 0, modDepthDefault(), "delay_row6_depth"),
    makeDescriptor(Axis::DelayModDepth, kDelayPage, 7, 0, modDepthDefault(), "delay_row7_depth"),

    makeDescriptor(Axis::PairArKnob, 0, 0, 0, 0.5f, "pair_ar0_knob"),
    makeDescriptor(Axis::PairArKnob, 0, 0, 1, 0.5f, "pair_ar1_knob"),
    makeDescriptor(Axis::PairArKnob, 0, 0, 2, 0.5f, "pair_ar2_knob"),
    makeDescriptor(Axis::PairArKnob, 0, 0, 3, 0.5f, "pair_ar3_knob"),

    makeDescriptor(Axis::PairArModDepth, 0, 0, 0, modDepthDefault(), "pair_ar0_depth"),
    makeDescriptor(Axis::PairArModDepth, 0, 0, 1, modDepthDefault(), "pair_ar1_depth"),
    makeDescriptor(Axis::PairArModDepth, 0, 0, 2, modDepthDefault(), "pair_ar2_depth"),
    makeDescriptor(Axis::PairArModDepth, 0, 0, 3, modDepthDefault(), "pair_ar3_depth"),

    makeDescriptor(Axis::VcoMorph, 0, 0, 0, morphDefault(), "vco_morph0"),
    makeDescriptor(Axis::VcoMorph, 0, 0, 1, morphDefault(), "vco_morph1"),
    makeDescriptor(Axis::VcoMorph, 0, 0, 2, morphDefault(), "vco_morph2"),
};

constexpr const Descriptor& descriptorAt(size_t index)
{
    return kDescriptors[index];
}

constexpr size_t idToIndex(Id id)
{
    return static_cast<size_t>(id);
}

inline bool validateInventory()
{
    for (size_t i = 0; i < kCount; ++i)
    {
        const Descriptor& entry = kDescriptors[i];
        if (idToIndex(entry.id) != i)
        {
            return false;
        }
        if (entry.stableKey == nullptr || entry.stableKey[0] == '\0')
        {
            return false;
        }
        if (entry.minNorm >= entry.maxNorm)
        {
            return false;
        }
        if (entry.defaultNorm < entry.minNorm || entry.defaultNorm > entry.maxNorm)
        {
            return false;
        }
    }
    return true;
}
} // namespace HostParameterInventory
