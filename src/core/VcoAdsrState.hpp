#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

struct VcoAdsrState
{
    static constexpr size_t kNumVoices = 3;
    static constexpr float kMinTimeSeconds = 0.0005f;
    static constexpr float kMaxAttackSeconds = 2.5f;
    static constexpr float kMaxReleaseSeconds = 10.0f;

    enum class Stage : uint8_t
    {
        Idle = 0,
        Attack,
        Hold,
        Release,
    };

    void init(float sampleRate)
    {
        setSampleRate(sampleRate);
        m_gateHigh = false;
        for (size_t i = 0; i < kNumVoices; ++i)
        {
            m_stage[i] = Stage::Idle;
            m_level[i] = 0.0f;
        }
    }

    void setSampleRate(float sampleRate)
    {
        m_sampleRate = sampleRate > 0.0f ? sampleRate : 44100.0f;
    }

    void setGate(bool high)
    {
        if (high == m_gateHigh)
        {
            return;
        }
        m_gateHigh = high;
        for (size_t i = 0; i < kNumVoices; ++i)
        {
            m_stage[i] = high ? Stage::Attack : Stage::Release;
        }
    }

    // Task 7.5 (D15): true per-voice ASR. sustainKnob is not a time -- it is
    // the target level (0..1, clamped) that Attack ramps toward and Hold
    // holds at, replacing the former hardcoded 1.0f ceiling.
    float apply(size_t voiceIndex, float input, float attackKnob, float sustainKnob, float releaseKnob)
    {
        if (voiceIndex >= kNumVoices)
        {
            return input;
        }
        stepVoice(voiceIndex, attackKnob, sustainKnob, releaseKnob);
        return input * m_level[voiceIndex];
    }

private:
    float mapAttack(float knob) const
    {
        const float clamped = std::min(std::max(knob, 0.0f), 1.0f);
        return kMinTimeSeconds + clamped * (kMaxAttackSeconds - kMinTimeSeconds);
    }

    float mapRelease(float knob) const
    {
        const float clamped = std::min(std::max(knob, 0.0f), 1.0f);
        return kMinTimeSeconds + clamped * (kMaxReleaseSeconds - kMinTimeSeconds);
    }

    void stepVoice(size_t voiceIndex, float attackKnob, float sustainKnob, float releaseKnob)
    {
        const float sustainLevel = std::min(std::max(sustainKnob, 0.0f), 1.0f);
        // Normalized attack (D15, operator-locked): the Attack-time knob sets
        // a DURATION, not a slope. A full 0->1 ramp at this attack knob would
        // take mapAttack(attackKnob) seconds; scaling the per-sample step by
        // sustainLevel makes reaching sustainLevel take that same duration
        // regardless of the sustain level, so the Attack-time knob reads the
        // same regardless of where Sustain is set.
        const float attackStep = sustainLevel / std::max(mapAttack(attackKnob) * m_sampleRate, 1.0f);
        const float releaseStep = 1.0f / std::max(mapRelease(releaseKnob) * m_sampleRate, 1.0f);

        switch (m_stage[voiceIndex])
        {
            case Stage::Idle:
                m_level[voiceIndex] = 0.0f;
                break;
            case Stage::Attack:
                m_level[voiceIndex] = std::min(sustainLevel, m_level[voiceIndex] + attackStep);
                if (m_level[voiceIndex] >= sustainLevel)
                {
                    m_stage[voiceIndex] = Stage::Hold;
                }
                break;
            case Stage::Hold:
                if (m_gateHigh)
                {
                    m_level[voiceIndex] = sustainLevel;
                }
                else
                {
                    m_stage[voiceIndex] = Stage::Release;
                }
                break;
            case Stage::Release:
                m_level[voiceIndex] = std::max(0.0f, m_level[voiceIndex] - releaseStep);
                if (m_level[voiceIndex] <= 0.0f)
                {
                    m_stage[voiceIndex] = Stage::Idle;
                }
                break;
        }
    }

    float m_sampleRate = 44100.0f;
    bool m_gateHigh = false;
    std::array<float, kNumVoices> m_level{};
    std::array<Stage, kNumVoices> m_stage{};
};
