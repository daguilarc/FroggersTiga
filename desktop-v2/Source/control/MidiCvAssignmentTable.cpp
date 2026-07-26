#include "control/MidiCvAssignmentTable.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace
{
constexpr uint8_t kStatusNoteOff = 0x80;
constexpr uint8_t kStatusNoteOn = 0x90;
constexpr uint8_t kStatusCc = 0xB0;
constexpr float kRelativeCcCenter = 64.0f;
constexpr float kRelativeCcDeltaScale = 0.25f;

size_t productRowLinearIndex(uint8_t page, uint8_t row)
{
    size_t cursor = 0;
    for (uint8_t p = 0; p < page; ++p)
    {
        cursor += HostParameterInventoryV2::rowsForUiPage(p);
    }
    return cursor + row;
}

bool decodeProductRowLinearIndex(size_t index, uint8_t& pageOut, uint8_t& rowOut)
{
    return HostParameterInventoryV2::decodePageRowIndex(index, pageOut, rowOut);
}

ControllerMappingEvent eventFromCcBinding(const MidiCvCcBinding& binding)
{
    ControllerMappingEvent event;
    event.kind = MidiCvTriggerKind::Cc;
    event.channel = binding.channel;
    event.number = binding.cc;
    return event;
}

ControllerMappingEvent eventFromButtonBinding(const MidiCvButtonBinding& binding)
{
    ControllerMappingEvent event;
    event.kind = binding.kind;
    event.channel = binding.channel;
    event.number = binding.number;
    return event;
}

ControllerMappingEvent eventFromEncoderTurn(const MidiCvEncoderTurnBinding& binding)
{
    ControllerMappingEvent event;
    event.kind = MidiCvTriggerKind::Cc;
    event.channel = binding.channel;
    event.number = binding.cc;
    return event;
}

ControllerMappingEvent eventFromEncoderDrillIn(const MidiCvEncoderDrillInBinding& binding)
{
    ControllerMappingEvent event;
    event.kind = binding.kind;
    event.channel = binding.channel;
    event.number = binding.number;
    return event;
}

bool eventsMatch(const ControllerMappingEvent& lhs, const ControllerMappingEvent& rhs)
{
    return lhs.kind == rhs.kind && lhs.channel == rhs.channel && lhs.number == rhs.number;
}

bool bindingChannelMatches(uint8_t bindingChannel, uint8_t channel1Based)
{
    return bindingChannel == 0 || bindingChannel == channel1Based;
}
} // namespace

const MidiCvControllerTargetIds& midiCvControllerTargetIds()
{
    static const MidiCvControllerTargetIds ids{
        froggers_v2::manifest::kBaseControllerTargetDeclarations[0].stableId,
        froggers_v2::manifest::kBaseControllerTargetDeclarations[1].stableId,
        froggers_v2::manifest::kBaseControllerTargetDeclarations[2].stableId,
        froggers_v2::manifest::kBaseControllerTargetDeclarations[3].stableId,
        froggers_v2::manifest::kBaseControllerTargetDeclarations[4].stableId,
        froggers_v2::manifest::kBaseControllerTargetDeclarations[5].stableId,
        froggers_v2::manifest::kBaseControllerTargetDeclarations[6].stableId,
        froggers_v2::manifest::kBaseControllerTargetDeclarations[7].stableId,
    };
    return ids;
}

const char* manifestTargetIdForBindingRole(MidiCvBindingRole role)
{
    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    switch (role)
    {
        case MidiCvBindingRole::SceneOrdinal0:
            return ids.scene1;
        case MidiCvBindingRole::SceneOrdinal1:
            return ids.scene2;
        case MidiCvBindingRole::SceneOrdinal2:
            return ids.scene3;
        case MidiCvBindingRole::None:
            break;
    }
    return nullptr;
}

bool MidiCvAssignmentTable::channelMatches(uint8_t channel1Based, bool fromQwerty) const
{
    if (fromQwerty)
    {
        if (!qwertyVirtualChannelEnabled)
        {
            return false;
        }
        return channel1Based == qwertyMidiChannel;
    }
    if (inputChannelFilter == 0)
    {
        return true;
    }
    return channel1Based == inputChannelFilter;
}

float MidiCvAssignmentTable::noteToNormalizedPitch(uint8_t note, uint8_t velocity) const
{
    if (note < pitchNoteMin || note > pitchNoteMax || velocity == 0)
    {
        return 0.0f;
    }
    const float span = static_cast<float>(pitchNoteMax - pitchNoteMin);
    if (span <= 0.0f)
    {
        return 0.0f;
    }
    const float step = static_cast<float>(note - pitchNoteMin + 1) / (span + 1.0f);
    const float velScale = static_cast<float>(velocity) / 127.0f;
    return std::min(std::max(step * velScale, 0.0f), 1.0f);
}

void MidiCvAssignmentTable::emitPitch()
{
    if (!pitchEnabled || !m_hostPitch)
    {
        return;
    }
    m_hostPitch(pitchPage, pitchRow, m_pitchValue.load(std::memory_order_relaxed));
}

void MidiCvAssignmentTable::emitGate(bool high)
{
    m_gateHigh.store(high, std::memory_order_relaxed);
    if (!gateEnabled || !m_hostGate)
    {
        return;
    }
    m_hostGate(high);
}

void MidiCvAssignmentTable::emitParamTurn(uint8_t page, uint8_t slot, float delta)
{
    if (m_paramTurnEmit)
    {
        m_paramTurnEmit(page, slot, delta);
    }
}

void MidiCvAssignmentTable::emitModDrillIn(uint8_t page, uint8_t slot)
{
    if (m_modDrillInEmit)
    {
        m_modDrillInEmit(page, slot);
    }
}

void MidiCvAssignmentTable::updatePitchFromHeldNotes()
{
    uint8_t highestNote = 0;
    uint8_t highestVelocity = 0;
    for (size_t note = 0; note < m_heldVelocities.size(); ++note)
    {
        const uint8_t velocity = m_heldVelocities[note].load(std::memory_order_relaxed);
        if (velocity == 0)
        {
            continue;
        }
        if (velocity >= highestVelocity)
        {
            highestVelocity = velocity;
            highestNote = static_cast<uint8_t>(note);
        }
    }
    const float pitch = noteToNormalizedPitch(highestNote, highestVelocity);
    m_pitchValue.store(pitch, std::memory_order_relaxed);
    emitPitch();
}

void MidiCvAssignmentTable::updateGateFromHeldNotes()
{
    bool anyHeld = false;
    for (const auto& velocity : m_heldVelocities)
    {
        if (velocity.load(std::memory_order_relaxed) != 0)
        {
            anyHeld = true;
            break;
        }
    }
    emitGate(anyHeld);
}

void MidiCvAssignmentTable::handleButtonTrigger(MidiCvButtonBinding& binding, bool pressed)
{
    if (!binding.enabled || binding.target == MidiCvBindingRole::None || !pressed)
    {
        return;
    }
    uint8_t ordinal = 0;
    switch (binding.target)
    {
        case MidiCvBindingRole::SceneOrdinal0:
            ordinal = 0;
            break;
        case MidiCvBindingRole::SceneOrdinal1:
            ordinal = 1;
            break;
        case MidiCvBindingRole::SceneOrdinal2:
            ordinal = 2;
            break;
        default:
            return;
    }
    m_scenePendingFlag.store(true, std::memory_order_relaxed);
    m_scenePendingOrdinal.store(ordinal, std::memory_order_relaxed);
    if (m_uiScene)
    {
        m_uiScene(ordinal);
    }
}

void MidiCvAssignmentTable::handleEncoderDrillIn(size_t productRowIndex, bool pressed)
{
    if (!pressed || productRowIndex >= encoderDrillIns.size())
    {
        return;
    }
    uint8_t page = 0;
    uint8_t row = 0;
    if (!decodeProductRowLinearIndex(productRowIndex, page, row))
    {
        return;
    }
    emitModDrillIn(page, row);
}

void MidiCvAssignmentTable::handleEncoderTurn(size_t productRowIndex, uint8_t value)
{
    if (productRowIndex >= encoderTurns.size())
    {
        return;
    }
    uint8_t page = 0;
    uint8_t row = 0;
    if (!decodeProductRowLinearIndex(productRowIndex, page, row))
    {
        return;
    }
    const float delta = (static_cast<float>(value) - kRelativeCcCenter) * kRelativeCcDeltaScale;
    if (delta == 0.0f)
    {
        return;
    }
    emitParamTurn(page, row, delta);
}

void MidiCvAssignmentTable::handleNoteOn(uint8_t channel1Based,
                                         uint8_t note,
                                         uint8_t velocity,
                                         bool fromQwerty)
{
    if (!channelMatches(channel1Based, fromQwerty) || note >= m_heldVelocities.size())
    {
        return;
    }
    if (velocity == 0)
    {
        handleNoteOff(channel1Based, note, fromQwerty);
        return;
    }

    for (MidiCvButtonBinding& binding : sceneButtons)
    {
        if (binding.enabled && binding.kind == MidiCvTriggerKind::Note && binding.number == note
            && bindingChannelMatches(binding.channel, channel1Based))
        {
            handleButtonTrigger(binding, true);
        }
    }

    for (size_t i = 0; i < encoderDrillIns.size(); ++i)
    {
        const MidiCvEncoderDrillInBinding& binding = encoderDrillIns[i];
        if (binding.enabled && binding.kind == MidiCvTriggerKind::Note && binding.number == note
            && bindingChannelMatches(binding.channel, channel1Based))
        {
            handleEncoderDrillIn(i, true);
        }
    }

    m_heldVelocities[note].store(velocity, std::memory_order_relaxed);
    if (pitchEnabled)
    {
        updatePitchFromHeldNotes();
    }
    if (gateEnabled)
    {
        updateGateFromHeldNotes();
    }
}

void MidiCvAssignmentTable::handleNoteOff(uint8_t channel1Based, uint8_t note, bool fromQwerty)
{
    if (!channelMatches(channel1Based, fromQwerty) || note >= m_heldVelocities.size())
    {
        return;
    }

    m_heldVelocities[note].store(0, std::memory_order_relaxed);
    if (pitchEnabled)
    {
        updatePitchFromHeldNotes();
    }
    if (gateEnabled)
    {
        updateGateFromHeldNotes();
    }
}

void MidiCvAssignmentTable::handleCc(uint8_t channel1Based,
                                     uint8_t cc,
                                     uint8_t value,
                                     bool fromQwerty)
{
    if (!channelMatches(channel1Based, fromQwerty))
    {
        return;
    }

    if (externalModA.enabled && cc == externalModA.cc
        && bindingChannelMatches(externalModA.channel, channel1Based))
    {
        m_externalModA.store(static_cast<float>(value) / 127.0f, std::memory_order_relaxed);
    }
    if (externalModB.enabled && cc == externalModB.cc
        && bindingChannelMatches(externalModB.channel, channel1Based))
    {
        m_externalModB.store(static_cast<float>(value) / 127.0f, std::memory_order_relaxed);
    }

    for (MidiCvButtonBinding& binding : sceneButtons)
    {
        if (binding.enabled && binding.kind == MidiCvTriggerKind::Cc && binding.number == cc
            && bindingChannelMatches(binding.channel, channel1Based))
        {
            handleButtonTrigger(binding, value >= 64);
        }
    }

    for (size_t i = 0; i < encoderTurns.size(); ++i)
    {
        const MidiCvEncoderTurnBinding& binding = encoderTurns[i];
        if (binding.enabled && binding.cc == cc && bindingChannelMatches(binding.channel, channel1Based))
        {
            handleEncoderTurn(i, value);
        }
    }

    for (size_t i = 0; i < encoderDrillIns.size(); ++i)
    {
        const MidiCvEncoderDrillInBinding& binding = encoderDrillIns[i];
        if (binding.enabled && binding.kind == MidiCvTriggerKind::Cc && binding.number == cc
            && bindingChannelMatches(binding.channel, channel1Based))
        {
            handleEncoderDrillIn(i, value >= 64);
        }
    }
}

void MidiCvAssignmentTable::processIncomingMessage(uint8_t channel1Based,
                                                   uint8_t status,
                                                   uint8_t data1,
                                                   uint8_t data2,
                                                   bool fromQwerty)
{
    m_receivingInput.store(true, std::memory_order_relaxed);
    const uint8_t statusType = static_cast<uint8_t>(status & 0xF0);
    if (statusType == kStatusNoteOn)
    {
        handleNoteOn(channel1Based, data1, data2, fromQwerty);
        return;
    }
    if (statusType == kStatusNoteOff)
    {
        handleNoteOff(channel1Based, data1, fromQwerty);
        return;
    }
    if (statusType == kStatusCc)
    {
        handleCc(channel1Based, data1, data2, fromQwerty);
    }
}

float MidiCvAssignmentTable::externalModLevel(uint8_t slot) const
{
    if (slot == 0)
    {
        return m_externalModA.load(std::memory_order_relaxed);
    }
    if (slot == 1)
    {
        return m_externalModB.load(std::memory_order_relaxed);
    }
    return 0.0f;
}

bool MidiCvAssignmentTable::gateHigh() const
{
    return m_gateHigh.load(std::memory_order_relaxed);
}

float MidiCvAssignmentTable::pitchValue() const
{
    return m_pitchValue.load(std::memory_order_relaxed);
}

MidiCvPitchTarget MidiCvAssignmentTable::pitchTarget() const
{
    return {pitchPage, pitchRow};
}

bool MidiCvAssignmentTable::consumeScenePending(uint8_t& ordinal)
{
    if (!m_scenePendingFlag.exchange(false, std::memory_order_acq_rel))
    {
        return false;
    }
    ordinal = m_scenePendingOrdinal.load(std::memory_order_relaxed);
    return true;
}

void MidiCvAssignmentTable::drainPendingUiActions()
{
    m_receivingInput.store(false, std::memory_order_relaxed);
    uint8_t ordinal = 0;
    if (consumeScenePending(ordinal) && m_uiScene)
    {
        m_uiScene(ordinal);
    }
}

void MidiCvAssignmentTable::setSelectedInputLabel(std::string label)
{
    m_selectedInputLabel = std::move(label);
}

const std::string& MidiCvAssignmentTable::selectedInputLabel() const
{
    return m_selectedInputLabel;
}

ControllerConnectionState MidiCvAssignmentTable::inputConnectionState() const
{
    if (m_selectedInputLabel.empty())
    {
        return ControllerConnectionState::Disconnected;
    }
    if (m_receivingInput.load(std::memory_order_relaxed))
    {
        return ControllerConnectionState::Receiving;
    }
    return ControllerConnectionState::Connected;
}

ControllerPersistenceState MidiCvAssignmentTable::mappingPersistenceState() const
{
    return m_persistenceState;
}

const std::string& MidiCvAssignmentTable::mappingPersistenceMessage() const
{
    return m_persistenceMessage;
}

void MidiCvAssignmentTable::markMappingsDirty()
{
    m_persistenceState = ControllerPersistenceState::Dirty;
    m_persistenceMessage = "Unsaved controller mappings";
}

void MidiCvAssignmentTable::markMappingsSaved(const std::string& message)
{
    m_persistenceState = ControllerPersistenceState::Saved;
    m_persistenceMessage = message;
}

size_t MidiCvAssignmentTable::fanOutCountForEvent(const ControllerMappingEvent& event,
                                                  const char* excludeTargetId) const
{
    size_t count = 0;
    const auto& ids = midiCvControllerTargetIds();
    const auto& targets = froggers_v2::manifest::controllerTargetDeclarations();

    auto countIfMatch = [&](const char* targetId, bool enabled, const ControllerMappingEvent& mapped) {
        if (!enabled)
        {
            return;
        }
        if (excludeTargetId != nullptr && targetId != nullptr
            && std::string(targetId) == std::string(excludeTargetId))
        {
            return;
        }
        if (eventsMatch(mapped, event))
        {
            ++count;
        }
    };

    countIfMatch(ids.externalModA, externalModA.enabled, eventFromCcBinding(externalModA));
    countIfMatch(ids.externalModB, externalModB.enabled, eventFromCcBinding(externalModB));
    countIfMatch(ids.scene1, sceneButtons[0].enabled, eventFromButtonBinding(sceneButtons[0]));
    countIfMatch(ids.scene2, sceneButtons[1].enabled, eventFromButtonBinding(sceneButtons[1]));
    countIfMatch(ids.scene3, sceneButtons[2].enabled, eventFromButtonBinding(sceneButtons[2]));

    for (const froggers_v2::manifest::ControllerTargetDeclaration& target : targets)
    {
        if (!froggers_v2::manifest::controllerTargetHasPageRow(target))
        {
            continue;
        }
        const size_t index = productRowLinearIndex(target.page, target.row);
        if (index >= froggers_v2::manifest::kEncoderParamCount)
        {
            continue;
        }
        if (froggers_v2::manifest::isEncoderTurnBindingRole(target.bindingRole))
        {
            countIfMatch(target.stableId, encoderTurns[index].enabled, eventFromEncoderTurn(encoderTurns[index]));
        }
        else if (froggers_v2::manifest::isEncoderModDrillInBindingRole(target.bindingRole))
        {
            countIfMatch(target.stableId,
                         encoderDrillIns[index].enabled,
                         eventFromEncoderDrillIn(encoderDrillIns[index]));
        }
    }
    return count;
}

std::vector<ControllerTargetMappingRow> MidiCvAssignmentTable::buildTargetMappingRows() const
{
    const auto& ids = midiCvControllerTargetIds();
    const auto& targets = froggers_v2::manifest::controllerTargetDeclarations();
    std::vector<ControllerTargetMappingRow> rows;
    rows.reserve(targets.size());

    auto pushRow = [&](const char* targetId,
                       const char* displayName,
                       bool enabled,
                       const ControllerMappingEvent& event,
                       const std::string& readback) {
        ControllerTargetMappingRow row;
        row.targetId = targetId;
        row.displayName = displayName;
        row.enabled = enabled;
        row.event = event;
        row.targetReadback = readback;
        row.fanOutCount = fanOutCountForEvent(event, targetId) + (enabled ? 1 : 0);
        if (row.fanOutCount == 0)
        {
            row.fanOutCount = enabled ? 1 : 0;
        }
        rows.push_back(row);
    };

    for (const froggers_v2::manifest::ControllerTargetDeclaration& target : targets)
    {
        if (std::strcmp(target.stableId, ids.pitch) == 0)
        {
            ControllerMappingEvent pitchEvent;
            pitchEvent.kind = MidiCvTriggerKind::Note;
            pitchEvent.channel = inputChannelFilter;
            pushRow(ids.pitch,
                    target.displayName,
                    pitchEnabled,
                    pitchEvent,
                    "Page " + std::to_string(static_cast<unsigned>(pitchPage + 1)) + " Row "
                        + std::to_string(static_cast<unsigned>(pitchRow + 1)));
            continue;
        }
        if (std::strcmp(target.stableId, ids.gate) == 0)
        {
            ControllerMappingEvent gateEvent;
            gateEvent.kind = MidiCvTriggerKind::Note;
            gateEvent.channel = inputChannelFilter;
            pushRow(ids.gate, target.displayName, gateEnabled, gateEvent, gateHigh() ? "On" : "Off");
            continue;
        }
        if (std::strcmp(target.stableId, ids.externalModA) == 0)
        {
            pushRow(ids.externalModA,
                    target.displayName,
                    externalModA.enabled,
                    eventFromCcBinding(externalModA),
                    std::to_string(static_cast<int>(externalModLevel(0) * 100.0f)) + "%");
            continue;
        }
        if (std::strcmp(target.stableId, ids.externalModB) == 0)
        {
            pushRow(ids.externalModB,
                    target.displayName,
                    externalModB.enabled,
                    eventFromCcBinding(externalModB),
                    std::to_string(static_cast<int>(externalModLevel(1) * 100.0f)) + "%");
            continue;
        }
        if (std::strcmp(target.stableId, ids.scene1) == 0)
        {
            pushRow(ids.scene1,
                    target.displayName,
                    sceneButtons[0].enabled,
                    eventFromButtonBinding(sceneButtons[0]),
                    sceneButtons[0].enabled ? "Mapped" : "Off");
            continue;
        }
        if (std::strcmp(target.stableId, ids.scene2) == 0)
        {
            pushRow(ids.scene2,
                    target.displayName,
                    sceneButtons[1].enabled,
                    eventFromButtonBinding(sceneButtons[1]),
                    sceneButtons[1].enabled ? "Mapped" : "Off");
            continue;
        }
        if (std::strcmp(target.stableId, ids.scene3) == 0)
        {
            pushRow(ids.scene3,
                    target.displayName,
                    sceneButtons[2].enabled,
                    eventFromButtonBinding(sceneButtons[2]),
                    sceneButtons[2].enabled ? "Mapped" : "Off");
            continue;
        }
        if (std::strcmp(target.stableId, ids.qwertyVirtual) == 0)
        {
            ControllerMappingEvent qwertyEvent;
            qwertyEvent.kind = MidiCvTriggerKind::Note;
            qwertyEvent.channel = qwertyMidiChannel;
            pushRow(ids.qwertyVirtual,
                    target.displayName,
                    qwertyVirtualChannelEnabled,
                    qwertyEvent,
                    qwertyVirtualChannelEnabled ? "Active" : "Off");
            continue;
        }
        if (!froggers_v2::manifest::controllerTargetHasPageRow(target))
        {
            continue;
        }
        const size_t index = productRowLinearIndex(target.page, target.row);
        if (index >= froggers_v2::manifest::kEncoderParamCount)
        {
            continue;
        }
        if (froggers_v2::manifest::isEncoderTurnBindingRole(target.bindingRole))
        {
            const MidiCvEncoderTurnBinding& binding = encoderTurns[index];
            pushRow(target.stableId,
                    target.displayName,
                    binding.enabled,
                    eventFromEncoderTurn(binding),
                    binding.enabled ? "Relative CC → ParamTurn" : "Off");
            continue;
        }
        if (froggers_v2::manifest::isEncoderModDrillInBindingRole(target.bindingRole))
        {
            const MidiCvEncoderDrillInBinding& binding = encoderDrillIns[index];
            pushRow(target.stableId,
                    target.displayName,
                    binding.enabled,
                    eventFromEncoderDrillIn(binding),
                    binding.enabled ? "Press → ModDrillIn" : "Off");
        }
    }

    return rows;
}

std::vector<ControllerMappingRecord> MidiCvAssignmentTable::exportMappings() const
{
    std::vector<ControllerMappingRecord> records;
    for (const ControllerTargetMappingRow& row : buildTargetMappingRows())
    {
        if (row.targetId == nullptr)
        {
            continue;
        }
        ControllerMappingRecord record;
        record.targetId = row.targetId;
        record.enabled = row.enabled;
        record.event = row.event;
        records.push_back(record);
    }
    return records;
}

bool MidiCvAssignmentTable::applyMappingRecord(const ControllerMappingRecord& record,
                                               std::string& rejectReason)
{
    froggers_v2::manifest::ControllerTargetDeclaration target{};
    if (!froggers_v2::manifest::findControllerTargetByStableId(record.targetId.c_str(), target))
    {
        rejectReason = "absent inventory target id";
        return false;
    }

    const auto& ids = midiCvControllerTargetIds();
    if (record.targetId == ids.pitch)
    {
        pitchEnabled = record.enabled;
        return true;
    }
    if (record.targetId == ids.gate)
    {
        gateEnabled = record.enabled;
        return true;
    }
    if (record.targetId == ids.externalModA)
    {
        externalModA.enabled = record.enabled;
        externalModA.channel = record.event.channel;
        externalModA.cc = record.event.number;
        return true;
    }
    if (record.targetId == ids.externalModB)
    {
        externalModB.enabled = record.enabled;
        externalModB.channel = record.event.channel;
        externalModB.cc = record.event.number;
        return true;
    }
    if (record.targetId == ids.scene1 || record.targetId == ids.scene2 || record.targetId == ids.scene3)
    {
        MidiCvButtonBinding* binding = nullptr;
        MidiCvBindingRole role = MidiCvBindingRole::None;
        if (record.targetId == ids.scene1)
        {
            binding = &sceneButtons[0];
            role = MidiCvBindingRole::SceneOrdinal0;
        }
        else if (record.targetId == ids.scene2)
        {
            binding = &sceneButtons[1];
            role = MidiCvBindingRole::SceneOrdinal1;
        }
        else
        {
            binding = &sceneButtons[2];
            role = MidiCvBindingRole::SceneOrdinal2;
        }
        binding->enabled = record.enabled;
        binding->kind = record.event.kind;
        binding->channel = record.event.channel;
        binding->number = record.event.number;
        binding->target = role;
        return true;
    }
    if (record.targetId == ids.qwertyVirtual)
    {
        qwertyVirtualChannelEnabled = record.enabled;
        if (record.event.channel >= 1 && record.event.channel <= 16)
        {
            qwertyMidiChannel = record.event.channel;
        }
        return true;
    }
    if (!froggers_v2::manifest::controllerTargetHasPageRow(target))
    {
        rejectReason = "unsupported target binding";
        return false;
    }
    const size_t index = productRowLinearIndex(target.page, target.row);
    if (index >= froggers_v2::manifest::kEncoderParamCount)
    {
        rejectReason = "encoder product-row index out of range";
        return false;
    }
    if (froggers_v2::manifest::isEncoderTurnBindingRole(target.bindingRole))
    {
        encoderTurns[index].enabled = record.enabled;
        encoderTurns[index].channel = record.event.channel;
        encoderTurns[index].cc = record.event.number;
        return true;
    }
    if (froggers_v2::manifest::isEncoderModDrillInBindingRole(target.bindingRole))
    {
        encoderDrillIns[index].enabled = record.enabled;
        encoderDrillIns[index].kind = record.event.kind;
        encoderDrillIns[index].channel = record.event.channel;
        encoderDrillIns[index].number = record.event.number;
        return true;
    }
    rejectReason = "unsupported encoder binding role";
    return false;
}

size_t MidiCvAssignmentTable::importMappings(const std::vector<ControllerMappingRecord>& records,
                                             std::vector<std::string>& rejectedTargetIds)
{
    size_t applied = 0;
    rejectedTargetIds.clear();
    for (const ControllerMappingRecord& record : records)
    {
        std::string rejectReason;
        if (applyMappingRecord(record, rejectReason))
        {
            ++applied;
            continue;
        }
        rejectedTargetIds.push_back(record.targetId.empty() ? std::string("<empty>") : record.targetId);
    }
    if (!rejectedTargetIds.empty())
    {
        m_persistenceState = ControllerPersistenceState::Error;
        m_persistenceMessage = "Rejected " + std::to_string(rejectedTargetIds.size())
                             + " mapping(s) with absent inventory IDs";
    }
    return applied;
}
