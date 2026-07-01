#include "control/MidiCvAssignmentTable.hpp"

#include <algorithm>
#include <cmath>

namespace
{
constexpr uint8_t kStatusNoteOff = 0x80;
constexpr uint8_t kStatusNoteOn = 0x90;
constexpr uint8_t kStatusCc = 0xB0;
} // namespace

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
    if (!binding.enabled || binding.target == MidiCvButtonTarget::None)
    {
        return;
    }
    if (binding.target == MidiCvButtonTarget::Shift)
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
        case MidiCvButtonTarget::Scene1:
            ordinal = 0;
            break;
        case MidiCvButtonTarget::Scene2:
            ordinal = 1;
            break;
        case MidiCvButtonTarget::Scene3:
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
