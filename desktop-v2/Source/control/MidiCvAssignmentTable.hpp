#pragma once

#include "SequencerState.hpp"
#include "manifest/FroggersV2AppManifest.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

enum class MidiCvBindingRole : uint8_t
{
    None = 0,
    SceneOrdinal0,
    SceneOrdinal1,
    SceneOrdinal2
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
    MidiCvBindingRole target = MidiCvBindingRole::None;
};

struct MidiCvEncoderTurnBinding
{
    bool enabled = false;
    uint8_t channel = 0;
    uint8_t cc = 0;
};

struct MidiCvEncoderDrillInBinding
{
    bool enabled = false;
    MidiCvTriggerKind kind = MidiCvTriggerKind::Note;
    uint8_t channel = 0;
    uint8_t number = 60;
};

struct MidiCvPitchTarget
{
    uint8_t page = 0;
    uint8_t row = 0;
};

struct MidiCvControllerTargetIds
{
    const char* pitch;
    const char* gate;
    const char* externalModA;
    const char* externalModB;
    const char* scene1;
    const char* scene2;
    const char* scene3;
    const char* qwertyVirtual;
    const char* externalClock;
};

const MidiCvControllerTargetIds& midiCvControllerTargetIds();

const char* manifestTargetIdForBindingRole(MidiCvBindingRole role);

enum class ControllerConnectionState : uint8_t
{
    Disconnected = 0,
    Connected,
    Receiving,
    Error
};

enum class ControllerPersistenceState : uint8_t
{
    Saved = 0,
    Dirty,
    Error
};

struct ControllerMappingEvent
{
    MidiCvTriggerKind kind = MidiCvTriggerKind::Note;
    uint8_t channel = 0;
    uint8_t number = 0;
    uint8_t valueMin = 0;
    uint8_t valueMax = 127;
};

struct ControllerTargetMappingRow
{
    const char* targetId = nullptr;
    const char* displayName = nullptr;
    bool enabled = false;
    ControllerMappingEvent event{};
    std::string targetReadback;
    size_t fanOutCount = 1;
};

struct ControllerMappingRecord
{
    std::string targetId;
    bool enabled = false;
    ControllerMappingEvent event{};
};

struct MidiCvAssignmentTable
{
    using SequencerAdvanceHook = std::function<void()>;
    using HostPitchFn = std::function<void(uint8_t page, uint8_t row, float value)>;
    using HostGateFn = std::function<void(bool high)>;
    using UiSceneFn = std::function<void(uint8_t ordinal)>;
    using ParamTurnEmitFn = std::function<void(uint8_t page, uint8_t slot, float delta)>;
    using ModDrillInEmitFn = std::function<void(uint8_t page, uint8_t slot)>;

    uint8_t inputChannelFilter = 0;
    bool pitchEnabled = true;
    uint8_t pitchPage = 0;
    uint8_t pitchRow = 0;
    uint8_t pitchNoteMin = 36;
    uint8_t pitchNoteMax = 96;
    bool gateEnabled = true;
    MidiCvCcBinding externalModA{true, 0, 1};
    MidiCvCcBinding externalModB{false, 0, 2};
    std::array<MidiCvButtonBinding, 3> sceneButtons{};
    std::array<MidiCvEncoderTurnBinding, froggers_v2::manifest::kEncoderParamCount> encoderTurns{};
    std::array<MidiCvEncoderDrillInBinding, froggers_v2::manifest::kEncoderParamCount> encoderDrillIns{};
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

    void setUiSceneCallback(UiSceneFn callback)
    {
        m_uiScene = std::move(callback);
    }

    void setParamTurnEmitCallback(ParamTurnEmitFn callback)
    {
        m_paramTurnEmit = std::move(callback);
    }

    void setModDrillInEmitCallback(ModDrillInEmitFn callback)
    {
        m_modDrillInEmit = std::move(callback);
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

    bool consumeScenePending(uint8_t& ordinal);

    void setSelectedInputLabel(std::string label);
    const std::string& selectedInputLabel() const;
    ControllerConnectionState inputConnectionState() const;
    ControllerPersistenceState mappingPersistenceState() const;
    const std::string& mappingPersistenceMessage() const;
    void markMappingsDirty();
    void markMappingsSaved(const std::string& message);
    std::vector<ControllerTargetMappingRow> buildTargetMappingRows() const;
    size_t fanOutCountForEvent(const ControllerMappingEvent& event, const char* excludeTargetId) const;

    std::vector<ControllerMappingRecord> exportMappings() const;
    size_t importMappings(const std::vector<ControllerMappingRecord>& records,
                          std::vector<std::string>& rejectedTargetIds);
    bool applyMappingRecord(const ControllerMappingRecord& record, std::string& rejectReason);

private:
    bool channelMatches(uint8_t channel1Based, bool fromQwerty) const;
    float noteToNormalizedPitch(uint8_t note, uint8_t velocity) const;
    void handleNoteOn(uint8_t channel1Based, uint8_t note, uint8_t velocity, bool fromQwerty);
    void handleNoteOff(uint8_t channel1Based, uint8_t note, bool fromQwerty);
    void handleCc(uint8_t channel1Based, uint8_t cc, uint8_t value, bool fromQwerty);
    void handleButtonTrigger(MidiCvButtonBinding& binding, bool pressed);
    void handleEncoderDrillIn(size_t productRowIndex, bool pressed);
    void handleEncoderTurn(size_t productRowIndex, uint8_t value);
    void updatePitchFromHeldNotes();
    void updateGateFromHeldNotes();
    void emitPitch();
    void emitGate(bool high);
    void emitParamTurn(uint8_t page, uint8_t slot, float delta);
    void emitModDrillIn(uint8_t page, uint8_t slot);

    std::array<std::atomic<uint8_t>, 128> m_heldVelocities{};
    std::atomic<float> m_pitchValue{0.0f};
    std::atomic<float> m_externalModA{0.0f};
    std::atomic<float> m_externalModB{0.0f};
    std::atomic<bool> m_gateHigh{false};
    std::atomic<bool> m_scenePendingFlag{false};
    std::atomic<uint8_t> m_scenePendingOrdinal{0};

    SequencerAdvanceHook m_sequencerAdvanceHook;
    HostPitchFn m_hostPitch;
    HostGateFn m_hostGate;
    UiSceneFn m_uiScene;
    ParamTurnEmitFn m_paramTurnEmit;
    ModDrillInEmitFn m_modDrillInEmit;

    std::string m_selectedInputLabel{"Computer keyboard"};
    ControllerPersistenceState m_persistenceState = ControllerPersistenceState::Saved;
    std::string m_persistenceMessage{"Saved"};
    std::atomic<bool> m_receivingInput{false};
};
