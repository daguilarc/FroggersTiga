#include "control/MidiCvAssignmentTable.hpp"

#include "manifest/FroggersV2AppManifest.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
constexpr uint8_t kStatusNoteOff = 0x80;
constexpr uint8_t kStatusNoteOn = 0x90;
constexpr uint8_t kStatusCc = 0xB0;
} // namespace

const MidiCvControllerTargetIds& midiCvControllerTargetIds()
{
    static const MidiCvControllerTargetIds ids{
        froggers_v2::manifest::kControllerTargetDeclarations[0].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[1].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[2].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[3].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[4].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[5].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[6].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[7].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[8].stableId,
        froggers_v2::manifest::kControllerTargetDeclarations[9].stableId,
    };
    return ids;
}

const char* manifestTargetIdForBindingRole(MidiCvBindingRole role)
{
    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    switch (role)
    {
        case MidiCvBindingRole::HeldModifier:
            return ids.shiftButton;
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
    if (!binding.enabled || binding.target == MidiCvBindingRole::None)
    {
        return;
    }
    if (binding.target == MidiCvBindingRole::HeldModifier)
    {
        m_shiftPendingFlag.store(true, std::memory_order_relaxed);
        m_shiftPendingValue.store(pressed, std::memory_order_relaxed);
        if (m_uiShift)
        {
            m_uiShift(pressed);
        }
        return;
    }
    if (!pressed)
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

    if (shiftButton.enabled && shiftButton.kind == MidiCvTriggerKind::Note
        && shiftButton.number == note
        && (shiftButton.channel == 0 || shiftButton.channel == channel1Based))
    {
        handleButtonTrigger(shiftButton, true);
    }
    for (MidiCvButtonBinding& binding : sceneButtons)
    {
        if (binding.enabled && binding.kind == MidiCvTriggerKind::Note && binding.number == note
            && (binding.channel == 0 || binding.channel == channel1Based))
        {
            handleButtonTrigger(binding, true);
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

    if (shiftButton.enabled && shiftButton.kind == MidiCvTriggerKind::Note
        && shiftButton.number == note
        && (shiftButton.channel == 0 || shiftButton.channel == channel1Based))
    {
        handleButtonTrigger(shiftButton, false);
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
        && (externalModA.channel == 0 || externalModA.channel == channel1Based))
    {
        m_externalModA.store(static_cast<float>(value) / 127.0f, std::memory_order_relaxed);
    }
    if (externalModB.enabled && cc == externalModB.cc
        && (externalModB.channel == 0 || externalModB.channel == channel1Based))
    {
        m_externalModB.store(static_cast<float>(value) / 127.0f, std::memory_order_relaxed);
    }

    if (shiftButton.enabled && shiftButton.kind == MidiCvTriggerKind::Cc
        && shiftButton.number == cc
        && (shiftButton.channel == 0 || shiftButton.channel == channel1Based))
    {
        handleButtonTrigger(shiftButton, value >= 64);
    }
    for (MidiCvButtonBinding& binding : sceneButtons)
    {
        if (binding.enabled && binding.kind == MidiCvTriggerKind::Cc && binding.number == cc
            && (binding.channel == 0 || binding.channel == channel1Based))
        {
            handleButtonTrigger(binding, value >= 64);
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

bool MidiCvAssignmentTable::consumeShiftPending(bool& held)
{
    if (!m_shiftPendingFlag.exchange(false, std::memory_order_acq_rel))
    {
        return false;
    }
    held = m_shiftPendingValue.load(std::memory_order_relaxed);
    return true;
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
    bool held = false;
    if (consumeShiftPending(held) && m_uiShift)
    {
        m_uiShift(held);
    }
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

namespace
{
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

bool eventsMatch(const ControllerMappingEvent& lhs, const ControllerMappingEvent& rhs)
{
    return lhs.kind == rhs.kind && lhs.channel == rhs.channel && lhs.number == rhs.number;
}
} // namespace

size_t MidiCvAssignmentTable::fanOutCountForEvent(const ControllerMappingEvent& event,
                                                  const char* excludeTargetId) const
{
    size_t count = 0;
    const auto& ids = midiCvControllerTargetIds();
    const std::array<std::pair<const char*, ControllerMappingEvent>, 6> rows{{
        {ids.externalModA, eventFromCcBinding(externalModA)},
        {ids.externalModB, eventFromCcBinding(externalModB)},
        {ids.shiftButton, eventFromButtonBinding(shiftButton)},
        {ids.scene1, eventFromButtonBinding(sceneButtons[0])},
        {ids.scene2, eventFromButtonBinding(sceneButtons[1])},
        {ids.scene3, eventFromButtonBinding(sceneButtons[2])},
    }};
    for (const auto& row : rows)
    {
        if (excludeTargetId != nullptr && row.first != nullptr
            && std::string(row.first) == std::string(excludeTargetId))
        {
            continue;
        }
        if (eventsMatch(row.second, event))
        {
            ++count;
        }
    }
    return count;
}

std::vector<ControllerTargetMappingRow> MidiCvAssignmentTable::buildTargetMappingRows() const
{
    const auto& ids = midiCvControllerTargetIds();
    const auto& targets = froggers_v2::manifest::kControllerTargetDeclarations;
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

    ControllerMappingEvent pitchEvent;
    pitchEvent.kind = MidiCvTriggerKind::Note;
    pitchEvent.channel = inputChannelFilter;
    pushRow(ids.pitch, targets[0].displayName, pitchEnabled, pitchEvent,
            "Page " + std::to_string(static_cast<unsigned>(pitchPage + 1)) + " Row "
                + std::to_string(static_cast<unsigned>(pitchRow + 1)));

    ControllerMappingEvent gateEvent;
    gateEvent.kind = MidiCvTriggerKind::Note;
    gateEvent.channel = inputChannelFilter;
    pushRow(ids.gate, targets[1].displayName, gateEnabled, gateEvent,
            gateHigh() ? "On" : "Off");

    pushRow(ids.externalModA, targets[2].displayName, externalModA.enabled,
            eventFromCcBinding(externalModA),
            std::to_string(static_cast<int>(externalModLevel(0) * 100.0f)) + "%");
    pushRow(ids.externalModB, targets[3].displayName, externalModB.enabled,
            eventFromCcBinding(externalModB),
            std::to_string(static_cast<int>(externalModLevel(1) * 100.0f)) + "%");
    pushRow(ids.shiftButton, targets[4].displayName, shiftButton.enabled,
            eventFromButtonBinding(shiftButton),
            shiftButton.enabled ? "Mapped" : "Off");
    pushRow(ids.scene1, targets[5].displayName, sceneButtons[0].enabled,
            eventFromButtonBinding(sceneButtons[0]), sceneButtons[0].enabled ? "Mapped" : "Off");
    pushRow(ids.scene2, targets[6].displayName, sceneButtons[1].enabled,
            eventFromButtonBinding(sceneButtons[1]), sceneButtons[1].enabled ? "Mapped" : "Off");
    pushRow(ids.scene3, targets[7].displayName, sceneButtons[2].enabled,
            eventFromButtonBinding(sceneButtons[2]), sceneButtons[2].enabled ? "Mapped" : "Off");

    ControllerMappingEvent qwertyEvent;
    qwertyEvent.kind = MidiCvTriggerKind::Note;
    qwertyEvent.channel = qwertyMidiChannel;
    pushRow(ids.qwertyVirtual, targets[8].displayName, qwertyVirtualChannelEnabled, qwertyEvent,
            qwertyVirtualChannelEnabled ? "Active" : "Off");

    ControllerMappingEvent clockEvent;
    clockEvent.kind = MidiCvTriggerKind::Cc;
    clockEvent.number = 0xF8;
    pushRow(ids.externalClock, targets[9].displayName, true, clockEvent, "Sequencer sync");

    return rows;
}
