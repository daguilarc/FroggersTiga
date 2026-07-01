#pragma once

#include "V2ParamDisplayNames.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// VST v2 host parameter inventory (OpenSpec §6.2). Dual stableId + displayName per entry.
// kCount = 142: 148 pre–Pair-AR minus 6 removed Sus axes (3 knob + 3 mod depth on page 6).
namespace HostParameterInventoryV2
{
constexpr uint8_t kNumUiPages = 7;
constexpr uint8_t kDelayUiPage = 5;
constexpr uint8_t kAdsrUiPage = 6;
constexpr uint8_t kPmAdsrPage = 5;
constexpr uint8_t kMorphCount = 3;
constexpr uint8_t kSequencerCount = 5;
constexpr uint8_t kGestureLaneCount = 2;

constexpr uint8_t rowsForUiPage(uint8_t page)
{
    if (page == 0)
    {
        return 8;
    }
    if (page == kAdsrUiPage)
    {
        return 7;
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
    if (row == V2ParamDisplayNames::CrispyRowForPage(page))
    {
        return 0.0f;
    }
    if (page == kAdsrUiPage)
    {
        if (row % 2 == 0)
        {
            return 0.05f;
        }
        if (row <= 5)
        {
            return 0.2f;
        }
        return 0.0f;
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

inline void buildDisplayName(char* buffer, size_t capacity, Axis axis, uint8_t page, uint8_t row, uint8_t index)
{
    if (capacity == 0)
    {
        return;
    }
    buffer[0] = '\0';

    switch (axis)
    {
        case Axis::PageKnob:
        case Axis::PageModDepth:
        {
            const char* pageName = V2ParamDisplayNames::forHostPage(page);
            const char* rowName = V2ParamDisplayNames::forHostPageRow(page, row);
            if (page == kAdsrUiPage)
            {
                std::snprintf(buffer,
                              capacity,
                              "Pair-AR/%s%s",
                              rowName,
                              axis == Axis::PageModDepth ? " depth" : "");
            }
            else
            {
                std::snprintf(buffer,
                              capacity,
                              "Module/%s/%s%s",
                              pageName,
                              rowName,
                              axis == Axis::PageModDepth ? " depth" : "");
            }
            break;
        }
        case Axis::GlobalCrunchy:
            std::snprintf(buffer, capacity, "Global/Crunchy");
            break;
        case Axis::VcoMorph:
            std::snprintf(buffer, capacity, "Global/VCO%u morph", static_cast<unsigned>(index + 1));
            break;
        case Axis::Sequencer:
            switch (index)
            {
                case 0:
                    std::snprintf(buffer, capacity, "Sequencer/BPM");
                    break;
                case 1:
                    std::snprintf(buffer, capacity, "Sequencer/PatternLength");
                    break;
                case 2:
                    std::snprintf(buffer, capacity, "Sequencer/Playing");
                    break;
                case 3:
                    std::snprintf(buffer, capacity, "Sequencer/RecordArm");
                    break;
                default:
                    std::snprintf(buffer, capacity, "Sequencer/Playhead");
                    break;
            }
            break;
        case Axis::SceneBlend:
            std::snprintf(buffer, capacity, "Global/SceneBlend");
            break;
        case Axis::GestureWeight:
            std::snprintf(buffer, capacity, "Global/Gesture%u", static_cast<unsigned>(index + 1));
            break;
    }
}

inline void buildStableId(char* buffer, size_t capacity, Axis axis, uint8_t page, uint8_t row, uint8_t index)
{
    if (capacity == 0)
    {
        return;
    }
    buffer[0] = '\0';

    switch (axis)
    {
        case Axis::PageKnob:
            std::snprintf(buffer, capacity, "page%u_row%u_knob", static_cast<unsigned>(page), static_cast<unsigned>(row));
            break;
        case Axis::PageModDepth:
            std::snprintf(buffer, capacity, "page%u_row%u_depth", static_cast<unsigned>(page), static_cast<unsigned>(row));
            break;
        case Axis::GlobalCrunchy:
            std::snprintf(buffer, capacity, "global_crunchy");
            break;
        case Axis::VcoMorph:
            std::snprintf(buffer, capacity, "vco_morph%u", static_cast<unsigned>(index));
            break;
        case Axis::Sequencer:
            switch (index)
            {
                case 0:
                    std::snprintf(buffer, capacity, "sequencer_bpm");
                    break;
                case 1:
                    std::snprintf(buffer, capacity, "sequencer_pattern_length");
                    break;
                case 2:
                    std::snprintf(buffer, capacity, "sequencer_playing");
                    break;
                case 3:
                    std::snprintf(buffer, capacity, "sequencer_record_arm");
                    break;
                default:
                    std::snprintf(buffer, capacity, "sequencer_playhead");
                    break;
            }
            break;
        case Axis::SceneBlend:
            std::snprintf(buffer, capacity, "global_scene_blend");
            break;
        case Axis::GestureWeight:
            std::snprintf(buffer, capacity, "global_gesture_weight_%u", static_cast<unsigned>(index));
            break;
    }
}

struct RuntimeDescriptor : Descriptor
{
    char stableIdStorage[48];
    char displayNameStorage[96];
};

inline RuntimeDescriptor buildDescriptorAt(size_t index)
{
    RuntimeDescriptor entry{};
    entry.id = static_cast<Id>(index);
    entry.minNorm = 0.0f;
    entry.maxNorm = 1.0f;

    if (index < kPageKnobCount)
    {
        uint8_t page = 0;
        uint8_t row = 0;
        decodePageRowIndex(index, page, row);
        entry.axis = Axis::PageKnob;
        entry.page = page;
        entry.row = row;
        entry.defaultNorm = pageKnobDefault(page, row);
    }
    else if (index < kPageKnobCount + kPageModDepthCount)
    {
        const size_t local = index - kPageKnobCount;
        uint8_t page = 0;
        uint8_t row = 0;
        decodePageRowIndex(local, page, row);
        entry.axis = Axis::PageModDepth;
        entry.page = page;
        entry.row = row;
        entry.defaultNorm = modDepthDefault();
    }
    else if (index < kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount)
    {
        entry.axis = Axis::GlobalCrunchy;
        entry.defaultNorm = 0.0f;
    }
    else if (index < kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount)
    {
        entry.axis = Axis::VcoMorph;
        entry.index = static_cast<uint8_t>(index - (kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount));
        entry.defaultNorm = vcoMorphDefault(entry.index);
    }
    else if (index < kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount + kSequencerCount)
    {
        entry.axis = Axis::Sequencer;
        entry.index = static_cast<uint8_t>(
            index - (kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount));
        switch (entry.index)
        {
            case 0:
                entry.defaultNorm = (120.0f - 20.0f) / (300.0f - 20.0f);
                break;
            case 1:
                entry.defaultNorm = (16.0f - 4.0f) / (64.0f - 4.0f);
                break;
            case 2:
            case 3:
                entry.defaultNorm = 0.0f;
                break;
            default:
                entry.defaultNorm = 0.0f;
                break;
        }
    }
    else if (index < kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount + kSequencerCount
                        + kSceneBlendCount)
    {
        entry.axis = Axis::SceneBlend;
        entry.defaultNorm = 0.5f;
    }
    else
    {
        entry.axis = Axis::GestureWeight;
        entry.index = static_cast<uint8_t>(
            index - (kPageKnobCount + kPageModDepthCount + kGlobalCrunchyCount + kMorphKnobCount + kSequencerCount
                     + kSceneBlendCount));
        entry.defaultNorm = 0.0f;
    }

    buildStableId(entry.stableIdStorage, sizeof(entry.stableIdStorage), entry.axis, entry.page, entry.row, entry.index);
    buildDisplayName(
        entry.displayNameStorage, sizeof(entry.displayNameStorage), entry.axis, entry.page, entry.row, entry.index);
    entry.stableId = entry.stableIdStorage;
    entry.displayName = entry.displayNameStorage;
    return entry;
}

inline bool validateInventory()
{
    for (size_t i = 0; i < kCount; ++i)
    {
        const RuntimeDescriptor entry = buildDescriptorAt(i);
        if (static_cast<size_t>(entry.id) != i)
        {
            return false;
        }
        if (entry.stableId == nullptr || entry.stableId[0] == '\0')
        {
            return false;
        }
        if (entry.displayName == nullptr || entry.displayName[0] == '\0')
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
} // namespace HostParameterInventoryV2
