#pragma once

#include "HostParameterInventoryV2.hpp"
#include "SequencerState.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace froggers_v2::manifest
{
constexpr uint32_t kSchemaVersion = 1;
constexpr const char* kAuthority = "cpp";
constexpr uint8_t kSequencerSlotCount = 16;
constexpr uint8_t kDirectionChoiceCount = 3;
constexpr uint8_t kSpeedChoiceCount = 5;
inline constexpr const char* kSequencerWrittenState = "written-unwritten";
inline constexpr std::array<const char*, 2> kRandomizationScopes{{"scene", "step"}};

// Page/row display name data + logic is the single authority in
// sim/V2ParamDisplayNames.hpp (needed below sim/, e.g. DelayState, wasm
// bindings; transitively included here via HostParameterInventoryV2.hpp).
// These wrappers exist only because callers in this manifest namespace
// already spell it as froggers_v2::manifest::product*DisplayName.
inline const char* productPageDisplayName(uint8_t hostPage)
{
    return V2ParamDisplayNames::forHostPage(hostPage);
}

inline const char* productRowDisplayName(uint8_t hostPage, uint8_t row)
{
    return V2ParamDisplayNames::forHostPageRow(hostPage, row);
}

inline void formatInventoryStableId(char* buffer,
                                    size_t capacity,
                                    HostParameterInventoryV2::Axis axis,
                                    uint8_t page,
                                    uint8_t row,
                                    uint8_t index)
{
    if (capacity == 0)
    {
        return;
    }
    buffer[0] = '\0';

    switch (axis)
    {
        case HostParameterInventoryV2::Axis::PageKnob:
            std::snprintf(buffer, capacity, "page%u_row%u_knob", static_cast<unsigned>(page), static_cast<unsigned>(row));
            break;
        case HostParameterInventoryV2::Axis::PageModDepth:
            std::snprintf(buffer, capacity, "page%u_row%u_depth", static_cast<unsigned>(page), static_cast<unsigned>(row));
            break;
        case HostParameterInventoryV2::Axis::GlobalCrunchy:
            std::snprintf(buffer, capacity, "global_crunchy");
            break;
        case HostParameterInventoryV2::Axis::VcoMorph:
            std::snprintf(buffer, capacity, "vco_morph%u", static_cast<unsigned>(index));
            break;
        case HostParameterInventoryV2::Axis::Sequencer:
            switch (index)
            {
                case 0:
                    std::snprintf(buffer, capacity, "sequencer_bpm");
                    break;
                case 1:
                    std::snprintf(buffer, capacity, "sequencer_direction");
                    break;
                case 2:
                    std::snprintf(buffer, capacity, "sequencer_playing");
                    break;
                case 3:
                    std::snprintf(buffer, capacity, "sequencer_record_arm");
                    break;
                default:
                    std::snprintf(buffer, capacity, "sequencer_speed");
                    break;
            }
            break;
        case HostParameterInventoryV2::Axis::SceneBlend:
            std::snprintf(buffer, capacity, "global_scene_blend");
            break;
        case HostParameterInventoryV2::Axis::GestureWeight:
            std::snprintf(buffer, capacity, "global_gesture_weight_%u", static_cast<unsigned>(index));
            break;
    }
}

inline void formatInventoryDisplayName(char* buffer,
                                       size_t capacity,
                                       HostParameterInventoryV2::Axis axis,
                                       uint8_t page,
                                       uint8_t row,
                                       uint8_t index)
{
    if (capacity == 0)
    {
        return;
    }
    buffer[0] = '\0';

    switch (axis)
    {
        case HostParameterInventoryV2::Axis::PageKnob:
        case HostParameterInventoryV2::Axis::PageModDepth:
        {
            const char* pageName = productPageDisplayName(page);
            const char* rowName = productRowDisplayName(page, row);
            if (page == HostParameterInventoryV2::kAdsrUiPage)
            {
                std::snprintf(buffer,
                              capacity,
                              "Pair-AR/%s%s",
                              rowName,
                              axis == HostParameterInventoryV2::Axis::PageModDepth ? " depth" : "");
            }
            else
            {
                std::snprintf(buffer,
                              capacity,
                              "Module/%s/%s%s",
                              pageName,
                              rowName,
                              axis == HostParameterInventoryV2::Axis::PageModDepth ? " depth" : "");
            }
            break;
        }
        case HostParameterInventoryV2::Axis::GlobalCrunchy:
            std::snprintf(buffer, capacity, "Global/Crunchy");
            break;
        case HostParameterInventoryV2::Axis::VcoMorph:
            std::snprintf(buffer, capacity, "Global/VCO%u morph", static_cast<unsigned>(index + 1));
            break;
        case HostParameterInventoryV2::Axis::Sequencer:
            switch (index)
            {
                case 0:
                    std::snprintf(buffer, capacity, "Sequencer/BPM");
                    break;
                case 1:
                    std::snprintf(buffer, capacity, "Sequencer/Direction");
                    break;
                case 2:
                    std::snprintf(buffer, capacity, "Sequencer/Playing");
                    break;
                case 3:
                    std::snprintf(buffer, capacity, "Sequencer/Write Seq.");
                    break;
                default:
                    std::snprintf(buffer, capacity, "Sequencer/Speed");
                    break;
            }
            break;
        case HostParameterInventoryV2::Axis::SceneBlend:
            std::snprintf(buffer, capacity, "Global/SceneBlend");
            break;
        case HostParameterInventoryV2::Axis::GestureWeight:
            std::snprintf(buffer, capacity, "Global/Gesture%u", static_cast<unsigned>(index + 1));
            break;
    }
}

struct Projection
{
    const char* id;
    const char* displayName;
    bool hidden;
    bool readOnly;
    bool interactive;
    bool reserved;
};

struct ProductRow
{
    uint8_t page;
    uint8_t row;
    const char* pageName;
    const char* displayName;
    char stableIdStorage[32];
};

struct HostedParameterEntry
{
    size_t index;
    std::string stableId;
    std::string displayName;
    uint8_t page;
    uint8_t row;
    HostParameterInventoryV2::Axis axis;
    float minNorm;
    float maxNorm;
    float defaultNorm;
};

struct ControllerTargetDeclaration
{
    const char* stableId;
    const char* displayName;
    const char* surface;
    const char* bindingRole;
};

struct ModulationSource
{
    const char* stableId;
    const char* displayName;
    const char* group;
    const char* rateClass;
    const char* availabilityRule;
    uint32_t colorRgb;
    bool randomizable;
};

struct OscilloscopeTap
{
    const char* stableId;
    const char* displayName;
    uint8_t channel;
    bool active;
};

struct RandomizationScopeChoice
{
    const char* displayName;
    bool selectedByDefault;
};

struct RandomizationScopeControl
{
    const char* stableId;
    const char* displayName;
    std::array<RandomizationScopeChoice, 2> choices;
    bool consumedByRandomizeAll;
    bool consumedByRandomizeMod;
};

struct SequencerSlot
{
    uint8_t index;
    const char* writtenStateField;
    bool sequencerPersistent;
    bool sequencerLockable;
};

struct ValidationSummary
{
    bool duplicateStableIds;
    bool invalidPageRowRefs;
    bool missingDisplayNames;
    bool invalidRanges;
    bool invalidDefaults;
    bool invalidProjectionOverlays;
    bool missingOscilloscopeTaps;
    bool invalidRandomizationScopes;
    bool invalidSequencerStepCounts;
    bool missingWrittenUnwrittenState;
    bool invalidSequencerDirectionSpeed;
    bool invalidSourceLaneCounts;
    bool duplicateSourceIds;
    bool missingSourceColorsGroups;
    bool invalidExternalAudioAvailability;
};

inline constexpr std::array<Projection, 4> kProjections{{
    {"product", "Product", false, false, true, false},
    {"desktop", "Desktop Standalone", false, false, true, false},
    {"plugin", "VST/AU", false, false, true, false},
    {"vcv", "VCV Reserved", true, true, false, true},
}};

inline constexpr std::array<const char*, kDirectionChoiceCount> kSequencerDirectionChoices{{"<", ">", "RND"}};
inline constexpr std::array<const char*, kSpeedChoiceCount> kSequencerSpeedChoices{{"/2", "/1.5", "1", "x1.5", "x2"}};
inline constexpr std::array<RandomizationScopeControl, 2> kRandomizationScopeControls{{
    {"scene",
     "Scene",
     {{{"All Scenes", true}, {"Current Scene", false}}},
     true,
     true},
    {"step",
     "Step",
     {{{"All Steps", true}, {"Current Step", false}}},
     true,
     true},
}};

inline constexpr std::array<SequencerSlot, kSequencerSlotCount> kSequencerSlots{{
    {0, "slot_0_written_unwritten_state", true, true},
    {1, "slot_1_written_unwritten_state", true, true},
    {2, "slot_2_written_unwritten_state", true, true},
    {3, "slot_3_written_unwritten_state", true, true},
    {4, "slot_4_written_unwritten_state", true, true},
    {5, "slot_5_written_unwritten_state", true, true},
    {6, "slot_6_written_unwritten_state", true, true},
    {7, "slot_7_written_unwritten_state", true, true},
    {8, "slot_8_written_unwritten_state", true, true},
    {9, "slot_9_written_unwritten_state", true, true},
    {10, "slot_10_written_unwritten_state", true, true},
    {11, "slot_11_written_unwritten_state", true, true},
    {12, "slot_12_written_unwritten_state", true, true},
    {13, "slot_13_written_unwritten_state", true, true},
    {14, "slot_14_written_unwritten_state", true, true},
    {15, "slot_15_written_unwritten_state", true, true},
}};

inline constexpr std::array<ControllerTargetDeclaration, 10> kControllerTargetDeclarations{{
    {"midi_pitch", "Pitch", "MIDI In", "pitch target"},
    {"midi_gate", "Gate", "MIDI In", "gate target"},
    {"midi_external_mod_a", "MIDI CC A", "MIDI In", "external mod A"},
    {"midi_external_mod_b", "MIDI CC B", "MIDI In", "external mod B"},
    {"midi_shift_button", "Shift button", "MIDI In", "held shift toggle"},
    {"midi_scene_1", "Scene S1", "MIDI In", "scene select"},
    {"midi_scene_2", "Scene S2", "MIDI In", "scene select"},
    {"midi_scene_3", "Scene S3", "MIDI In", "scene select"},
    {"midi_qwerty_virtual", "QWERTY virtual MIDI", "MIDI In", "virtual keyboard"},
    {"midi_external_clock", "External MIDI clock", "MIDI In", "sequencer sync"},
}};

inline constexpr std::array<ModulationSource, 15> kPermanentModulationSources{{
    {"vco_pair_12", "VCO 1+2", "vco-pair-bus", "audio", "always", 0xff5f57u, true},
    {"vco_pair_23", "VCO 2+3", "vco-pair-bus", "audio", "always", 0xffbd2eu, true},
    {"vco_pair_13", "VCO 1+3", "vco-pair-bus", "audio", "always", 0x28c840u, true},
    {"vco_1_ef", "VCO 1 EF", "vco-envelope-follower", "envelope", "always", 0xff7b72u, true},
    {"vco_2_ef", "VCO 2 EF", "vco-envelope-follower", "envelope", "always", 0xd29922u, true},
    {"vco_3_ef", "VCO 3 EF", "vco-envelope-follower", "envelope", "always", 0x56d364u, true},
    {"vco_12_ef", "VCO 1+2 EF", "vco-envelope-follower", "envelope", "always", 0xf0883eu, true},
    {"vco_23_ef", "VCO 2+3 EF", "vco-envelope-follower", "envelope", "always", 0xa371f7u, true},
    {"lfo_1", "LFO 1", "lfo", "control", "always", 0x79c0ffu, true},
    {"lfo_2", "LFO 2", "lfo", "control", "always", 0x88d1f1u, true},
    {"lfo_3", "LFO 3", "lfo", "control", "always", 0xb3f0ffu, true},
    {"random_marbles_1", "Random/Marbles 1", "random", "random", "always", 0xf778bau, true},
    {"random_marbles_2", "Random/Marbles 2", "random", "random", "always", 0xdb61a2u, true},
    {"external_audio_rate", "External Audio (audio rate)", "external-audio", "audio", "external-audio-input", 0x39d0d8u, false},
    {"external_audio_ef", "External Audio (envelope follower)", "external-audio", "envelope", "external-audio-input", 0x33b1ffu, false},
}};

inline constexpr uint8_t kExternalAudioRateLane = 13;
inline constexpr uint8_t kExternalAudioEfLane = 14;
inline constexpr uint8_t kModDetailCellCount = 16;

inline bool isExternalAudioModLane(uint8_t lane)
{
    return lane == kExternalAudioRateLane || lane == kExternalAudioEfLane;
}

inline bool isVcoPairBusModLane(uint8_t lane)
{
    return lane <= 2;
}

inline bool isModSourceEligibleForRow(uint8_t page, uint8_t row, uint8_t lane, bool externalAudioAvailable)
{
    if (lane >= kPermanentModulationSources.size())
    {
        return false;
    }
    const ModulationSource& source = kPermanentModulationSources[lane];
    if (!source.randomizable)
    {
        return false;
    }
    if (isExternalAudioModLane(lane) && !externalAudioAvailable)
    {
        return false;
    }
    if (page == 0 && isVcoPairBusModLane(lane))
    {
        if (row == 0)
        {
            return lane == 1;
        }
        if (row == 1)
        {
            return lane == 2;
        }
        if (row == 2)
        {
            return lane == 0;
        }
    }
    return true;
}

inline std::array<HostedParameterEntry, HostParameterInventoryV2::kCount> buildHostedParameterEntries()
{
    std::array<HostedParameterEntry, HostParameterInventoryV2::kCount> entries{};
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor descriptor = HostParameterInventoryV2::buildDescriptorAt(i);
        entries[i] = HostedParameterEntry{
            i,
            descriptor.stableId != nullptr ? descriptor.stableId : "",
            descriptor.displayName != nullptr ? descriptor.displayName : "",
            descriptor.page,
            descriptor.row,
            descriptor.axis,
            descriptor.minNorm,
            descriptor.maxNorm,
            descriptor.defaultNorm,
        };
    }
    return entries;
}

inline constexpr std::array<OscilloscopeTap, 3> kOscilloscopeTaps{{
    {"oscilloscope_vco_1_ef", "VCO 1 EF", 0, true},
    {"oscilloscope_vco_2_ef", "VCO 2 EF", 1, true},
    {"oscilloscope_vco_3_ef", "VCO 3 EF", 2, true},
}};

inline bool isValidNorm(float value)
{
    return value >= 0.0f && value <= 1.0f;
}

template <size_t N>
inline bool hasDuplicateStableIds(const std::array<HostParameterInventoryV2::RuntimeDescriptor, N>& entries)
{
    for (size_t i = 0; i < N; ++i)
    {
        for (size_t j = i + 1; j < N; ++j)
        {
            if (std::string(entries[i].stableId) == std::string(entries[j].stableId))
            {
                return true;
            }
        }
    }
    return false;
}

inline bool hasDuplicateStableIds(const std::array<ModulationSource, 15>& entries)
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        for (size_t j = i + 1; j < entries.size(); ++j)
        {
            if (std::string(entries[i].stableId) == std::string(entries[j].stableId))
            {
                return true;
            }
        }
    }
    return false;
}

template <typename EntryT, size_t N, typename StableIdGetter>
inline bool hasDuplicateStableIds(const std::array<EntryT, N>& entries, StableIdGetter stableIdOf)
{
    for (size_t i = 0; i < N; ++i)
    {
        const char* lhs = stableIdOf(entries[i]);
        for (size_t j = i + 1; j < N; ++j)
        {
            const char* rhs = stableIdOf(entries[j]);
            if (std::string(lhs != nullptr ? lhs : "") == std::string(rhs != nullptr ? rhs : ""))
            {
                return true;
            }
        }
    }
    return false;
}

inline ValidationSummary buildValidationSummary();

inline std::string jsonEscape(const char* text)
{
    std::string out;
    if (text == nullptr)
    {
        return out;
    }
    for (const char c : std::string(text))
    {
        switch (c)
        {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

template <typename EntryT, size_t N, typename StableIdGetter>
inline std::array<EntryT, N> sortedByStableId(const std::array<EntryT, N>& entries, StableIdGetter stableIdOf)
{
    std::array<EntryT, N> sorted = entries;
    std::sort(sorted.begin(), sorted.end(), [&](const EntryT& lhs, const EntryT& rhs) {
        const char* lhsStableId = stableIdOf(lhs);
        const char* rhsStableId = stableIdOf(rhs);
        const std::string lhsId = lhsStableId != nullptr ? lhsStableId : "";
        const std::string rhsId = rhsStableId != nullptr ? rhsStableId : "";
        return lhsId < rhsId;
    });
    return sorted;
}

template <typename EntryT, size_t N, typename GroupGetter, typename StableIdGetter>
inline std::array<EntryT, N> sortedByGroupThenStableId(const std::array<EntryT, N>& entries,
                                                      GroupGetter groupOf,
                                                      StableIdGetter stableIdOf)
{
    std::array<EntryT, N> sorted = entries;
    std::sort(sorted.begin(), sorted.end(), [&](const EntryT& lhs, const EntryT& rhs) {
        const char* lhsGroup = groupOf(lhs);
        const char* rhsGroup = groupOf(rhs);
        const std::string lhsGroupId = lhsGroup != nullptr ? lhsGroup : "";
        const std::string rhsGroupId = rhsGroup != nullptr ? rhsGroup : "";
        if (lhsGroupId != rhsGroupId)
        {
            return lhsGroupId < rhsGroupId;
        }

        const char* lhsStableId = stableIdOf(lhs);
        const char* rhsStableId = stableIdOf(rhs);
        const std::string lhsId = lhsStableId != nullptr ? lhsStableId : "";
        const std::string rhsId = rhsStableId != nullptr ? rhsStableId : "";
        return lhsId < rhsId;
    });
    return sorted;
}

inline ProductRow buildProductRow(uint8_t page, uint8_t row)
{
    ProductRow entry{};
    entry.page = page;
    entry.row = row;
    entry.pageName = productPageDisplayName(page);
    entry.displayName = productRowDisplayName(page, row);
    std::snprintf(entry.stableIdStorage,
                  sizeof(entry.stableIdStorage),
                  "page%u_row%u",
                  static_cast<unsigned>(page),
                  static_cast<unsigned>(row));
    return entry;
}

inline size_t productRowCount()
{
    return HostParameterInventoryV2::kPageRowCount;
}

inline bool validateFoundation()
{
    return HostParameterInventoryV2::validateInventory();
}

inline ValidationSummary buildValidationSummary()
{
    ValidationSummary summary{};
    std::array<std::string, HostParameterInventoryV2::kCount> stableIds{};
    std::array<std::string, HostParameterInventoryV2::kCount> displayNames{};
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        stableIds[i] = entry.stableId != nullptr ? entry.stableId : "";
        displayNames[i] = entry.displayName != nullptr ? entry.displayName : "";
    }

    std::array<ProductRow, HostParameterInventoryV2::kPageRowCount> productRows{};
    size_t productRowIndex = 0;
    for (uint8_t page = 0; page < HostParameterInventoryV2::kNumUiPages; ++page)
    {
        for (uint8_t row = 0; row < HostParameterInventoryV2::rowsForUiPage(page); ++row)
        {
            productRows[productRowIndex++] = buildProductRow(page, row);
        }
    }

    constexpr size_t kGlobalStableIdCount = HostParameterInventoryV2::kPageRowCount + HostParameterInventoryV2::kCount
                                          + kPermanentModulationSources.size() + kOscilloscopeTaps.size()
                                          + kRandomizationScopeControls.size();
    std::array<std::string, kGlobalStableIdCount> globalStableIds{};
    size_t globalStableIdIndex = 0;
    for (const ProductRow& row : productRows)
    {
        globalStableIds[globalStableIdIndex++] = row.stableIdStorage;
    }
    for (const std::string& stableId : stableIds)
    {
        globalStableIds[globalStableIdIndex++] = stableId;
    }
    for (const ModulationSource& source : kPermanentModulationSources)
    {
        globalStableIds[globalStableIdIndex++] = source.stableId != nullptr ? source.stableId : "";
    }
    for (const OscilloscopeTap& tap : kOscilloscopeTaps)
    {
        globalStableIds[globalStableIdIndex++] = tap.stableId != nullptr ? tap.stableId : "";
    }
    for (const RandomizationScopeControl& control : kRandomizationScopeControls)
    {
        globalStableIds[globalStableIdIndex++] = control.stableId != nullptr ? control.stableId : "";
    }
    summary.duplicateStableIds = hasDuplicateStableIds(globalStableIds, [](const std::string& stableId) {
        return stableId.c_str();
    });
    summary.invalidPageRowRefs = false;
    summary.missingDisplayNames = false;
    summary.invalidRanges = false;
    summary.invalidDefaults = false;
    summary.invalidProjectionOverlays = false;
    summary.missingOscilloscopeTaps = kOscilloscopeTaps.size() != 3;
    summary.invalidRandomizationScopes = false;
    summary.invalidSequencerStepCounts = kSequencerSlots.size() != kSequencerSlotCount;
    summary.missingWrittenUnwrittenState = false;
    summary.invalidSequencerDirectionSpeed = (kDirectionChoiceCount != 3 || kSpeedChoiceCount != 5);
    summary.invalidSourceLaneCounts = kPermanentModulationSources.size() != 15;
    summary.duplicateSourceIds = hasDuplicateStableIds(kPermanentModulationSources,
                                                        [](const ModulationSource& source) { return source.stableId; });
    summary.missingSourceColorsGroups = false;
    summary.invalidExternalAudioAvailability = false;

    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        if (entry.page >= HostParameterInventoryV2::kNumUiPages || entry.row >= HostParameterInventoryV2::rowsForUiPage(entry.page))
        {
            summary.invalidPageRowRefs = true;
        }
        if (displayNames[i].empty() || stableIds[i].empty())
        {
            summary.missingDisplayNames = true;
        }
        if (!isValidNorm(entry.minNorm) || !isValidNorm(entry.maxNorm) || entry.minNorm >= entry.maxNorm)
        {
            summary.invalidRanges = true;
        }
        if (!isValidNorm(entry.defaultNorm) || entry.defaultNorm < entry.minNorm || entry.defaultNorm > entry.maxNorm)
        {
            summary.invalidDefaults = true;
        }
    }

    if (kProjections.size() != 4)
    {
        summary.invalidProjectionOverlays = true;
    }
    for (size_t i = 0; i < kProjections.size(); ++i)
    {
        const Projection& projection = kProjections[i];
        const bool isReserved = i == 3;
        const bool isVisible = !isReserved;
        const char* expectedId = isReserved ? "vcv" : (i == 0 ? "product" : (i == 1 ? "desktop" : "plugin"));
        if (projection.id == nullptr || projection.id[0] == '\0' || projection.displayName == nullptr
            || projection.displayName[0] == '\0' || std::string(projection.id) != expectedId
            || projection.hidden != isReserved || projection.readOnly != isReserved
            || projection.interactive != isVisible || projection.reserved != isReserved)
        {
            summary.invalidProjectionOverlays = true;
        }
    }
    if (kOscilloscopeTaps.size() != 3)
    {
        summary.missingOscilloscopeTaps = true;
    }
    else
    {
        for (const OscilloscopeTap& tap : kOscilloscopeTaps)
        {
            if (tap.stableId == nullptr || tap.stableId[0] == '\0' || tap.displayName == nullptr
                || tap.displayName[0] == '\0' || tap.channel >= kOscilloscopeTaps.size() || !tap.active)
            {
                summary.missingOscilloscopeTaps = true;
            }
        }
    }
    if (kRandomizationScopeControls.size() != 2)
    {
        summary.invalidRandomizationScopes = true;
    }
    else
    {
        for (size_t i = 0; i < kRandomizationScopeControls.size(); ++i)
        {
            const RandomizationScopeControl& control = kRandomizationScopeControls[i];
            const bool isScene = i == 0;
            const char* expectedStableId = isScene ? "scene" : "step";
            const char* expectedDisplayName = isScene ? "Scene" : "Step";
            const char* expectedFirstChoice = isScene ? "All Scenes" : "All Steps";
            const char* expectedSecondChoice = isScene ? "Current Scene" : "Current Step";
            if (control.stableId == nullptr || std::string(control.stableId) != expectedStableId
                || control.displayName == nullptr || std::string(control.displayName) != expectedDisplayName
                || control.choices.size() != 2 || control.choices[0].displayName == nullptr
                || std::string(control.choices[0].displayName) != expectedFirstChoice
                || control.choices[1].displayName == nullptr
                || std::string(control.choices[1].displayName) != expectedSecondChoice
                || !control.choices[0].selectedByDefault || control.choices[1].selectedByDefault
                || !control.consumedByRandomizeAll || !control.consumedByRandomizeMod)
            {
                summary.invalidRandomizationScopes = true;
            }
        }
    }
    if (kSequencerSlots.size() != kSequencerSlotCount)
    {
        summary.invalidSequencerStepCounts = true;
        summary.missingWrittenUnwrittenState = true;
    }
    else
    {
        for (size_t i = 0; i < kSequencerSlots.size(); ++i)
        {
            const SequencerSlot& slot = kSequencerSlots[i];
            if (slot.index != i)
            {
                summary.invalidSequencerStepCounts = true;
            }
            if (slot.writtenStateField == nullptr || slot.writtenStateField[0] == '\0' || !slot.sequencerPersistent
                || !slot.sequencerLockable)
            {
                summary.missingWrittenUnwrittenState = true;
            }
        }
    }
    if (kDirectionChoiceCount != 3 || kSpeedChoiceCount != 5 || std::string(kSequencerDirectionChoices[2]) != "RND"
        || std::string(kSequencerSpeedChoices[2]) != "1" || std::string(kSequencerWrittenState) != "written-unwritten"
        || kRandomizationScopes.size() != 2 || std::string(kRandomizationScopes[0]) != "scene"
        || std::string(kRandomizationScopes[1]) != "step")
    {
        summary.invalidSequencerDirectionSpeed = true;
    }
    if (kPermanentModulationSources.size() != 15)
    {
        summary.invalidSourceLaneCounts = true;
    }
    for (const ModulationSource& source : kPermanentModulationSources)
    {
        if (source.stableId == nullptr || source.stableId[0] == '\0' || source.displayName == nullptr
            || source.displayName[0] == '\0' || source.group == nullptr || source.group[0] == '\0'
            || source.rateClass == nullptr || source.rateClass[0] == '\0' || source.availabilityRule == nullptr
            || source.availabilityRule[0] == '\0' || source.colorRgb == 0)
        {
            summary.missingSourceColorsGroups = true;
        }
        if ((std::string(source.stableId).rfind("external_audio_", 0) == 0)
            && std::string(source.availabilityRule) != "external-audio-input")
        {
            summary.invalidExternalAudioAvailability = true;
        }
    }
    for (const ModulationSource& source : kPermanentModulationSources)
    {
        if (!source.randomizable && std::string(source.group) != "external-audio")
        {
            summary.invalidRandomizationScopes = true;
        }
        if (source.randomizable && std::string(source.availabilityRule) == "external-audio-input")
        {
            summary.invalidRandomizationScopes = true;
        }
        if (std::string(source.group) == "external-audio" && std::string(source.availabilityRule) != "external-audio-input")
        {
            summary.invalidExternalAudioAvailability = true;
        }
    }
    return summary;
}

inline std::string buildSnapshotJson()
{
    std::ostringstream out;
    out << "{\n";
    out << "  \"schemaVersion\": " << kSchemaVersion << ",\n";
    out << "  \"authority\": \"" << kAuthority << "\",\n";
    out << "  \"schema\": {\n";
    out << "    \"productControls\": {\n";
    out << "      \"visibility\": {\"hidden\": false, \"readOnly\": false, \"interactive\": true},\n";
    out << "      \"rows\": " << productRowCount() << "\n";
    out << "    },\n";
    out << "    \"projectionOverlays\": {\n";
    out << "      \"visibility\": {\"hidden\": " << (kProjections[0].hidden ? "true" : "false")
        << ", \"readOnly\": " << (kProjections[0].readOnly ? "true" : "false")
        << ", \"interactive\": " << (kProjections[0].interactive ? "true" : "false") << "},\n";
    for (size_t i = 0; i < kProjections.size(); ++i)
    {
        const Projection& projection = kProjections[i];
        out << "      \"" << projection.id << "\": {\"hidden\": " << (projection.hidden ? "true" : "false")
            << ", \"readOnly\": " << (projection.readOnly ? "true" : "false") << ", \"interactive\": "
            << (projection.interactive ? "true" : "false") << "}";
        out << (i + 1 == kProjections.size() ? "\n" : ",\n");
    }
    out << "    },\n";
    out << "    \"runtimeControls\": {\n";
    out << "      \"sequencer\": {\"slots\": " << static_cast<unsigned>(kSequencerSlotCount)
        << ", \"defaultDirection\": \">\", \"defaultSpeed\": \"1\"}\n";
    out << "    },\n";
    out << "    \"randomizationScopeControls\": [\n";
    for (size_t i = 0; i < kRandomizationScopeControls.size(); ++i)
    {
        const RandomizationScopeControl& control = kRandomizationScopeControls[i];
        out << "      {\"stableId\": \"" << jsonEscape(control.stableId) << "\", \"displayName\": \""
            << jsonEscape(control.displayName) << "\", \"choices\": [";
        for (size_t j = 0; j < control.choices.size(); ++j)
        {
            const RandomizationScopeChoice& choice = control.choices[j];
            if (j != 0)
            {
                out << ", ";
            }
            out << "{\"displayName\": \"" << jsonEscape(choice.displayName)
                << "\", \"selectedByDefault\": " << (choice.selectedByDefault ? "true" : "false") << "}";
        }
        out << "], \"consumedByRandomizeAll\": "
            << (control.consumedByRandomizeAll ? "true" : "false") << ", \"consumedByRandomizeMod\": "
            << (control.consumedByRandomizeMod ? "true" : "false") << "}";
        out << (i + 1 == kRandomizationScopeControls.size() ? "\n" : ",\n");
    }
    out << "    ],\n";
    out << "    \"controllerTargets\": [\n";
    for (size_t i = 0; i < kControllerTargetDeclarations.size(); ++i)
    {
        const ControllerTargetDeclaration& target = kControllerTargetDeclarations[i];
        out << "      {\"stableId\": \"" << jsonEscape(target.stableId) << "\", \"displayName\": \""
            << jsonEscape(target.displayName) << "\", \"surface\": \"" << jsonEscape(target.surface)
            << "\", \"bindingRole\": \"" << jsonEscape(target.bindingRole) << "\"}";
        out << (i + 1 == kControllerTargetDeclarations.size() ? "\n" : ",\n");
    }
    out << "    ],\n";
    out << "    \"hostParameterMapping\": {\n";
    out << "      \"projection\": \"plugin\",\n";
    out << "      \"count\": " << HostParameterInventoryV2::kCount << "\n";
    out << "    },\n";
    out << "    \"hardwareControls\": {\n";
    out << "      \"permanentModulationSources\": " << kPermanentModulationSources.size() << "\n";
    out << "    },\n";
    out << "    \"oscilloscopeTaps\": " << kOscilloscopeTaps.size() << ",\n";
    out << "    \"productContract\": {\n";
    out << "      \"defaultModulePage\": \"audio_vco\",\n";
    out << "      \"crossCouplers\": [\"vco_12\", \"vco_23\"],\n";
    out << "      \"envelopePage\": {\"vcoAttackReleasePairs\": true},\n";
    out << "      \"waveformMorphControls\": true,\n";
    out << "      \"lfoModule\": {\"firstClass\": true, \"firstClassSourceParticipation\": true},\n";
    out << "      \"permanentModulationRack\": {\"lanes\": " << kPermanentModulationSources.size()
        << ", \"externalAudioLaneAvailable\": true},\n";
    out << "      \"globalRandomizationScopes\": {\"randomizeAll\": [\"" << kRandomizationScopes[0]
        << "\", \"" << kRandomizationScopes[1] << "\"], \"randomizeMod\": [\"" << kRandomizationScopes[0]
        << "\", \"" << kRandomizationScopes[1] << "\"]},\n";
    out << "      \"sequencer\": {\n";
    out << "        \"slots\": " << static_cast<unsigned>(kSequencerSlotCount) << ",\n";
    out << "        \"writtenState\": \"" << kSequencerWrittenState << "\",\n";
    out << "        \"deviceNeutralClearStep\": {\"longPress\": true},\n";
    out << "        \"sequencerLocks\": true,\n";
    out << "        \"directionChoices\": [\"<\", \">\", \"RND\"],\n";
    out << "        \"defaultDirection\": \">\",\n";
    out << "        \"speedChoices\": [\"/2\", \"/1.5\", \"1\", \"x1.5\", \"x2\"],\n";
    out << "        \"defaultSpeed\": \"1\",\n";
    out << "        \"optionalMidiClockSync\": true,\n";
    out << "        \"heldGestures\": false\n";
    out << "      }\n";
    out << "    },\n";
    out << "    \"layoutGroups\": {\n";
    out << "      \"pages\": " << static_cast<unsigned>(HostParameterInventoryV2::kNumUiPages) << "\n";
    out << "    },\n";
    out << "    \"reservedSchemaOnly\": {\n";
    out << "      \"vcv\": {\"hidden\": " << (kProjections[3].hidden ? "true" : "false")
        << ", \"readOnly\": " << (kProjections[3].readOnly ? "true" : "false")
        << ", \"interactive\": " << (kProjections[3].interactive ? "true" : "false") << "}\n";
    out << "    }\n";
    out << "  },\n";
    out << "  \"counts\": {\n";
    out << "    \"productRows\": " << productRowCount() << ",\n";
    out << "    \"hostParameters\": " << HostParameterInventoryV2::kCount << ",\n";
    out << "    \"modulationSources\": " << kPermanentModulationSources.size() << ",\n";
    out << "    \"oscilloscopeTaps\": " << kOscilloscopeTaps.size() << ",\n";
    out << "    \"sequencerSlots\": " << static_cast<unsigned>(kSequencerSlotCount) << "\n";
    out << "  },\n";

    out << "  \"oscilloscopeTaps\": [\n";
    for (size_t i = 0; i < kOscilloscopeTaps.size(); ++i)
    {
        const OscilloscopeTap& tap = kOscilloscopeTaps[i];
        out << "    {\"stableId\": \"" << jsonEscape(tap.stableId) << "\", \"displayName\": \""
            << jsonEscape(tap.displayName) << "\", \"channel\": " << static_cast<unsigned>(tap.channel)
            << ", \"active\": " << (tap.active ? "true" : "false") << "}";
        out << (i + 1 == kOscilloscopeTaps.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    out << "  \"randomizationScopeControls\": [\n";
    for (size_t i = 0; i < kRandomizationScopeControls.size(); ++i)
    {
        const RandomizationScopeControl& control = kRandomizationScopeControls[i];
        out << "    {\"stableId\": \"" << jsonEscape(control.stableId) << "\", \"displayName\": \""
            << jsonEscape(control.displayName) << "\", \"choices\": [";
        for (size_t j = 0; j < control.choices.size(); ++j)
        {
            const RandomizationScopeChoice& choice = control.choices[j];
            if (j != 0)
            {
                out << ", ";
            }
            out << "{\"displayName\": \"" << jsonEscape(choice.displayName)
                << "\", \"selectedByDefault\": " << (choice.selectedByDefault ? "true" : "false") << "}";
        }
        out << "], \"consumedByRandomizeAll\": "
            << (control.consumedByRandomizeAll ? "true" : "false") << ", \"consumedByRandomizeMod\": "
            << (control.consumedByRandomizeMod ? "true" : "false") << "}";
        out << (i + 1 == kRandomizationScopeControls.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    out << "  \"sequencerSlots\": [\n";
    for (size_t i = 0; i < kSequencerSlots.size(); ++i)
    {
        const SequencerSlot& slot = kSequencerSlots[i];
        out << "    {\"index\": " << static_cast<unsigned>(slot.index) << ", \"writtenStateField\": \""
            << jsonEscape(slot.writtenStateField) << "\", \"sequencerPersistent\": "
            << (slot.sequencerPersistent ? "true" : "false") << ", \"sequencerLockable\": "
            << (slot.sequencerLockable ? "true" : "false") << "}";
        out << (i + 1 == kSequencerSlots.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    const ValidationSummary validation = buildValidationSummary();
    out << "  \"validation\": {\n";
    out << "    \"duplicateStableIds\": " << (validation.duplicateStableIds ? "true" : "false") << ",\n";
    out << "    \"invalidPageRowRefs\": " << (validation.invalidPageRowRefs ? "true" : "false") << ",\n";
    out << "    \"missingDisplayNames\": " << (validation.missingDisplayNames ? "true" : "false") << ",\n";
    out << "    \"invalidRanges\": " << (validation.invalidRanges ? "true" : "false") << ",\n";
    out << "    \"invalidDefaults\": " << (validation.invalidDefaults ? "true" : "false") << ",\n";
    out << "    \"invalidProjectionOverlays\": " << (validation.invalidProjectionOverlays ? "true" : "false")
        << ",\n";
    out << "    \"missingOscilloscopeTaps\": " << (validation.missingOscilloscopeTaps ? "true" : "false")
        << ",\n";
    out << "    \"invalidRandomizationScopes\": " << (validation.invalidRandomizationScopes ? "true" : "false")
        << ",\n";
    out << "    \"invalidSequencerStepCounts\": " << (validation.invalidSequencerStepCounts ? "true" : "false")
        << ",\n";
    out << "    \"missingWrittenUnwrittenState\": "
        << (validation.missingWrittenUnwrittenState ? "true" : "false") << ",\n";
    out << "    \"invalidSequencerDirectionSpeed\": "
        << (validation.invalidSequencerDirectionSpeed ? "true" : "false") << ",\n";
    out << "    \"invalidSourceLaneCounts\": " << (validation.invalidSourceLaneCounts ? "true" : "false")
        << ",\n";
    out << "    \"duplicateSourceIds\": " << (validation.duplicateSourceIds ? "true" : "false") << ",\n";
    out << "    \"missingSourceColorsGroups\": " << (validation.missingSourceColorsGroups ? "true" : "false")
        << ",\n";
    out << "    \"invalidExternalAudioAvailability\": "
        << (validation.invalidExternalAudioAvailability ? "true" : "false") << "\n";
    out << "  },\n";

    out << "  \"projections\": [\n";
    for (size_t i = 0; i < kProjections.size(); ++i)
    {
        const Projection& projection = kProjections[i];
        out << "    {\"id\": \"" << projection.id << "\", \"displayName\": \""
            << jsonEscape(projection.displayName) << "\", \"hidden\": "
            << (projection.hidden ? "true" : "false") << ", \"readOnly\": "
            << (projection.readOnly ? "true" : "false") << ", \"interactive\": "
            << (projection.interactive ? "true" : "false") << ", \"reserved\": "
            << (projection.reserved ? "true" : "false") << "}";
        out << (i + 1 == kProjections.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    out << "  \"productRows\": [\n";
    bool firstRow = true;
    for (uint8_t page = 0; page < HostParameterInventoryV2::kNumUiPages; ++page)
    {
        for (uint8_t row = 0; row < HostParameterInventoryV2::rowsForUiPage(page); ++row)
        {
            const ProductRow productRow = buildProductRow(page, row);
            if (!firstRow)
            {
                out << ",\n";
            }
            firstRow = false;
            out << "    {\"projection\": \"product\", \"page\": " << static_cast<unsigned>(page)
                << ", \"row\": " << static_cast<unsigned>(row) << ", \"stableId\": \""
                << productRow.stableIdStorage << "\", \"pageName\": \"" << jsonEscape(productRow.pageName)
                << "\", \"displayName\": \"" << jsonEscape(productRow.displayName) << "\"}";
        }
    }
    out << "\n  ],\n";

    const auto sortedModulationSources = sortedByGroupThenStableId(
        kPermanentModulationSources,
        [](const ModulationSource& source) { return source.group; },
        [](const ModulationSource& source) { return source.stableId; });
    out << "  \"modulationSources\": [\n";
    for (size_t i = 0; i < sortedModulationSources.size(); ++i)
    {
        const ModulationSource& source = sortedModulationSources[i];
        out << "    {\"stableId\": \"" << source.stableId << "\", \"displayName\": \""
            << jsonEscape(source.displayName) << "\", \"group\": \"" << source.group
            << "\", \"rateClass\": \"" << source.rateClass << "\", \"availabilityRule\": \""
            << source.availabilityRule << "\", \"colorRgb\": \"#";
        out << std::hex << std::setw(6) << std::setfill('0') << source.colorRgb << std::dec
            << "\", \"randomizable\": " << (source.randomizable ? "true" : "false") << "}";
        out << (i + 1 == sortedModulationSources.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    std::array<size_t, HostParameterInventoryV2::kCount> hostParameterIndices{};
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        hostParameterIndices[i] = i;
    }
    std::sort(hostParameterIndices.begin(), hostParameterIndices.end(), [](size_t lhs, size_t rhs) {
        const HostParameterInventoryV2::RuntimeDescriptor lhsEntry = HostParameterInventoryV2::buildDescriptorAt(lhs);
        const HostParameterInventoryV2::RuntimeDescriptor rhsEntry = HostParameterInventoryV2::buildDescriptorAt(rhs);
        const std::string lhsId = lhsEntry.stableId != nullptr ? lhsEntry.stableId : "";
        const std::string rhsId = rhsEntry.stableId != nullptr ? rhsEntry.stableId : "";
        return lhsId < rhsId;
    });
    out << "  \"hostParameters\": [\n";
    for (size_t i = 0; i < hostParameterIndices.size(); ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(
            hostParameterIndices[i]);
        out << "    {\"projection\": \"plugin\", \"index\": " << hostParameterIndices[i] << ", \"stableId\": \""
            << jsonEscape(entry.stableId) << "\", \"displayName\": \"" << jsonEscape(entry.displayName)
            << "\", \"defaultNorm\": " << entry.defaultNorm << "}";
        out << (i + 1 == hostParameterIndices.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    out << "  \"sequencer\": {\"slots\": " << static_cast<unsigned>(kSequencerSlotCount)
        << ", \"directionChoices\": [\"<\", \">\", \"RND\"], \"defaultDirection\": \">\", "
           "\"speedChoices\": [\"/2\", \"/1.5\", \"1\", \"x1.5\", \"x2\"], \"defaultSpeed\": \"1\"}\n";
    out << "}\n";
    return out.str();
}

inline std::string buildReviewerReportMarkdown()
{
    std::ostringstream out;
    out << "# Froggers v2 Manifest Report\n\n";
    out << "- Authority: C++ declarations\n";
    out << "- Product rows: " << productRowCount() << "\n";
    out << "- Host parameters: " << HostParameterInventoryV2::kCount << "\n";
    out << "- Permanent modulation sources: " << kPermanentModulationSources.size() << "\n";
    out << "- Sequencer slots: " << static_cast<unsigned>(kSequencerSlotCount) << "\n\n";
    out << "## Validation Summary\n\n";
    const ValidationSummary validation = buildValidationSummary();
    out << "| Check | Value |\n";
    out << "|---|---|\n";
    out << "| duplicateStableIds | " << (validation.duplicateStableIds ? "true" : "false") << " |\n";
    out << "| invalidPageRowRefs | " << (validation.invalidPageRowRefs ? "true" : "false") << " |\n";
    out << "| missingDisplayNames | " << (validation.missingDisplayNames ? "true" : "false") << " |\n";
    out << "| invalidRanges | " << (validation.invalidRanges ? "true" : "false") << " |\n";
    out << "| invalidDefaults | " << (validation.invalidDefaults ? "true" : "false") << " |\n";
    out << "| invalidProjectionOverlays | " << (validation.invalidProjectionOverlays ? "true" : "false") << " |\n";
    out << "| missingOscilloscopeTaps | " << (validation.missingOscilloscopeTaps ? "true" : "false") << " |\n";
    out << "| invalidRandomizationScopes | " << (validation.invalidRandomizationScopes ? "true" : "false") << " |\n";
    out << "| invalidSequencerStepCounts | " << (validation.invalidSequencerStepCounts ? "true" : "false") << " |\n";
    out << "| missingWrittenUnwrittenState | " << (validation.missingWrittenUnwrittenState ? "true" : "false")
        << " |\n";
    out << "| invalidSequencerDirectionSpeed | "
        << (validation.invalidSequencerDirectionSpeed ? "true" : "false") << " |\n";
    out << "| invalidSourceLaneCounts | " << (validation.invalidSourceLaneCounts ? "true" : "false") << " |\n";
    out << "| duplicateSourceIds | " << (validation.duplicateSourceIds ? "true" : "false") << " |\n";
    out << "| missingSourceColorsGroups | " << (validation.missingSourceColorsGroups ? "true" : "false")
        << " |\n";
    out << "| invalidExternalAudioAvailability | "
        << (validation.invalidExternalAudioAvailability ? "true" : "false") << " |\n\n";
    out << "## Manifest Family Schema\n\n";
    out << "- Product controls\n";
    out << "- Projection overlays\n";
    out << "- Runtime controls\n";
    out << "- Host parameter mapping\n";
    out << "- Hardware controls\n";
    out << "- Layout groups\n";
    out << "- Reserved VCV schema-only overlay fields\n";
    out << "- Visibility fields: hidden, readOnly, interactive\n\n";
    out << "## Controller Targets\n\n";
    out << "| Stable ID | Display | Surface | Binding |\n";
    out << "|---|---|---|---|\n";
    for (const ControllerTargetDeclaration& target : kControllerTargetDeclarations)
    {
        out << "| `" << target.stableId << "` | " << target.displayName << " | " << target.surface << " | "
            << target.bindingRole << " |\n";
    }
    out << "\n";
    out << "## Randomization Scope Controls\n\n";
    out << "| Stable ID | Display | Choices | All | Mod |\n";
    out << "|---|---|---|---|---|---|\n";
    for (const RandomizationScopeControl& control : kRandomizationScopeControls)
    {
        out << "| `" << control.stableId << "` | " << control.displayName << " | ";
        out << control.choices[0].displayName << " / " << control.choices[1].displayName << " | ";
        out << (control.consumedByRandomizeAll ? "yes" : "no") << " | "
            << (control.consumedByRandomizeMod ? "yes" : "no") << " |\n";
    }
    out << "\n## Sequencer Slots\n\n";
    out << "| Index | Written State Field | Persistent | Lockable |\n";
    out << "|---|---|---|---|\n";
    for (const SequencerSlot& slot : kSequencerSlots)
    {
        out << "| " << static_cast<unsigned>(slot.index) << " | `" << slot.writtenStateField << "` | "
            << (slot.sequencerPersistent ? "yes" : "no") << " | "
            << (slot.sequencerLockable ? "yes" : "no") << " |\n";
    }
    out << "\n";
    out << "## Projections\n\n";
    out << "| Projection | Display | Hidden | ReadOnly | Interactive | Reserved |\n";
    out << "|---|---|---|---|---|\n";
    for (const Projection& projection : kProjections)
    {
        out << "| `" << projection.id << "` | " << projection.displayName << " | "
            << (projection.hidden ? "yes" : "no") << " | " << (projection.readOnly ? "yes" : "no") << " | "
            << (projection.interactive ? "yes" : "no") << " | " << (projection.reserved ? "yes" : "no") << " |\n";
    }
    out << "\n## Permanent Modulation Sources\n\n";
    out << "| Stable ID | Display | Group | Rate | Availability |\n";
    out << "|---|---|---|---|---|\n";
    const auto sortedModulationSources = sortedByGroupThenStableId(
        kPermanentModulationSources,
        [](const ModulationSource& source) { return source.group; },
        [](const ModulationSource& source) { return source.stableId; });
    for (const ModulationSource& source : sortedModulationSources)
    {
        out << "| `" << source.stableId << "` | " << source.displayName << " | " << source.group << " | "
            << source.rateClass << " | " << source.availabilityRule << " |\n";
    }
    return out.str();
}

inline bool writeReviewerArtifacts(const std::filesystem::path& outputDir)
{
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec)
    {
        return false;
    }

    {
        std::ofstream json(outputDir / "froggers-v2-manifest.snapshot.json", std::ios::binary);
        if (!json)
        {
            return false;
        }
        json << buildSnapshotJson();
    }
    {
        std::ofstream report(outputDir / "froggers-v2-manifest-report.md", std::ios::binary);
        if (!report)
        {
            return false;
        }
        report << buildReviewerReportMarkdown();
    }
    return true;
}
} // namespace froggers_v2::manifest

namespace HostParameterInventoryV2
{
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
                entry.defaultNorm = static_cast<float>(SequencerState::kDefaultDirection)
                    / static_cast<float>(SequencerState::kDirectionChoiceCount - 1u);
                break;
            case 2:
            case 3:
                entry.defaultNorm = 0.0f;
                break;
            default:
                entry.defaultNorm = static_cast<float>(SequencerState::kDefaultSpeed)
                    / static_cast<float>(SequencerState::kSpeedChoiceCount - 1u);
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

    froggers_v2::manifest::formatInventoryStableId(
        entry.idBuffer, sizeof(entry.idBuffer), entry.axis, entry.page, entry.row, entry.index);
    froggers_v2::manifest::formatInventoryDisplayName(
        entry.nameBuffer, sizeof(entry.nameBuffer), entry.axis, entry.page, entry.row, entry.index);
    entry.stableId = entry.idBuffer;
    entry.displayName = entry.nameBuffer;
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
