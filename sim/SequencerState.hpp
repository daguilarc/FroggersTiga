#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

struct SequencerStepSnapshot
{
    static constexpr uint8_t kNumHostPages = 7;
    static constexpr uint8_t kNumRows = 10;
    static constexpr uint8_t kNumScenes = 3;
    static constexpr uint8_t kNumGestures = 2;

    std::array<std::array<std::array<float, kNumScenes>, kNumRows>, kNumHostPages> sceneCenter{};
    std::array<float, kNumScenes> crunchySceneCenter{};
    std::array<float, kNumGestures> gestureWeight{};
    bool gate = false;
    bool hasData = false;
};

struct SequencerState
{
    static constexpr uint8_t kMinLength = 4;
    static constexpr uint8_t kMaxLength = 64;
    static constexpr uint8_t kMaxSteps = 64;

    float m_bpm = 120.0f;
    uint8_t m_patternLength = 16;
    uint8_t m_playhead = 0;
    uint8_t m_editStep = 0;
    bool m_playing = false;
    bool m_recordArm = false;
    bool m_externalClock = false;
    float m_phaseBeats = 0.0f;

    std::array<SequencerStepSnapshot, kMaxSteps> m_steps{};

    void setBpm(float bpm)
    {
        m_bpm = std::min(std::max(bpm, 20.0f), 300.0f);
    }

    void setPatternLength(uint8_t length)
    {
        m_patternLength = std::clamp(length, kMinLength, kMaxLength);
        if (m_playhead >= m_patternLength)
        {
            m_playhead = 0;
        }
        if (m_editStep >= m_patternLength)
        {
            m_editStep = 0;
        }
    }

    void prevEditStep()
    {
        m_editStep = static_cast<uint8_t>((m_editStep + m_patternLength - 1u) % m_patternLength);
    }

    void nextEditStep()
    {
        m_editStep = static_cast<uint8_t>((m_editStep + 1u) % m_patternLength);
    }

    bool stepGate() const
    {
        return m_steps[m_playhead].gate;
    }

    bool activeStepGate() const
    {
        return m_playing && stepGate();
    }

    void captureStep(uint8_t step, const SequencerStepSnapshot& snapshot)
    {
        if (step < kMaxSteps)
        {
            m_steps[step] = snapshot;
        }
    }

    const SequencerStepSnapshot& currentStep() const
    {
        return m_steps[m_playhead];
    }

    bool advanceOnSamples(size_t numSamples, float sampleRate)
    {
        if (!m_playing || m_externalClock || sampleRate <= 0.0f || m_bpm <= 0.0f)
        {
            return false;
        }
        const float beatsPerSample = (m_bpm / 60.0f) / sampleRate;
        bool stepped = false;
        for (size_t i = 0; i < numSamples; ++i)
        {
            m_phaseBeats += beatsPerSample;
            if (m_phaseBeats >= 1.0f)
            {
                m_phaseBeats -= 1.0f;
                m_playhead = static_cast<uint8_t>((m_playhead + 1u) % m_patternLength);
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
        m_playhead = static_cast<uint8_t>((m_playhead + 1u) % m_patternLength);
    }
};
