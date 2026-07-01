#pragma once

#include "SequencerState.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>

enum class MidiCvButtonTarget : uint8_t
{
    None = 0,
    Shift,
    Scene1,
    Scene2,
    Scene3
};

enum class MidiCvTriggerKind : uint8_t
{
    Note = 0,
    Cc
};

struct MidiCvCcBinding
{
    bool enabled = false;
    uint8_t channel = 0;
    uint8_t cc = 0;
};

struct MidiCvButtonBinding
{
    bool enabled = false;
    MidiCvTriggerKind kind = MidiCvTriggerKind::Note;
    uint8_t channel = 0;
    uint8_t number = 60;
    MidiCvButtonTarget target = MidiCvButtonTarget::None;
};

struct MidiCvPitchTarget
{
    uint8_t page = 0;
    uint8_t row = 0;
};

struct MidiCvAssignmentTable
{
    using SequencerAdvanceHook = std::function<void()>;
    using HostPitchFn = std::function<void(uint8_t page, uint8_t row, float value)>;
    using HostGateFn = std::function<void(bool high)>;
    using UiShiftFn = std::function<void(bool held)>;
    using UiSceneFn = std::function<void(uint8_t ordinal)>;

    uint8_t inputChannelFilter = 0;
    bool pitchEnabled = true;
    uint8_t pitchPage = 0;
    uint8_t pitchRow = 0;
    uint8_t pitchNoteMin = 36;
    uint8_t pitchNoteMax = 96;
    bool gateEnabled = true;
    MidiCvCcBinding externalModA{true, 0, 1};
    MidiCvCcBinding externalModB{false, 0, 2};
    MidiCvButtonBinding shiftButton{};
    std::array<MidiCvButtonBinding, 3> sceneButtons{};
    bool qwertyVirtualChannelEnabled = true;
    uint8_t qwertyMidiChannel = 1;

    void setSequencerAdvanceHook(SequencerAdvanceHook hook)
    {
        m_sequencerAdvanceHook = std::move(hook);
    }

    void setHostPitchCallback(HostPitchFn callback)
    {
        m_hostPitch = std::move(callback);
    }

    void setHostGateCallback(HostGateFn callback)
    {
        m_hostGate = std::move(callback);
    }

    void setUiShiftCallback(UiShiftFn callback)
    {
        m_uiShift = std::move(callback);
    }

    void setUiSceneCallback(UiSceneFn callback)
    {
        m_uiScene = std::move(callback);
    }

    void onMidiClockTick(SequencerState& sequencer)
    {
        if (!sequencer.m_playing || !sequencer.m_externalClock)
        {
            return;
        }
        sequencer.advanceOnExternalClock();
        if (m_sequencerAdvanceHook)
        {
            m_sequencerAdvanceHook();
        }
    }

    void processIncomingMessage(uint8_t channel1Based,
                                uint8_t status,
                                uint8_t data1,
                                uint8_t data2,
                                bool fromQwerty = false);

    void drainPendingUiActions();

    float externalModLevel(uint8_t slot) const;
    bool gateHigh() const;
    float pitchValue() const;
    MidiCvPitchTarget pitchTarget() const;

    bool consumeShiftPending(bool& held);
    bool consumeScenePending(uint8_t& ordinal);

private:
    bool channelMatches(uint8_t channel1Based, bool fromQwerty) const;
    float noteToNormalizedPitch(uint8_t note, uint8_t velocity) const;
    void handleNoteOn(uint8_t channel1Based, uint8_t note, uint8_t velocity, bool fromQwerty);
    void handleNoteOff(uint8_t channel1Based, uint8_t note, bool fromQwerty);
    void handleCc(uint8_t channel1Based, uint8_t cc, uint8_t value, bool fromQwerty);
    void handleButtonTrigger(MidiCvButtonBinding& binding, bool pressed);
    void updatePitchFromHeldNotes();
    void updateGateFromHeldNotes();
    void emitPitch();
    void emitGate(bool high);

    std::array<std::atomic<uint8_t>, 128> m_heldVelocities{};
    std::atomic<float> m_pitchValue{0.0f};
    std::atomic<float> m_externalModA{0.0f};
    std::atomic<float> m_externalModB{0.0f};
    std::atomic<bool> m_gateHigh{false};
    std::atomic<bool> m_shiftPendingFlag{false};
    std::atomic<bool> m_shiftPendingValue{false};
    std::atomic<bool> m_scenePendingFlag{false};
    std::atomic<uint8_t> m_scenePendingOrdinal{0};

    SequencerAdvanceHook m_sequencerAdvanceHook;
    HostPitchFn m_hostPitch;
    HostGateFn m_hostGate;
    UiShiftFn m_uiShift;
    UiSceneFn m_uiScene;
};
