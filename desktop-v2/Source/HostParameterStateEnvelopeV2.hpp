#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "HostParameterInventoryV2.hpp"
#include "HostParameterRoutingV2.hpp"
#include "SequencerState.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

// Packet 11 (VST/AU hosted projection) gap fill.
//
// The legacy SimPresetSnapshot payload (sim/SimPresetSnapshot.hpp) only captures the
// PageManager knob/mod-depth axes (HostParameterInventoryV2::Axis::PageKnob / PageModDepth),
// DelayState, and AudioPairArState — by copying each Parameter's raw pre-processing
// m_knobValue. It never captures Axis::GlobalCrunchy, VcoMorph, or SceneBlend. Left alone,
// those manifest-owned host parameters would silently reset to their construction defaults
// every time a DAW closes and reopens a saved project (a fresh FroggersTigaAudioProcessorV2 +
// AudioEngine instance is constructed, then setStateInformation replays only the legacy blob).
//
// This header adds a small, additive, versioned tail that serializes exactly those
// currently-unsaved axes (indices [kFirstNonPageIndex, kCount) — GlobalCrunchy, VcoMorph x3,
// SceneBlend = 5 stable IDs), by inventory index, which is a
// pure/deterministic function of each entry's stable ID (see FroggersV2AppManifest.hpp
// buildDescriptorAt / formatInventoryStableId — the single authority). Consuming only:
// HostParameterInventoryV2::buildDescriptorAt and HostParameterRoutingV2::readValue/applyValue.
// No private/parallel ID mapping is introduced here.
//
// Deliberately NOT covering Axis::PageKnob / PageModDepth here: those already round-trip
// correctly through the legacy raw-value SimPresetSnapshot payload, and several PageKnob rows
// are read back through the "V2Fuego" musical curve (HostParameterRoutingV2::readValue ->
// DesktopHostIO::GetPageParam -> Page::GetParamV2), which folds in GlobalCrunchy and the page's
// "crispy" row value. That read-time transform is not its own inverse, so re-applying a
// Fuego-transformed read as if it were a raw knob position would drift the stored value on every
// save/reload cycle. Restricting this tail to the axes with no such read-time transform
// keeps every entry it touches a clean, exact round trip.
namespace HostParameterStateEnvelopeV2
{
constexpr size_t kFirstNonPageIndex =
    HostParameterInventoryV2::kPageKnobCount + HostParameterInventoryV2::kPageModDepthCount;
constexpr size_t kEntryCount = HostParameterInventoryV2::kCount - kFirstNonPageIndex;
constexpr size_t kTailBytes = kEntryCount * sizeof(float);

// Writes kTailBytes bytes to dest: one native-endian float per covered HostParameterInventoryV2
// entry (indices [kFirstNonPageIndex, kCount)), in inventory-index order (index -> stableId is a
// pure function, so the same index always denotes the same stable ID on both write and read).
inline void writeTail(DesktopHostIO& host,
                      DelayState& delay,
                      SequencerState& sequencer,
                      froggers_v2::FroggersV2ControlCore& core,
                      uint8_t* dest)
{
    for (size_t i = 0; i < kEntryCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry =
            HostParameterInventoryV2::buildDescriptorAt(kFirstNonPageIndex + i);
        const float value = HostParameterRoutingV2::readValue(entry, host, delay, sequencer, core);
        std::memcpy(dest + (i * sizeof(float)), &value, sizeof(float));
    }
}

// Inverse of writeTail: reads kTailBytes bytes from src and applies each value through
// HostParameterRoutingV2::applyValue, keyed by the same deterministic inventory index / stable
// ID mapping used above.
inline void readTail(const uint8_t* src,
                     DesktopHostIO& host,
                     DelayState& delay,
                     SequencerState& sequencer,
                     froggers_v2::FroggersV2ControlCore& core)
{
    for (size_t i = 0; i < kEntryCount; ++i)
    {
        float value = 0.0f;
        std::memcpy(&value, src + (i * sizeof(float)), sizeof(float));
        const HostParameterInventoryV2::RuntimeDescriptor entry =
            HostParameterInventoryV2::buildDescriptorAt(kFirstNonPageIndex + i);
        HostParameterRoutingV2::applyValue(entry, value, host, delay, sequencer, core);
    }
}
} // namespace HostParameterStateEnvelopeV2
