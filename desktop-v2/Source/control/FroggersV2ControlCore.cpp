#include "FroggersV2ControlCore.hpp"

#include "HostParameterInventoryV2.hpp"
#include "V2ModTapBank.hpp"

#include <algorithm>
#include <cmath>

namespace froggers_v2
{
namespace
{
constexpr float kTurnStep = 0.04f;
constexpr uint8_t kEncoderCount = 10;
constexpr uint8_t kCrunchyPage = kNumHostPages;
constexpr uint8_t kClockSourceStart = 6;
} // namespace

bool MessageInBus::push(const MessageIn& message)
{
    const uint32_t write = m_write.load(std::memory_order_relaxed);
    const uint32_t read = m_read.load(std::memory_order_acquire);
    if (write - read >= kCapacity)
    {
        return false;
    }
    m_queue[write % kCapacity] = message;
    m_write.store(write + 1, std::memory_order_release);
    return true;
}

bool MessageInBus::pop(MessageIn& message)
{
    const uint32_t read = m_read.load(std::memory_order_relaxed);
    const uint32_t write = m_write.load(std::memory_order_acquire);
    if (read >= write)
    {
        return false;
    }
    message = m_queue[read % kCapacity];
    m_read.store(read + 1, std::memory_order_release);
    return true;
}

void MessageInBus::clear()
{
    m_read.store(m_write.load(std::memory_order_acquire), std::memory_order_release);
}

FroggersV2ControlCore::FroggersV2ControlCore()
{
    seedSceneCentersFromDefaults();
    for (uint8_t source = 0; source < kNumModSources; ++source)
    {
        m_sourceValues[source] = 0.5f;
    }
    m_externalMidiMods.fill(0.0f);
    rebuildVisibleSlots();
    compute();
    populateUiState();
}

float FroggersV2ControlCore::clamp01(float value)
{
    return std::min(std::max(value, 0.0f), 1.0f);
}

float FroggersV2ControlCore::clampSigned(float value)
{
    return std::min(std::max(value, -1.0f), 1.0f);
}

bool FroggersV2ControlCore::isAdsrPage(uint8_t page)
{
    return page == 6;
}

uint8_t FroggersV2ControlCore::crispyRowForPage(uint8_t page)
{
    if (page == 0)
    {
        return 7;
    }
    if (page == 6)
    {
        return 6;
    }
    return 9;
}

uint8_t FroggersV2ControlCore::visibleRowForSlot(uint8_t slot) const
{
    if (slot >= m_visibleCount)
    {
        return kNoSelection;
    }
    return m_visibleSlots[slot].row;
}

float FroggersV2ControlCore::globalCrunchy() const
{
    return blendedSceneCenter(m_crunchy);
}

void FroggersV2ControlCore::setGlobalCrunchy(float value)
{
    const uint8_t scene = activeSceneOrdinal();
    m_crunchy.sceneCenter[scene] = clamp01(value);
    compute();
    populateUiState();
}

float FroggersV2ControlCore::sceneBlend() const
{
    return m_sceneBlend;
}

float FroggersV2ControlCore::gestureWeight(uint8_t lane) const
{
    if (lane >= kNumGestures)
    {
        return 0.0f;
    }
    return m_gestureWeights[lane];
}

void FroggersV2ControlCore::setSceneBlend(float value)
{
    m_sceneBlend = clamp01(value);
    compute();
    populateUiState();
}

void FroggersV2ControlCore::setGestureWeight(uint8_t lane, float value)
{
    if (lane >= kNumGestures)
    {
        return;
    }
    m_gestureWeights[lane] = clampSigned(value);
    compute();
    populateUiState();
}

uint8_t FroggersV2ControlCore::assignedModSource(uint8_t page, uint8_t row) const
{
    if (page >= kNumHostPages || row >= rowsForPage(page))
    {
        return kNoSelection;
    }
    const ParamState& param = m_params[page][row];
    for (uint8_t i = 0; i < kNumModSources; ++i)
    {
        if (param.modSource[i] != kNoSelection)
        {
            return param.modSource[i];
        }
    }
    return kNoSelection;
}

void FroggersV2ControlCore::applyHostModRoute(uint8_t page,
                                              uint8_t row,
                                              uint8_t engineModIndex,
                                              float depth)
{
    if (page >= kNumHostPages || row >= rowsForPage(page))
    {
        return;
    }
    ParamState& param = m_params[page][row];
    for (uint8_t i = 0; i < kNumModSources; ++i)
    {
        param.modSource[i] = kNoSelection;
        param.modDepth[i] = 0.0f;
    }
    if (engineModIndex >= V2ModTapBank::kFirstIndex && engineModIndex <= V2ModTapBank::kLastIndex)
    {
        const uint8_t internal =
            static_cast<uint8_t>(engineModIndex - V2ModTapBank::kFirstIndex);
        param.modSource[0] = internal;
        param.modDepth[0] = clampSigned(depth);
    }
}

float FroggersV2ControlCore::assignedModDepth(uint8_t page, uint8_t row) const
{
    if (page >= kNumHostPages || row >= rowsForPage(page))
    {
        return 0.0f;
    }
    const ParamState& param = m_params[page][row];
    for (uint8_t i = 0; i < kNumModSources; ++i)
    {
        if (param.modSource[i] != kNoSelection)
        {
            return param.modDepth[i];
        }
    }
    return 0.0f;
}

void FroggersV2ControlCore::processBus()
{
    MessageIn message;
    while (m_bus.pop(message))
    {
        applyMessage(message);
    }
    compute();
    populateUiState();
}

void FroggersV2ControlCore::compute()
{
    // Computation is pulled by bridge/UI from the same state snapshot.
}

void FroggersV2ControlCore::populateUiState()
{
    for (uint8_t slot = 0; slot < m_visibleCount; ++slot)
    {
        const VisibleSlot& visible = m_visibleSlots[slot];
        if (visible.isTarget)
        {
            const ParamState& target = m_params[m_activePage][visible.row];
            const float blended = blendedSceneCenter(target);
            m_uiState.sceneLeft[slot].store(blended, std::memory_order_release);
            m_uiState.sceneRight[slot].store(blended, std::memory_order_release);
            m_uiState.effective[slot].store(blended, std::memory_order_release);
            m_uiState.arcMin[slot].store(0.0f, std::memory_order_release);
            m_uiState.arcMax[slot].store(1.0f, std::memory_order_release);
            m_uiState.modulatorsMask[slot].store(0, std::memory_order_release);
            m_uiState.gesturesMask[slot].store(0, std::memory_order_release);
            continue;
        }
        const EffectiveRow row = effectiveRow(m_activePage, visible.row);
        m_uiState.sceneLeft[slot].store(row.sceneLeft, std::memory_order_release);
        m_uiState.sceneRight[slot].store(row.sceneRight, std::memory_order_release);
        m_uiState.effective[slot].store(row.effective, std::memory_order_release);
        m_uiState.arcMin[slot].store(row.arcMin, std::memory_order_release);
        m_uiState.arcMax[slot].store(row.arcMax, std::memory_order_release);
        m_uiState.modulatorsMask[slot].store(row.modulatorsMask, std::memory_order_release);
        m_uiState.gesturesMask[slot].store(row.gesturesMask, std::memory_order_release);
    }

    for (uint8_t slot = m_visibleCount; slot < kUiSlots; ++slot)
    {
        m_uiState.sceneLeft[slot].store(0.0f, std::memory_order_release);
        m_uiState.sceneRight[slot].store(0.0f, std::memory_order_release);
        m_uiState.effective[slot].store(0.0f, std::memory_order_release);
        m_uiState.arcMin[slot].store(0.0f, std::memory_order_release);
        m_uiState.arcMax[slot].store(0.0f, std::memory_order_release);
        m_uiState.modulatorsMask[slot].store(0, std::memory_order_release);
        m_uiState.gesturesMask[slot].store(0, std::memory_order_release);
    }

    m_uiState.activePage.store(m_activePage, std::memory_order_release);
    m_uiState.modViewTargetRow.store(
        m_modView.open ? m_modView.targetRow : kNoSelection,
        std::memory_order_release);
    m_uiState.visibleCount.store(m_visibleCount, std::memory_order_release);
    m_uiState.leftSceneOrdinal.store(m_sceneLeftOrdinal, std::memory_order_release);
    m_uiState.rightSceneOrdinal.store(m_sceneRightOrdinal, std::memory_order_release);
    m_uiState.sceneBlend.store(m_sceneBlend, std::memory_order_release);
    m_uiState.activeGesture.store(m_activeGestureLane, std::memory_order_release);

    const EffectiveRow crunchyRow = computeEffective(m_crunchy);
    m_uiState.crunchySceneLeft.store(crunchyRow.sceneLeft, std::memory_order_release);
    m_uiState.crunchySceneRight.store(crunchyRow.sceneRight, std::memory_order_release);
    m_uiState.crunchyEffective.store(crunchyRow.effective, std::memory_order_release);
    m_uiState.crunchyArcMin.store(crunchyRow.arcMin, std::memory_order_release);
    m_uiState.crunchyArcMax.store(crunchyRow.arcMax, std::memory_order_release);

    m_uiState.version.fetch_add(1, std::memory_order_acq_rel);
}

FroggersV2ControlCore::EffectiveRow FroggersV2ControlCore::effectiveRow(uint8_t page, uint8_t row) const
{
    return computeEffective(m_params[page][row]);
}

void FroggersV2ControlCore::setSequencerState(SequencerState* sequencer)
{
    m_sequencer = sequencer;
}

void FroggersV2ControlCore::applySequencerStepSnapshot(const SequencerStepSnapshot& snapshot)
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            ParamState& param = m_params[page][row];
            for (uint8_t scene = 0; scene < kNumScenes; ++scene)
            {
                param.sceneCenter[scene] =
                    clamp01(snapshot.sceneCenter[page][row][scene]);
            }
        }
    }
    for (uint8_t scene = 0; scene < kNumScenes; ++scene)
    {
        m_crunchy.sceneCenter[scene] = clamp01(snapshot.crunchySceneCenter[scene]);
    }
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        m_gestureWeights[lane] = clamp01(snapshot.gestureWeight[lane]);
    }
    compute();
    populateUiState();
}

void FroggersV2ControlCore::captureSequencerStepSnapshot(SequencerStepSnapshot& out) const
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            const ParamState& param = m_params[page][row];
            for (uint8_t scene = 0; scene < kNumScenes; ++scene)
            {
                out.sceneCenter[page][row][scene] = param.sceneCenter[scene];
            }
        }
    }
    for (uint8_t scene = 0; scene < kNumScenes; ++scene)
    {
        out.crunchySceneCenter[scene] = m_crunchy.sceneCenter[scene];
    }
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        out.gestureWeight[lane] = m_gestureWeights[lane];
    }
    out.hasData = true;
}

void FroggersV2ControlCore::captureFactoryStepSnapshot(SequencerStepSnapshot& out) const
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            const float center = HostParameterInventoryV2::pageKnobDefault(page, row);
            for (uint8_t scene = 0; scene < kNumScenes; ++scene)
            {
                out.sceneCenter[page][row][scene] = center;
            }
        }
    }
    out.crunchySceneCenter.fill(0.0f);
    out.gestureWeight.fill(0.0f);
    out.gate = false;
    out.hasData = true;
}

void FroggersV2ControlCore::zeroStepGestures(SequencerStepSnapshot& snapshot)
{
    snapshot.gestureWeight.fill(0.0f);
}

void FroggersV2ControlCore::randomizeFullStepSnapshot(SequencerStepSnapshot& out)
{
    randomizeSceneSlotsInto(out);
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        advanceRandState();
        out.gestureWeight[lane] = static_cast<float>(m_randState & 1023u) / 1023.0f;
    }
    advanceRandState();
    out.gate = (m_randState & 1u) != 0u;
    out.hasData = true;
}

void FroggersV2ControlCore::applyMessage(const MessageIn& message)
{
    switch (message.type)
    {
        case MessageIn::Type::ParamIncDec:
            onParamTurn(message.page, message.slot, message.value);
            break;
        case MessageIn::Type::ParamPress:
            onParamPress(message.page, message.slot);
            break;
        case MessageIn::Type::ShiftHeld:
            m_shiftHeld = message.value >= 0.5f;
            break;
        case MessageIn::Type::SceneSelect:
            onSceneSelect(message.index % kNumScenes);
            break;
        case MessageIn::Type::SceneBlend:
            m_sceneBlend = clamp01(message.value);
            break;
        case MessageIn::Type::GestureSelect:
            m_activeGestureLane = message.index < kNumGestures ? message.index : kNoSelection;
            break;
        case MessageIn::Type::GestureWeight:
            if (message.index < kNumGestures)
            {
                m_gestureWeights[message.index] = clamp01(message.value);
            }
            break;
        case MessageIn::Type::ModSourceAssign:
            onModSourceAssign(message.page, message.slot, message.index);
            break;
        case MessageIn::Type::SelectPage:
            if (message.page < kNumHostPages)
            {
                m_activePage = message.page;
                rebuildVisibleSlots();
            }
            break;
        case MessageIn::Type::RandAll:
            onRandAll();
            break;
        case MessageIn::Type::RandPage:
            onRandPage(message.page);
            break;
        case MessageIn::Type::ResetSequencerStep:
            onResetSequencerStep(message.slot);
            break;
        case MessageIn::Type::RandSequencerStep:
            onRandSequencerStep(message.slot, message.page);
            break;
        case MessageIn::Type::Clock:
            if (message.index == kNoSelection)
            {
                break;
            }
            if (message.index <= 1)
            {
                setExternalMidiMod(message.index, message.value);
                break;
            }
            if (message.index >= kClockSourceStart
                && message.index < (kClockSourceStart + kNumModSources))
            {
                const uint8_t source = static_cast<uint8_t>(message.index - kClockSourceStart);
                m_sourceValues[source] = clamp01(message.value);
            }
            break;
    }
}

void FroggersV2ControlCore::onParamTurn(uint8_t page, uint8_t slot, float delta)
{
    if (m_shiftHeld)
    {
        return;
    }
    if (page == kCrunchyPage)
    {
        const uint8_t scene = activeSceneOrdinal();
        m_crunchy.sceneCenter[scene] =
            clamp01(m_crunchy.sceneCenter[scene] + delta * kTurnStep);
        return;
    }
    if (page >= kNumHostPages)
    {
        return;
    }
    const uint8_t activeRow = slotToRow(page, slot);
    if (activeRow >= rowsForPage(page))
    {
        return;
    }
    if (m_modView.open && page == m_activePage)
    {
        const VisibleSlot visible = m_visibleSlots[slot];
        if (visible.isTarget)
        {
            return;
        }
        m_params[page][visible.row].modDepth[visible.modIndex] = clampSigned(
            m_params[page][visible.row].modDepth[visible.modIndex] + delta * kTurnStep);
        return;
    }
    ParamState& param = m_params[page][activeRow];
    if (m_activeGestureLane < kNumGestures)
    {
        param.gestureDepth[m_activeGestureLane] =
            clampSigned(param.gestureDepth[m_activeGestureLane] + delta * kTurnStep);
        return;
    }
    const uint8_t scene = activeSceneOrdinal();
    param.sceneCenter[scene] = clamp01(param.sceneCenter[scene] + delta * kTurnStep);
}

void FroggersV2ControlCore::onParamPress(uint8_t page, uint8_t slot)
{
    if (page == kCrunchyPage)
    {
        if (m_shiftHeld)
        {
            resetCrunchy();
        }
        return;
    }
    if (page >= kNumHostPages)
    {
        return;
    }
    if (m_shiftHeld)
    {
        const uint8_t row = slotToRow(page, slot);
        if (row < rowsForPage(page))
        {
            resetParameter(page, row);
        }
        return;
    }
    if (m_modView.open)
    {
        const VisibleSlot visible = m_visibleSlots[slot];
        if (visible.isTarget)
        {
            m_modView.open = false;
            m_modView.targetRow = kNoSelection;
            rebuildVisibleSlots();
        }
        return;
    }
    const uint8_t row = slotToRow(page, slot);
    if (row >= rowsForPage(page))
    {
        return;
    }
    const ParamState& param = m_params[page][row];
    bool hasAssignment = false;
    for (uint8_t i = 0; i < kNumModSources; ++i)
    {
        if (param.modSource[i] != kNoSelection)
        {
            hasAssignment = true;
            break;
        }
    }
    if (hasAssignment)
    {
        m_modView.open = true;
        m_modView.targetRow = row;
        rebuildVisibleSlots();
    }
}

void FroggersV2ControlCore::onSceneSelect(uint8_t sceneOrdinal)
{
    if (m_sceneSelectFlip == 0)
    {
        m_sceneLeftOrdinal = sceneOrdinal;
        m_sceneSelectFlip = 1;
        return;
    }
    m_sceneRightOrdinal = sceneOrdinal;
    m_sceneSelectFlip = 0;
}

void FroggersV2ControlCore::onModSourceAssign(uint8_t page, uint8_t row, uint8_t source)
{
    if (page >= kNumHostPages || row >= rowsForPage(page))
    {
        return;
    }
    ParamState& param = m_params[page][row];
    uint8_t insert = kNoSelection;
    for (uint8_t i = 0; i < kNumModSources; ++i)
    {
        if (param.modSource[i] == source)
        {
            insert = i;
            break;
        }
        if (param.modSource[i] == kNoSelection && insert == kNoSelection)
        {
            insert = i;
        }
    }
    if (insert == kNoSelection)
    {
        insert = 0;
    }
    param.modSource[insert] = source < kNumModSources ? source : kNoSelection;
    if (param.modSource[insert] == kNoSelection)
    {
        param.modDepth[insert] = 0.0f;
    }
    if (m_modView.open && m_modView.targetRow == row && page == m_activePage)
    {
        rebuildVisibleSlots();
    }
}

void FroggersV2ControlCore::advanceRandState()
{
    m_randState ^= (m_randState << 13);
    m_randState ^= (m_randState >> 17);
    m_randState ^= (m_randState << 5);
}

void FroggersV2ControlCore::randomizeSceneSlotValues(float (&scenes)[kNumScenes])
{
    for (uint8_t scene = 0; scene < kNumScenes; ++scene)
    {
        advanceRandState();
        scenes[scene] = static_cast<float>(m_randState & 1023u) / 1023.0f;
    }
}

void FroggersV2ControlCore::randomizeSceneSlotValues(std::array<float, kNumScenes>& scenes)
{
    randomizeSceneSlotValues(reinterpret_cast<float(&)[kNumScenes]>(scenes[0]));
}

void FroggersV2ControlCore::randomizeSceneSlotValues(ParamState& param)
{
    randomizeSceneSlotValues(param.sceneCenter);
}

void FroggersV2ControlCore::randomizeSceneSlotsInto(uint8_t page)
{
    if (page >= kNumHostPages)
    {
        return;
    }
    const uint8_t rowLimit = rowsForPage(page);
    for (uint8_t row = 0; row < rowLimit; ++row)
    {
        if (row == crispyRowForPage(page))
        {
            continue;
        }
        randomizeSceneSlotValues(m_params[page][row]);
    }
}

void FroggersV2ControlCore::randomizeSceneSlotsInto(SequencerStepSnapshot& snapshot)
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            if (row == crispyRowForPage(page))
            {
                continue;
            }
            randomizeSceneSlotValues(snapshot.sceneCenter[page][row]);
        }
    }
    randomizeSceneSlotValues(snapshot.crunchySceneCenter);
}

void FroggersV2ControlCore::randomizeSceneEndpointsAndBlend()
{
    advanceRandState();
    const uint8_t left = static_cast<uint8_t>(m_randState % kNumScenes);
    advanceRandState();
    uint8_t right = static_cast<uint8_t>(m_randState % (kNumScenes - 1));
    if (right >= left)
    {
        ++right;
    }
    m_sceneLeftOrdinal = left;
    m_sceneRightOrdinal = right;
    advanceRandState();
    m_sceneBlend = static_cast<float>(m_randState & 1023u) / 1023.0f;
    m_sceneSelectFlip = 0;
}

void FroggersV2ControlCore::resetCrunchy()
{
    for (uint8_t scene = 0; scene < kNumScenes; ++scene)
    {
        m_crunchy.sceneCenter[scene] = 0.0f;
    }
    compute();
    populateUiState();
}

uint8_t FroggersV2ControlCore::activeSceneOrdinal() const
{
    return static_cast<uint8_t>(std::round(
        static_cast<float>(m_sceneLeftOrdinal) * (1.0f - m_sceneBlend)
        + static_cast<float>(m_sceneRightOrdinal) * m_sceneBlend));
}

void FroggersV2ControlCore::onRandPage(uint8_t page)
{
    randomizeSceneSlotsInto(page);
}

void FroggersV2ControlCore::onResetSequencerStep(uint8_t step)
{
    if (!m_sequencer || step >= SequencerState::kMaxSteps)
    {
        return;
    }
    captureFactoryStepSnapshot(m_sequencer->m_steps[step]);
}

void FroggersV2ControlCore::onRandSequencerStep(uint8_t step, uint8_t scope)
{
    if (!m_sequencer)
    {
        return;
    }
    if (scope == kRandSeqScopeFullStep)
    {
        if (step >= SequencerState::kMaxSteps)
        {
            return;
        }
        randomizeFullStepSnapshot(m_sequencer->m_steps[step]);
        return;
    }

    randomizeSceneEndpointsAndBlend();

    if (scope == kRandSeqScopeStep)
    {
        const uint8_t editStep = m_sequencer->m_editStep;
        if (editStep >= SequencerState::kMaxSteps)
        {
            return;
        }
        randomizeSceneSlotsInto(m_sequencer->m_steps[editStep]);
        zeroStepGestures(m_sequencer->m_steps[editStep]);
        m_sequencer->m_steps[editStep].hasData = true;
        return;
    }

    if (scope != kRandSeqScopePattern)
    {
        return;
    }
    for (uint8_t i = 0; i < m_sequencer->m_patternLength; ++i)
    {
        if (m_sequencer->m_steps[i].hasData)
        {
            continue;
        }
        randomizeSceneSlotsInto(m_sequencer->m_steps[i]);
        zeroStepGestures(m_sequencer->m_steps[i]);
        m_sequencer->m_steps[i].hasData = true;
    }
}

void FroggersV2ControlCore::onRandAll()
{
    m_activeGestureLane = kNoSelection;
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        m_gestureWeights[lane] = 0.0f;
    }
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        randomizeSceneSlotsInto(page);
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            if (row == crispyRowForPage(page))
            {
                continue;
            }
            ParamState& param = m_params[page][row];
            for (uint8_t source = 0; source < kNumModSources; ++source)
            {
                advanceRandState();
                param.modDepth[source] = clampSigned(
                    static_cast<float>(static_cast<int32_t>(m_randState & 1023u) - 512) / 512.0f);
            }
            for (uint8_t lane = 0; lane < kNumGestures; ++lane)
            {
                param.gestureDepth[lane] = 0.0f;
            }
        }
    }
    randomizeSceneSlotValues(m_crunchy);
    randomizeSceneEndpointsAndBlend();
}

float FroggersV2ControlCore::sourceValue(uint8_t source) const
{
    if (source == kModSourceMidiCcA)
    {
        return m_externalMidiMods[0];
    }
    if (source == kModSourceMidiCcB)
    {
        return m_externalMidiMods[1];
    }
    if (source >= kNumModSources)
    {
        return 0.5f;
    }
    return m_sourceValues[source];
}

void FroggersV2ControlCore::setExternalMidiMod(uint8_t slot, float value)
{
    if (slot >= m_externalMidiMods.size())
    {
        return;
    }
    m_externalMidiMods[slot] = clamp01(value);
}

float FroggersV2ControlCore::externalMidiMod(uint8_t slot) const
{
    if (slot >= m_externalMidiMods.size())
    {
        return 0.0f;
    }
    return m_externalMidiMods[slot];
}

uint8_t FroggersV2ControlCore::rowsForPage(uint8_t page) const
{
    if (page == 0)
    {
        return 8;
    }
    if (page == 6)
    {
        return 7;
    }
    if (page < kNumHostPages)
    {
        return 10;
    }
    return 0;
}

uint8_t FroggersV2ControlCore::slotToRow(uint8_t page, uint8_t slot) const
{
    if (slot >= kEncoderCount || slot >= rowsForPage(page))
    {
        return kNoSelection;
    }
    return slot;
}

float FroggersV2ControlCore::blendedSceneCenter(const ParamState& state) const
{
    const float left = state.sceneCenter[m_sceneLeftOrdinal];
    const float right = state.sceneCenter[m_sceneRightOrdinal];
    return left * (1.0f - m_sceneBlend) + right * m_sceneBlend;
}

void FroggersV2ControlCore::seedSceneCentersFromDefaults()
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            const float center = HostParameterInventoryV2::pageKnobDefault(page, row);
            ParamState& param = m_params[page][row];
            for (uint8_t scene = 0; scene < kNumScenes; ++scene)
            {
                param.sceneCenter[scene] = center;
            }
        }
    }
    for (uint8_t scene = 0; scene < kNumScenes; ++scene)
    {
        m_crunchy.sceneCenter[scene] = 0.0f;
    }
}

void FroggersV2ControlCore::resetParameter(uint8_t page, uint8_t row)
{
    ParamState& param = m_params[page][row];
    const float center = HostParameterInventoryV2::pageKnobDefault(page, row);
    for (uint8_t scene = 0; scene < kNumScenes; ++scene)
    {
        param.sceneCenter[scene] = center;
    }
    for (uint8_t source = 0; source < kNumModSources; ++source)
    {
        param.modDepth[source] = HostParameterInventoryV2::modDepthDefault();
    }
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        param.gestureDepth[lane] = 0.0f;
    }
}

void FroggersV2ControlCore::rebuildVisibleSlots()
{
    for (uint8_t slot = 0; slot < kUiSlots; ++slot)
    {
        m_visibleSlots[slot] = {};
    }
    if (m_modView.open)
    {
        const ParamState& param = m_params[m_activePage][m_modView.targetRow];
        uint8_t count = 0;
        for (uint8_t mod = 0; mod < kNumModSources; ++mod)
        {
            if (param.modSource[mod] == kNoSelection)
            {
                continue;
            }
            m_visibleSlots[count].isTarget = false;
            m_visibleSlots[count].row = m_modView.targetRow;
            m_visibleSlots[count].modIndex = mod;
            ++count;
        }
        m_visibleSlots[count].isTarget = true;
        m_visibleSlots[count].row = m_modView.targetRow;
        m_visibleCount = static_cast<uint8_t>(count + 1);
        return;
    }
    const uint8_t rows = rowsForPage(m_activePage);
    m_visibleCount = rows;
    for (uint8_t slot = 0; slot < m_visibleCount; ++slot)
    {
        m_visibleSlots[slot].isTarget = false;
        m_visibleSlots[slot].row = slot;
        m_visibleSlots[slot].modIndex = 0;
    }
}

FroggersV2ControlCore::EffectiveRow FroggersV2ControlCore::computeEffective(const ParamState& state) const
{
    EffectiveRow out;
    out.sceneLeft = state.sceneCenter[m_sceneLeftOrdinal];
    out.sceneRight = state.sceneCenter[m_sceneRightOrdinal];
    const float center = blendedSceneCenter(state);
    const float modulationScale = std::min(center, 1.0f - center);

    float sum = 0.0f;
    float minSum = 0.0f;
    float maxSum = 0.0f;
    for (uint8_t i = 0; i < kNumModSources; ++i)
    {
        if (state.modSource[i] == kNoSelection)
        {
            continue;
        }
        const float depth = state.modDepth[i];
        const float bipolarSource = sourceValue(state.modSource[i]) * 2.0f - 1.0f;
        sum += bipolarSource * depth;
        minSum -= std::abs(depth);
        maxSum += std::abs(depth);
        out.modulatorsMask |= static_cast<uint16_t>(1u << i);
    }

    float gesture = 0.0f;
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        if (std::abs(state.gestureDepth[lane]) < 1.0e-6f || m_gestureWeights[lane] <= 0.0f)
        {
            continue;
        }
        out.gesturesMask |= static_cast<uint8_t>(1u << lane);
        gesture += state.gestureDepth[lane] * m_gestureWeights[lane];
    }

    out.effective = clamp01(center + (sum + gesture) * modulationScale);
    out.arcMin = clamp01(center + minSum * modulationScale);
    out.arcMax = clamp01(center + maxSum * modulationScale);
    return out;
}
} // namespace froggers_v2
