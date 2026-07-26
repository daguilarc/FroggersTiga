#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "HostParameterInventoryV2.hpp"
#include "HostParameterPendingStoreV2.hpp"
#include "OwnedAllocationGuard.hpp"
#include "SequencerState.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace HostParameterRoutingV2
{
inline float clamp01(float value)
{
    return std::min(std::max(value, 0.0f), 1.0f);
}

inline float sequencerNormToBpm(float norm)
{
    return 20.0f + clamp01(norm) * (300.0f - 20.0f);
}

inline float sequencerBpmToNorm(float bpm)
{
    return clamp01((bpm - 20.0f) / (300.0f - 20.0f));
}

inline uint8_t sequencerNormToDirection(float norm)
{
    if (SequencerState::kDirectionChoiceCount <= 1)
    {
        return 0;
    }
    const float scaled = clamp01(norm) * static_cast<float>(SequencerState::kDirectionChoiceCount - 1u);
    return static_cast<uint8_t>(std::round(scaled));
}

inline float sequencerDirectionToNorm(uint8_t direction)
{
    if (SequencerState::kDirectionChoiceCount <= 1)
    {
        return 0.0f;
    }
    return clamp01(static_cast<float>(direction) / static_cast<float>(SequencerState::kDirectionChoiceCount - 1u));
}

inline uint8_t sequencerNormToSpeed(float norm)
{
    if (SequencerState::kSpeedChoiceCount <= 1)
    {
        return 0;
    }
    const float scaled = clamp01(norm) * static_cast<float>(SequencerState::kSpeedChoiceCount - 1u);
    return static_cast<uint8_t>(std::round(scaled));
}

inline float sequencerSpeedToNorm(uint8_t speedChoice)
{
    if (SequencerState::kSpeedChoiceCount <= 1)
    {
        return 0.0f;
    }
    return clamp01(static_cast<float>(speedChoice) / static_cast<float>(SequencerState::kSpeedChoiceCount - 1u));
}

inline bool isDelayUiPage(uint8_t page)
{
    return page == HostParameterInventoryV2::kDelayUiPage;
}

inline bool isAdsrUiPage(uint8_t page)
{
    return page == HostParameterInventoryV2::kAdsrUiPage;
}

inline uint8_t pmPageForUiPage(uint8_t page)
{
    if (isAdsrUiPage(page))
    {
        return HostParameterInventoryV2::kPmAdsrPage;
    }
    // The Random S&H module UI page was deleted, but the shared engine PageManager
    // still carries the Marbles page at PM index 1 (it drives the retained Random S&H
    // 1/2 mod lanes at default knob values). So every desktop-v2 UI page after Audio
    // maps one PM slot higher: Reverb UI1->PM2, Filter UI2->PM3, Drive UI3->PM4. Delay
    // is routed through DelayState (isDelayUiPage) and never reaches this function.
    if (page == 0)
    {
        return 0;
    }
    return static_cast<uint8_t>(page + 1);
}

// Task 7.4 (D11/D14): the Cross-coupler UI row was removed from the desktop-v2
// Audio page, but its underlying shared-engine XCPL param slot (engine row 3)
// is kept allocated for Daisy/v1 index stability -- it is only hidden from
// this V2 UI and neutralized in the flag-gated DSP. That leaves the Audio
// UI-row numbering (0..6: 3 pitch, 3 PM, Crispy) offset by one engine row
// from PM1A/PM2A/OLVL(PM3)/FUEG(engine rows 4/5/6/7) once past the pitch
// rows. This is the single place that translates a desktop-v2 Audio UI row
// into the engine row it actually reads/writes; every accessor below (and
// any other direct engine-param access keyed by UI page/row) must route
// through this so the hidden XCPL slot is skipped consistently.
inline uint8_t engineRowForUiRow(uint8_t uiPage, uint8_t uiRow)
{
    if (uiPage == 0 && uiRow >= 3)
    {
        return static_cast<uint8_t>(uiRow + 1);
    }
    return uiRow;
}

inline float readPageKnob(uint8_t uiPage, uint8_t row, DesktopHostIO& host, DelayState& delay)
{
    if (isDelayUiPage(uiPage))
    {
        return delay.getKnob(row);
    }
    return host.GetPageParam(pmPageForUiPage(uiPage), engineRowForUiRow(uiPage, row));
}

inline void applyPageKnob(uint8_t uiPage, uint8_t row, float value, DesktopHostIO& host, DelayState& delay)
{
    if (isDelayUiPage(uiPage))
    {
        delay.setKnob(row, value);
        return;
    }
    host.SetPageKnob(pmPageForUiPage(uiPage), engineRowForUiRow(uiPage, row), value);
}

inline float readPageModDepth(uint8_t uiPage, uint8_t row, DesktopHostIO& host, DelayState& delay)
{
    if (isDelayUiPage(uiPage))
    {
        return delay.getModDepth(row);
    }
    return host.GetPageModDepth(pmPageForUiPage(uiPage), engineRowForUiRow(uiPage, row));
}

inline uint8_t readPageModSource(uint8_t uiPage, uint8_t row, DesktopHostIO& host, DelayState* delay)
{
    if (isDelayUiPage(uiPage))
    {
        return delay != nullptr ? delay->getModSource(row) : static_cast<uint8_t>(255);
    }
    return host.GetPageModSource(pmPageForUiPage(uiPage), engineRowForUiRow(uiPage, row));
}

inline void applyPageModSource(uint8_t uiPage,
                               uint8_t row,
                               uint8_t modIndex,
                               DesktopHostIO& host,
                               DelayState* delay)
{
    if (isDelayUiPage(uiPage))
    {
        if (delay != nullptr)
        {
            host.EnqueueDelaySetModSource(row, modIndex);
        }
        return;
    }
    host.SetPageModSource(pmPageForUiPage(uiPage), engineRowForUiRow(uiPage, row), modIndex);
}

inline void applyPageModDepth(uint8_t uiPage, uint8_t row, float value, DesktopHostIO& host, DelayState& delay)
{
    if (isDelayUiPage(uiPage))
    {
        delay.setModDepth(row, value);
        return;
    }
    host.SetPageModDepth(pmPageForUiPage(uiPage), engineRowForUiRow(uiPage, row), value);
}

inline float readValue(const HostParameterInventoryV2::RuntimeDescriptor& entry,
                       DesktopHostIO& host,
                       DelayState& delay,
                       SequencerState& sequencer,
                       froggers_v2::FroggersV2ControlCore& core)
{
    using Axis = HostParameterInventoryV2::Axis;
    switch (entry.axis)
    {
        case Axis::PageKnob:
            return readPageKnob(entry.page, entry.row, host, delay);
        case Axis::PageModDepth:
            return readPageModDepth(entry.page, entry.row, host, delay);
        case Axis::GlobalCrunchy:
            return host.GetGlobalCrunchy();
        case Axis::VcoMorph:
            return host.GetVcoMorph(entry.index);
        case Axis::SceneBlend:
            return core.sceneBlend();
    }
    return entry.defaultNorm;
}

inline void applyValue(const HostParameterInventoryV2::RuntimeDescriptor& entry,
                       float normalizedValue,
                       DesktopHostIO& host,
                       DelayState& delay,
                       SequencerState& sequencer,
                       froggers_v2::FroggersV2ControlCore& core)
{
    const float value = clamp01(normalizedValue);
    using Axis = HostParameterInventoryV2::Axis;
    switch (entry.axis)
    {
        case Axis::PageKnob:
            applyPageKnob(entry.page, entry.row, value, host, delay);
            break;
        case Axis::PageModDepth:
            applyPageModDepth(entry.page, entry.row, value, host, delay);
            break;
        case Axis::GlobalCrunchy:
            host.SetGlobalCrunchy(value);
            core.setGlobalCrunchy(value);
            break;
        case Axis::VcoMorph:
            host.SetVcoMorph(entry.index, value);
            break;
        case Axis::SceneBlend:
            core.setSceneBlend(value);
            break;
    }
}

inline void applyPending(DesktopHostIO& host,
                         DelayState& delay,
                         SequencerState& sequencer,
                         froggers_v2::FroggersV2ControlCore& core,
                         HostParameterPendingStoreV2& pendingStore)
{
    FROGGERS_OWNED_ALLOCATION_GUARD();
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        if (pendingStore.dirty[i].exchange(0, std::memory_order_acq_rel) == 0)
        {
            continue;
        }
        const float value = pendingStore.values[i].load(std::memory_order_relaxed);
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        applyValue(entry, value, host, delay, sequencer, core);
    }
}

inline bool inventoryMatchesRegistryTable()
{
    return HostParameterInventoryV2::validateInventory();
}
} // namespace HostParameterRoutingV2
