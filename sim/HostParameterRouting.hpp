#pragma once

#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "HostParameterInventory.hpp"
#include "HostParameterPendingStore.hpp"
#include "OwnedAllocationGuard.hpp"

#include <algorithm>
#include <cstddef>

namespace HostParameterRouting
{
inline float clamp01(float value)
{
    return std::min(std::max(value, 0.0f), 1.0f);
}

inline float readValue(const HostParameterInventory::Descriptor& entry,
                       DesktopHostIO& host,
                       const DelayState& delay)
{
    using Axis = HostParameterInventory::Axis;
    switch (entry.axis)
    {
        case Axis::PageKnob:
            return host.GetPageParam(entry.page, entry.row);
        case Axis::PageModDepth:
            return host.GetPageModDepth(entry.page, entry.row);
        case Axis::DelayKnob:
            return delay.getKnob(entry.row);
        case Axis::DelayModDepth:
            return delay.getModDepth(entry.row);
        case Axis::PairArKnob:
            return host.GetAudioPairArKnob(entry.index);
        case Axis::PairArModDepth:
            return host.GetAudioPairArModDepth(entry.index);
        case Axis::VcoMorph:
            return host.GetVcoMorph(entry.index);
    }
    return entry.defaultNorm;
}

inline void applyValue(const HostParameterInventory::Descriptor& entry,
                       float normalizedValue,
                       DesktopHostIO& host,
                       DelayState& delay)
{
    const float value = clamp01(normalizedValue);
    using Axis = HostParameterInventory::Axis;
    switch (entry.axis)
    {
        case Axis::PageKnob:
            host.SetPageKnob(entry.page, entry.row, value);
            break;
        case Axis::PageModDepth:
            host.SetPageModDepth(entry.page, entry.row, value);
            break;
        case Axis::DelayKnob:
            delay.setKnob(entry.row, value);
            break;
        case Axis::DelayModDepth:
            delay.setModDepth(entry.row, value);
            break;
        case Axis::PairArKnob:
            host.SetAudioPairArKnob(entry.index, value);
            break;
        case Axis::PairArModDepth:
            host.SetAudioPairArModDepth(entry.index, value);
            break;
        case Axis::VcoMorph:
            host.SetVcoMorph(entry.index, value);
            break;
    }
}

inline void applyPending(DesktopHostIO& host,
                         DelayState& delay,
                         HostParameterPendingStore& pendingStore)
{
    FROGGERS_OWNED_ALLOCATION_GUARD();
    for (size_t i = 0; i < HostParameterInventory::kCount; ++i)
    {
        if (pendingStore.dirty[i].exchange(0, std::memory_order_acq_rel) == 0)
        {
            continue;
        }
        const float value = pendingStore.values[i].load(std::memory_order_relaxed);
        applyValue(HostParameterInventory::descriptorAt(i), value, host, delay);
    }
}

inline bool inventoryMatchesRegistryTable()
{
    return HostParameterInventory::validateInventory();
}
} // namespace HostParameterRouting
