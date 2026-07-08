#include "FroggersV2HostBridge.hpp"

#include "HostParameterInventoryV2.hpp"
#include "HostParameterRoutingV2.hpp"
#include "PermanentModTapRack.hpp"

#include <algorithm>

namespace froggers_v2
{
namespace
{
uint8_t engineModFromInternal(uint8_t internal)
{
    if (internal == kNoSelection || internal >= kNumModSources)
    {
        return 255;
    }
    return internal;
}
} // namespace

FroggersV2HostBridge::FroggersV2HostBridge(FroggersV2ControlCore& core, DesktopHostIO& host)
    : m_core(core)
    , m_host(host)
{
    m_host.m_onSequencerStepAdvance = [this]() { onSequencerStepAdvance(); };
}

FroggersV2HostBridge::~FroggersV2HostBridge()
{
    m_host.m_onSequencerStepAdvance = nullptr;
}

void FroggersV2HostBridge::setOnStepCaptured(std::function<void(uint8_t step)> callback)
{
    m_onStepCaptured = std::move(callback);
}

void FroggersV2HostBridge::captureLiveToSequencerStep(uint8_t step)
{
    SequencerSlotPayload captured;
    m_core.captureSequencerSlotPayload(captured);
    m_host.m_sequencer.captureStep(step, captured);
    if (m_onStepCaptured)
    {
        m_onStepCaptured(step);
    }
}

void FroggersV2HostBridge::recallSequencerStep(uint8_t step)
{
    if (step >= SequencerState::kSlotCount || !m_host.m_sequencer.slotWritten(step))
    {
        return;
    }
    m_core.applySequencerSlotPayload(m_host.m_sequencer.m_slots[step].payload);
    syncAllModRoutesToHost();
    syncToHost();
}

void FroggersV2HostBridge::onSequencerStepAdvance()
{
    SequencerState& seq = m_host.m_sequencer;
    if (seq.m_writeSeqArm)
    {
        if (seq.m_writeSeqJustStarted)
        {
            seq.m_writeSeqJustStarted = false;
        }
        else
        {
            const uint8_t playhead = seq.m_playhead;
            const uint8_t stepLeft =
                static_cast<uint8_t>((playhead + SequencerState::kSlotCount - 1u) % SequencerState::kSlotCount);
            captureLiveToSequencerStep(stepLeft);
        }
        seq.m_editStep = seq.m_playhead;
    }

    if (seq.slotWritten(seq.m_playhead) && !(seq.slotLocked(seq.m_playhead)))
    {
        m_core.applySequencerSlotPayload(seq.currentStep());
        syncAllModRoutesToHost();
    }

    m_core.bus().push(MessageIn::SequencerStepClock(seq.m_playhead));
    m_core.processBus();
    syncToHost();
}

void FroggersV2HostBridge::syncAllModRoutesToHost()
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = HostParameterInventoryV2::rowsForUiPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            syncModRoutes(page, row, ModRouteDirection::ToHost);
        }
    }
}

void FroggersV2HostBridge::syncModRoutes(uint8_t page, uint8_t row, ModRouteDirection direction)
{
    DelayState* delay = m_host.m_delay;
    if (direction == ModRouteDirection::FromHost)
    {
        const uint8_t engineMod =
            HostParameterRoutingV2::readPageModSource(page, row, m_host, delay);
        DelayState fallbackDelay;
        DelayState& delayRef = delay != nullptr ? *delay : fallbackDelay;
        const float depth =
            HostParameterRoutingV2::readPageModDepth(page, row, m_host, delayRef);
        m_core.applyHostModRoute(page, row, engineMod, depth);
        return;
    }

    const uint8_t internal = m_core.assignedModSource(page, row);
    const uint8_t engineMod = engineModFromInternal(internal);
    const float depth = m_core.assignedModDepth(page, row);
    HostParameterRoutingV2::applyPageModSource(page, row, engineMod, m_host, delay);
    if (delay != nullptr)
    {
        HostParameterRoutingV2::applyPageModDepth(page, row, depth, m_host, *delay);
        return;
    }
    m_host.SetPageModDepth(HostParameterRoutingV2::pmPageForUiPage(page), row, depth);
}

void FroggersV2HostBridge::syncFromHostModRoutes()
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = HostParameterInventoryV2::rowsForUiPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            syncModRoutes(page, row, ModRouteDirection::FromHost);
        }
    }
    m_core.populateUiState();
}

void FroggersV2HostBridge::syncToHost()
{
    DelayState fallbackDelay;
    DelayState& delay = m_host.m_delay != nullptr ? *m_host.m_delay : fallbackDelay;
    const uint8_t page = m_core.activePage();
    for (uint8_t slot = 0; slot < m_core.visibleCount(); ++slot)
    {
        const uint8_t row = m_core.visibleRowForSlot(slot);
        if (row == kNoSelection)
        {
            continue;
        }
        const FroggersV2ControlCore::EffectiveRow effective = m_core.effectiveRow(page, row);
        HostParameterRoutingV2::applyPageKnob(page, row, effective.effective, m_host, delay);
    }

    m_host.SetGlobalCrunchy(m_core.globalCrunchy());

    if (!m_vcoMorphDefaultsApplied)
    {
        for (uint8_t i = 0; i < HostParameterInventoryV2::kMorphCount; ++i)
        {
            m_host.SetVcoMorph(i, HostParameterInventoryV2::vcoMorphDefault(i));
        }
        m_vcoMorphDefaultsApplied = true;
    }
}
} // namespace froggers_v2
