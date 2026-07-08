#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

// Fixed 16-slot sequencer runtime. Each slot tracks written/unwritten state; locks apply on written active steps.

struct SequencerSlotPayload
{
    static constexpr uint8_t kNumHostPages = 7;
    static constexpr uint8_t kNumRows = 10;
    static constexpr uint8_t kNumScenes = 3;
    static constexpr uint8_t kNumGestures = 2;

    std::array<std::array<std::array<float, kNumScenes>, kNumRows>, kNumHostPages> sceneCenter{};
    std::array<std::array<uint8_t, kNumRows>, kNumHostPages> modSource{};
    std::array<std::array<float, kNumRows>, kNumHostPages> modDepth{};
    std::array<float, kNumScenes> crunchySceneCenter{};
    std::array<float, kNumGestures> gestureWeight{};
    bool gate = false;
};

struct SequencerSlotRuntime
{
    SequencerSlotPayload payload{};
    bool written = false;
    bool locked = false;
};

struct SequencerState
{
    static constexpr uint8_t kSlotCount = 16;
    static constexpr uint8_t kDirectionChoiceCount = 3;
    static constexpr uint8_t kSpeedChoiceCount = 5;
    static constexpr uint8_t kDefaultDirection = 1;
    static constexpr uint8_t kDefaultSpeed = 2;

    float m_bpm = 120.0f;
    uint8_t m_direction = kDefaultDirection;
    uint8_t m_speedChoice = kDefaultSpeed;
    uint8_t m_playhead = 0;
    uint8_t m_editStep = 0;
    bool m_playing = false;
    bool m_writeSeqArm = false;
    bool m_writeSeqJustStarted = false;
    bool m_externalClock = false;
    float m_phaseBeats = 0.0f;

    std::array<SequencerSlotRuntime, kSlotCount> m_slots{};

    void setBpm(float bpm)
    {
        m_bpm = std::min(std::max(bpm, 20.0f), 300.0f);
    }

    void setDirection(uint8_t direction)
    {
        m_direction = std::min(direction, static_cast<uint8_t>(kDirectionChoiceCount - 1u));
    }

    void setSpeedChoice(uint8_t speedChoice)
    {
        m_speedChoice = std::min(speedChoice, static_cast<uint8_t>(kSpeedChoiceCount - 1u));
    }

    void prevEditStep()
    {
        m_editStep = static_cast<uint8_t>((m_editStep + kSlotCount - 1u) % kSlotCount);
    }

    void nextEditStep()
    {
        m_editStep = static_cast<uint8_t>((m_editStep + 1u) % kSlotCount);
    }

    uint8_t wrappedEditStep(int deltaSteps) const
    {
        const int cur = static_cast<int>(m_editStep);
        const int next = ((cur + deltaSteps) % static_cast<int>(kSlotCount) + static_cast<int>(kSlotCount))
            % static_cast<int>(kSlotCount);
        return static_cast<uint8_t>(next);
    }

    bool slotWritten(uint8_t step) const
    {
        return step < kSlotCount && m_slots[step].written;
    }

    bool slotLocked(uint8_t step) const
    {
        return step < kSlotCount && m_slots[step].locked;
    }

    bool stepGate() const
    {
        return m_slots[m_playhead].payload.gate;
    }

    bool activeStepGate() const
    {
        return m_playing && slotWritten(m_playhead) && stepGate();
    }

    void captureStep(uint8_t step, const SequencerSlotPayload& snapshot)
    {
        if (step >= kSlotCount)
        {
            return;
        }
        m_slots[step].payload = snapshot;
        m_slots[step].written = true;
    }

    void clearStep(uint8_t step)
    {
        if (step >= kSlotCount)
        {
            return;
        }
        m_slots[step] = {};
    }

    const SequencerSlotPayload& currentStep() const
    {
        return m_slots[m_playhead].payload;
    }

    float speedMultiplier() const
    {
        switch (m_speedChoice)
        {
            case 0:
                return 0.5f;
            case 1:
                return 2.0f / 3.0f;
            case 2:
                return 1.0f;
            case 3:
                return 1.5f;
            case 4:
                return 2.0f;
            default:
                return 1.0f;
        }
    }

    void advancePlayhead()
    {
        if (m_direction == 0)
        {
            m_playhead = static_cast<uint8_t>((m_playhead + kSlotCount - 1u) % kSlotCount);
            return;
        }
        if (m_direction == 2)
        {
            m_playhead = static_cast<uint8_t>(std::rand() % kSlotCount);
            return;
        }
        m_playhead = static_cast<uint8_t>((m_playhead + 1u) % kSlotCount);
    }

    bool advanceOnSamples(size_t numSamples, float sampleRate)
    {
        if (!m_playing || m_externalClock || sampleRate <= 0.0f || m_bpm <= 0.0f)
        {
            return false;
        }
        const float beatsPerSample = (m_bpm * speedMultiplier() / 60.0f) / sampleRate;
        bool stepped = false;
        for (size_t i = 0; i < numSamples; ++i)
        {
            m_phaseBeats += beatsPerSample;
            if (m_phaseBeats >= 1.0f)
            {
                m_phaseBeats -= 1.0f;
                advancePlayhead();
                stepped = true;
            }
        }
        return stepped;
    }

    void advanceOnExternalClock()
    {
        if (!m_playing || !m_externalClock)
        {
            return;
        }
        advancePlayhead();
    }
};
