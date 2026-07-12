#include "FroggersV2ControlCore.hpp"

#include "HostParameterInventoryV2.hpp"
#include "PermanentModTapRack.hpp"

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

uint8_t engineModFromInternal(uint8_t internal)
{
    if (internal == kNoSelection || internal >= kNumModSources)
    {
        return 255;
    }
    return internal;
}
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

uint8_t FroggersV2ControlCore::visibleModIndexForSlot(uint8_t slot) const
{
    if (slot >= m_visibleCount || m_visibleSlots[slot].isTarget)
    {
        return kNoSelection;
    }
    return m_visibleSlots[slot].modIndex;
}

bool FroggersV2ControlCore::visibleSlotIsTarget(uint8_t slot) const
{
    if (slot >= m_visibleCount)
    {
        return false;
    }
    return m_visibleSlots[slot].isTarget;
}

void FroggersV2ControlCore::setExternalAudioAvailable(bool available)
{
    m_externalAudioAvailable = available;
}

bool FroggersV2ControlCore::externalAudioAvailable() const
{
    return m_externalAudioAvailable;
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
        if (param.modDepth[i] != 0.0f)
        {
            return i;
        }
    }
    return kNoSelection;
}

void FroggersV2ControlCore::applyHostModRoute(uint8_t page,
                                              uint8_t row,
                                              uint8_t engineModIndex,
                                              float depth)
{
    if (page >= kNumHostPages || row >= rowsForPage(page) || engineModIndex >= kNumModSources)
    {
        return;
    }
    m_params[page][row].modDepth[engineModIndex] = clampSigned(depth);
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
        if (param.modDepth[i] != 0.0f)
        {
            return param.modDepth[i];
        }
    }
    return 0.0f;
}

float FroggersV2ControlCore::laneDepth(uint8_t page, uint8_t row, uint8_t lane) const
{
    if (page >= kNumHostPages || row >= rowsForPage(page) || lane >= kNumModSources)
    {
        return 0.0f;
    }
    return m_params[page][row].modDepth[lane];
}

void FroggersV2ControlCore::executeRandomization(RandomizationCommand command,
                                                  uint8_t sceneScope,
                                                  uint8_t stepScope)
{
    MessageIn message;
    switch (command)
    {
        case RandomizationCommand::RandAll:
            message.type = MessageIn::Type::RandAll;
            message.page = sceneScope;
            break;
        case RandomizationCommand::RandMods:
            message.type = MessageIn::Type::RandMods;
            message.page = stepScope;
            break;
    }
    m_bus.push(message);
    processBus();
}

namespace
{
bool messageMutatesPatchContent(MessageIn::Type type)
{
    switch (type)
    {
        case MessageIn::Type::ParamIncDec:
        case MessageIn::Type::ParamPress:
        case MessageIn::Type::ModSourceAssign:
        case MessageIn::Type::GestureWeight:
        case MessageIn::Type::RandAll:
        case MessageIn::Type::RandMods:
        case MessageIn::Type::RandPage:
        case MessageIn::Type::ResetSequencerStep:
        case MessageIn::Type::RandSequencerStep:
        case MessageIn::Type::RandSequencerMods:
            return true;
        case MessageIn::Type::ShiftHeld:
        case MessageIn::Type::SceneSelect:
        case MessageIn::Type::SceneBlend:
        case MessageIn::Type::GestureSelect:
        case MessageIn::Type::SelectPage:
        case MessageIn::Type::Clock:
            return false;
    }
    return false;
}
} // namespace

void FroggersV2ControlCore::processBus()
{
    MessageIn message;
    while (m_bus.pop(message))
    {
        if (messageMutatesPatchContent(message.type))
        {
            m_contentMutated.store(true, std::memory_order_release);
        }
        applyMessage(message);
    }
    compute();
    populateUiState();
}

bool FroggersV2ControlCore::consumeContentMutated()
{
    return m_contentMutated.exchange(false, std::memory_order_acq_rel);
}

void FroggersV2ControlCore::compute()
{
    // Computation is pulled by bridge/UI from the same state snapshot.
}

void FroggersV2ControlCore::storeSlotState(uint8_t slot, const EffectiveRow& row)
{
    m_uiState.sceneLeft[slot].store(row.sceneLeft, std::memory_order_release);
    m_uiState.sceneRight[slot].store(row.sceneRight, std::memory_order_release);
    m_uiState.effective[slot].store(row.effective, std::memory_order_release);
    m_uiState.arcMin[slot].store(row.arcMin, std::memory_order_release);
    m_uiState.arcMax[slot].store(row.arcMax, std::memory_order_release);
    m_uiState.modulatorsMask[slot].store(row.modulatorsMask, std::memory_order_release);
    m_uiState.gesturesMask[slot].store(row.gesturesMask, std::memory_order_release);
}

FroggersV2ControlCore::EffectiveRow
FroggersV2ControlCore::slotViewEffective(const VisibleSlot& visible) const
{
    const ParamState& param = m_params[m_activePage][visible.row];
    if (visible.isTarget)
    {
        // Dedicated target/Crispy cell: shows the row's base value only.
        EffectiveRow target;
        const float blended = blendedSceneCenter(param);
        target.sceneLeft = blended;
        target.sceneRight = blended;
        target.effective = blended;
        target.arcMin = 0.0f;
        target.arcMax = 1.0f;
        return target;
    }
    if (m_modView.open)
    {
        // Detail-grid lane cell: centre = no depth; the CV LED (modulator bit)
        // lights only for the row's active source, and its bipolar depth drives
        // the effective dot and the symmetric arc bounds.
        const bool selected = visible.modIndex < kNumModSources && param.modDepth[visible.modIndex] != 0.0f;
        const float depth = selected ? param.modDepth[visible.modIndex] : 0.0f;
        const float span = 0.5f * std::abs(depth);
        EffectiveRow lane;
        lane.sceneLeft = 0.5f;
        lane.sceneRight = 0.5f;
        lane.effective = clamp01(0.5f + 0.5f * depth);
        lane.arcMin = clamp01(0.5f - span);
        lane.arcMax = clamp01(0.5f + span);
        lane.modulatorsMask = selected ? static_cast<uint16_t>(1u) : static_cast<uint16_t>(0u);
        return lane;
    }
    return computeEffective(param);
}

void FroggersV2ControlCore::populateUiState()
{
    for (uint8_t slot = 0; slot < m_visibleCount; ++slot)
    {
        storeSlotState(slot, slotViewEffective(m_visibleSlots[slot]));
    }

    EffectiveRow empty;
    empty.arcMax = 0.0f;
    for (uint8_t slot = m_visibleCount; slot < kUiSlots; ++slot)
    {
        storeSlotState(slot, empty);
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

void FroggersV2ControlCore::applySequencerSlotPayload(const SequencerSlotPayload& snapshot)
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
            applyHostModRoute(
                page,
                row,
                engineModFromInternal(snapshot.modSource[page][row]),
                snapshot.modDepth[page][row]);
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

void FroggersV2ControlCore::captureSequencerSlotPayload(SequencerSlotPayload& out) const
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
            out.modSource[page][row] = assignedModSource(page, row);
            out.modDepth[page][row] = assignedModDepth(page, row);
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
    out.gate = true;
}

void FroggersV2ControlCore::captureFactorySlotPayload(SequencerSlotPayload& out) const
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
            out.modSource[page][row] = kNoSelection;
            out.modDepth[page][row] = 0.0f;
        }
    }
    out.crunchySceneCenter.fill(0.0f);
    out.gestureWeight.fill(0.0f);
    out.gate = false;
    out.gate = false;
}

void FroggersV2ControlCore::zeroStepGestures(SequencerSlotPayload& snapshot)
{
    snapshot.gestureWeight.fill(0.0f);
}

void FroggersV2ControlCore::randomizeFullSlotPayload(SequencerSlotPayload& out)
{
    randomizeSceneSlotsInto(out);
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        advanceRandState();
        out.gestureWeight[lane] = static_cast<float>(m_randState & 1023u) / 1023.0f;
    }
    advanceRandState();
    out.gate = (m_randState & 1u) != 0u;
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
            onRandAll(message.page);
            break;
        case MessageIn::Type::RandMods:
            onRandMods(message.page);
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
        case MessageIn::Type::RandSequencerMods:
            onRandSequencerMods(message.page);
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
    // In the 4x4 parameter-detail grid the slot indexes a detail cell (a mod
    // lane or the dedicated target/Crispy cell), NOT a page row, so this must
    // be handled before the row-based slotToRow guard below.
    if (m_modView.open && page == m_activePage)
    {
        if (slot >= m_visibleCount)
        {
            return;
        }
        const VisibleSlot visible = m_visibleSlots[slot];
        ParamState& detailParam = m_params[page][visible.row];
        if (visible.isTarget)
        {
            // Dedicated target/Crispy encoder edits the row's base value.
            const uint8_t targetScene = activeSceneOrdinal();
            detailParam.sceneCenter[targetScene] =
                clamp01(detailParam.sceneCenter[targetScene] + delta * kTurnStep);
            return;
        }
        // Packet 15.2a: every lane cell in the 4x4 detail grid is
        // independently editable -- lane identity is the cell index itself,
        // so turning any cell edits that lane's own depth with no prior
        // "selection" required.
        if (visible.modIndex < kNumModSources)
        {
            detailParam.modDepth[visible.modIndex] =
                clampSigned(detailParam.modDepth[visible.modIndex] + delta * kTurnStep);
        }
        return;
    }
    const uint8_t activeRow = slotToRow(page, slot);
    if (activeRow >= rowsForPage(page))
    {
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
        return;
    }
    if (page >= kNumHostPages)
    {
        return;
    }
    if (m_modView.open)
    {
        if (slot >= m_visibleCount)
        {
            return;
        }
        const VisibleSlot visible = m_visibleSlots[slot];
        if (visible.isTarget)
        {
            m_modView.open = false;
            m_modView.targetRow = kNoSelection;
            rebuildVisibleSlots();
            return;
        }
        // Packet 15.2a: pressing a lane cell in the 16-cell detail grid clears
        // that lane (turn sets a lane's depth, press clears it -- there is no
        // "selection" concept left once every lane is independently active).
        if (visible.modIndex < kNumModSources)
        {
            m_params[page][visible.row].modDepth[visible.modIndex] = 0.0f;
        }
        return;
    }
    const uint8_t row = slotToRow(page, slot);
    if (row >= rowsForPage(page))
    {
        return;
    }
    // Packet 15.2a: any row's detail grid is reachable on press, not only
    // rows that already carry a route -- the old hasAssignment gate encoded
    // the retired single-route model (a row's detail grid only opened once a
    // compact picker had assigned it a route) and is dead now that every
    // lane is independently turn-editable straight from the grid.
    m_modView.open = true;
    m_modView.targetRow = row;
    rebuildVisibleSlots();
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
    // Packet 15.2a: compact route pickers are single-select: choosing a
    // source silences every other lane on the row, leaving the chosen lane's
    // own depth untouched (0 if it was never turned); choosing "None"
    // (source == kNoSelection, or any other out-of-range value) silences
    // every lane -- no lane index ever equals an out-of-range source, so the
    // same loop handles both cases without a special case. A freshly-assigned
    // lane genuinely reads as "off" (depth 0) until turned -- there is no
    // separate identity marker in this model, matching the target contract
    // ("each lane depth defaults to 0/off"; assignment alone is not enough
    // to make a lane audible).
    ParamState& param = m_params[page][row];
    for (uint8_t i = 0; i < kNumModSources; ++i)
    {
        if (i != source)
        {
            param.modDepth[i] = 0.0f;
        }
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

void FroggersV2ControlCore::randomizeSceneSlotValues(ParamState& param, uint8_t sceneScope)
{
    if (sceneScope == kRandSceneScopeCurrent)
    {
        // Current Scene scope only touches the active scene's edit-target
        // endpoint value, leaving the other two scene slots untouched.
        const uint8_t scene = activeSceneOrdinal();
        advanceRandState();
        param.sceneCenter[scene] = static_cast<float>(m_randState & 1023u) / 1023.0f;
        return;
    }
    randomizeSceneSlotValues(param.sceneCenter);
}

void FroggersV2ControlCore::randomizeSceneSlotsInto(uint8_t page, uint8_t sceneScope)
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
        randomizeSceneSlotValues(m_params[page][row], sceneScope);
    }
}

void FroggersV2ControlCore::randomizeSceneSlotsInto(uint8_t page)
{
    randomizeSceneSlotsInto(page, kRandSceneScopeAll);
}

void FroggersV2ControlCore::randomizeLiveModDepths(uint8_t page)
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
        ParamState& param = m_params[page][row];
        for (uint8_t source = 0; source < kNumModSources; ++source)
        {
            advanceRandState();
            const float candidate = clampSigned(
                static_cast<float>(static_cast<int32_t>(m_randState & 1023u) - 512) / 512.0f);
            const bool eligible = manifest::isModSourceEligibleForRow(
                page, row, source, m_externalAudioAvailable);
            param.modDepth[source] = eligible ? candidate : 0.0f;
        }
    }
}

void FroggersV2ControlCore::randomizeLiveModDepths()
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        randomizeLiveModDepths(page);
    }
}

void FroggersV2ControlCore::captureModRoutesIntoSnapshot(SequencerSlotPayload& snapshot) const
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            snapshot.modSource[page][row] = assignedModSource(page, row);
            snapshot.modDepth[page][row] = assignedModDepth(page, row);
        }
    }
}

void FroggersV2ControlCore::randomizeSceneSlotsInto(SequencerSlotPayload& snapshot)
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
    if (!m_sequencer || step >= manifest::kSequencerSlotCount)
    {
        return;
    }
    captureFactorySlotPayload(m_sequencer->m_slots[step].payload);
    m_sequencer->m_slots[step].written = true;
}

void FroggersV2ControlCore::onRandSequencerStep(uint8_t step, uint8_t scope)
{
    if (!m_sequencer)
    {
        return;
    }
    if (scope == kRandSeqScopeFullStep)
    {
        if (step >= manifest::kSequencerSlotCount)
        {
            return;
        }
        randomizeFullSlotPayload(m_sequencer->m_slots[step].payload);
        m_sequencer->m_slots[step].written = true;
        return;
    }

    randomizeSceneEndpointsAndBlend();

    if (scope == kRandSeqScopeStep)
    {
        const uint8_t targetStep =
            m_sequencer->m_playing ? m_sequencer->m_playhead : m_sequencer->m_editStep;
        if (targetStep >= manifest::kSequencerSlotCount)
        {
            return;
        }
        randomizeSceneSlotsInto(m_sequencer->m_slots[targetStep].payload);
        zeroStepGestures(m_sequencer->m_slots[targetStep].payload);
        m_sequencer->m_slots[targetStep].written = true;
        return;
    }

    if (scope != kRandSeqScopePattern)
    {
        return;
    }
    // desktop-v2-sequencer-operator-loop "All Steps randomization writes all
    // written steps": Pattern/All-Steps scope SHALL only affect steps that
    // are already written, not silently write previously-blank slots.
    for (uint8_t i = 0; i < manifest::kSequencerSlotCount; ++i)
    {
        if (!m_sequencer->m_slots[i].written)
        {
            continue;
        }
        randomizeSceneSlotsInto(m_sequencer->m_slots[i].payload);
        zeroStepGestures(m_sequencer->m_slots[i].payload);
    }
}

void FroggersV2ControlCore::randomizeModIntoSnapshot(SequencerSlotPayload& snapshot)
{
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            if (row == crispyRowForPage(page))
            {
                snapshot.modSource[page][row] = kNoSelection;
                snapshot.modDepth[page][row] = 0.0f;
                continue;
            }
            advanceRandState();
            const uint8_t source = static_cast<uint8_t>(m_randState % kNumModSources);
            snapshot.modSource[page][row] = source;
            advanceRandState();
            snapshot.modDepth[page][row] = clampSigned(
                static_cast<float>(static_cast<int32_t>(m_randState & 1023u) - 512) / 512.0f);
        }
    }
}

void FroggersV2ControlCore::onRandSequencerMods(uint8_t scope)
{
    if (!m_sequencer)
    {
        return;
    }

    if (scope == kRandSeqScopeStep)
    {
        const uint8_t targetStep =
            m_sequencer->m_playing ? m_sequencer->m_playhead : m_sequencer->m_editStep;
        if (targetStep >= manifest::kSequencerSlotCount)
        {
            return;
        }
        randomizeModIntoSnapshot(m_sequencer->m_slots[targetStep].payload);
        m_sequencer->m_slots[targetStep].written = true;
        if (m_sequencer->m_playing && targetStep == m_sequencer->m_playhead
            && m_sequencer->slotWritten(targetStep)
            && !(m_sequencer->slotLocked(targetStep)))
        {
            applySequencerSlotPayload(m_sequencer->m_slots[targetStep].payload);
        }
        return;
    }

    if (scope != kRandSeqScopePattern)
    {
        return;
    }
    for (uint8_t i = 0; i < manifest::kSequencerSlotCount; ++i)
    {
        randomizeModIntoSnapshot(m_sequencer->m_slots[i].payload);
        m_sequencer->m_slots[i].written = true;
        if (m_sequencer->m_playing && i == m_sequencer->m_playhead && m_sequencer->slotWritten(i)
            && !m_sequencer->slotLocked(i))
        {
            applySequencerSlotPayload(m_sequencer->m_slots[i].payload);
        }
    }
}

void FroggersV2ControlCore::onRandAll(uint8_t sceneScope)
{
    m_activeGestureLane = kNoSelection;
    for (uint8_t lane = 0; lane < kNumGestures; ++lane)
    {
        m_gestureWeights[lane] = 0.0f;
    }
    for (uint8_t page = 0; page < kNumHostPages; ++page)
    {
        randomizeSceneSlotsInto(page, sceneScope);
        // Mod portion of Rand All goes through the same live-mod-depth
        // authority Rand Mods uses (4.4) instead of a second inline copy.
        randomizeLiveModDepths(page);
        const uint8_t rowLimit = rowsForPage(page);
        for (uint8_t row = 0; row < rowLimit; ++row)
        {
            if (row == crispyRowForPage(page))
            {
                continue;
            }
            ParamState& param = m_params[page][row];
            for (uint8_t lane = 0; lane < kNumGestures; ++lane)
            {
                param.gestureDepth[lane] = 0.0f;
            }
        }
    }
    randomizeSceneSlotValues(m_crunchy, sceneScope);
    if (sceneScope == kRandSceneScopeAll)
    {
        // Current Scene scope must leave the left/right scene ordinal
        // selections and blend position untouched.
        randomizeSceneEndpointsAndBlend();
    }
}

void FroggersV2ControlCore::onRandMods(uint8_t stepScope)
{
    // Global Rand Mods is not sequencer-only: it always randomizes the live
    // mod depths on the control core regardless of sequencer play state or
    // which carousel page is visible.
    randomizeLiveModDepths();

    if (!m_sequencer)
    {
        return;
    }

    if (stepScope == kRandSeqScopeStep)
    {
        const uint8_t targetStep =
            m_sequencer->m_playing ? m_sequencer->m_playhead : m_sequencer->m_editStep;
        if (targetStep < manifest::kSequencerSlotCount && m_sequencer->m_slots[targetStep].written)
        {
            // Snapshot changes only for written steps in scope; the live
            // state (already randomized above) is what gets captured.
            captureModRoutesIntoSnapshot(m_sequencer->m_slots[targetStep].payload);
        }
        return;
    }

    if (stepScope != kRandSeqScopePattern)
    {
        return;
    }
    for (uint8_t i = 0; i < manifest::kSequencerSlotCount; ++i)
    {
        if (m_sequencer->m_slots[i].written)
        {
            captureModRoutesIntoSnapshot(m_sequencer->m_slots[i].payload);
        }
    }
}

float FroggersV2ControlCore::sourceValue(uint8_t source) const
{
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
        // The parameter-detail grid is a fixed 16-cell layout (kUiSlots): every
        // permanent modulation lane in manifest order, plus one dedicated
        // target/Crispy cell for the row's own base value. Eligibility/
        // availability for each lane is a UI concern (manifest projection),
        // not something the control core filters out of the grid.
        for (uint8_t lane = 0; lane < kNumModSources; ++lane)
        {
            m_visibleSlots[lane].isTarget = false;
            m_visibleSlots[lane].row = m_modView.targetRow;
            m_visibleSlots[lane].modIndex = lane;
        }
        m_visibleSlots[kNumModSources].isTarget = true;
        m_visibleSlots[kNumModSources].row = m_modView.targetRow;
        m_visibleCount = kUiSlots;
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
        if (state.modDepth[i] == 0.0f)
        {
            continue;
        }
        const float depth = state.modDepth[i];
        const float bipolarSource = sourceValue(i) * 2.0f - 1.0f;
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
