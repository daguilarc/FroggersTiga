#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "manifest/FroggersV2AppManifest.hpp"
#include "ui/CvLaneHistoryStore.hpp"
#include "ui/EncoderRingComponent.hpp"

#include <array>
#include <cstdint>

namespace desktop_v2
{
inline juce::Colour colourFromManifestRgb(uint32_t rgb)
{
    return juce::Colour(static_cast<juce::uint8>((rgb >> 16) & 0xffu),
                        static_cast<juce::uint8>((rgb >> 8) & 0xffu),
                        static_cast<juce::uint8>(rgb & 0xffu));
}

// Shared detail-grid underlay bind for SubmodulePagePanel + AdsrPagePanel.
// Target (Back) clears; lane cells bind non-owning views from the store.
inline void bindDetailUnderlays(std::array<EncoderRingComponent, froggers_v2::kUiSlots>& rings,
                                froggers_v2::FroggersV2ControlCore* core,
                                const CvLaneHistoryStore* store,
                                bool detailOpen)
{
    if (!detailOpen || core == nullptr || store == nullptr)
    {
        for (auto& ring : rings)
        {
            ring.clearUnderlay();
        }
        return;
    }

    for (uint8_t slot = 0; slot < froggers_v2::kUiSlots; ++slot)
    {
        EncoderRingComponent& ring = rings[slot];
        if (core->visibleSlotIsTarget(slot))
        {
            ring.clearUnderlay();
            continue;
        }

        const uint8_t lane = core->visibleModIndexForSlot(slot);
        if (lane >= CvLaneHistoryStore::kNumLanes)
        {
            ring.clearUnderlay();
            continue;
        }

        const auto& source = froggers_v2::manifest::kPermanentModulationSources[lane];
        ring.setUnderlay(store->samples(lane),
                         store->bufferSize(),
                         store->writeIndex(lane),
                         colourFromManifestRgb(source.colorRgb));
    }
}
} // namespace desktop_v2
