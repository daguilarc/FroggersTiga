#pragma once

#include "PermanentModTapRack.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

struct V2EnvelopeFollowerBank
{
    static constexpr size_t kNumTaps = 5;
    static constexpr uint8_t kFirstModIndex = 3;

    float m_levels[kNumTaps]{};
    float m_attackCoeff = 0.05f;
    float m_releaseCoeff = 0.01f;

    void setSampleRate(float sampleRate)
    {
        const float sr = sampleRate > 0.0f ? sampleRate : 44100.0f;
        const float attackSec = 0.01f;
        const float releaseSec = 0.05f;
        m_attackCoeff = 1.0f - std::exp(-1.0f / (attackSec * sr));
        m_releaseCoeff = 1.0f - std::exp(-1.0f / (releaseSec * sr));
    }

    void Process(float v1, float v2, float v3, PermanentModTapRack& taps)
    {
        const float targets[kNumTaps] = {
            std::fabs(v1),
            std::fabs(v2),
            std::fabs(v3),
            std::fabs(v1 + v2) * 0.5f,
            std::fabs(v2 + v3) * 0.5f,
        };

        for (size_t i = 0; i < kNumTaps; ++i)
        {
            const float target = std::min(std::max(targets[i], 0.0f), 1.0f);
            const float coeff = (target > m_levels[i]) ? m_attackCoeff : m_releaseCoeff;
            m_levels[i] += (target - m_levels[i]) * coeff;
            taps.SetTap(static_cast<uint8_t>(kFirstModIndex + i), m_levels[i]);
        }
    }
};

// packet 12 (openspec/changes/desktop-v2-sheaf-runtime-harmonization,
// tasks.md 12.1, design.md D13/D14): the "LFO EF" mod sources (permanent-rack
// taps 8-10, manifest stableIds lfo_1/2/3) are the same three VCO inputs as
// V2EnvelopeFollowerBank's fast per-VCO taps (3-5) above, run through a
// second, much slower attack/release pass so they move at LFO rate instead
// of audio-envelope rate. Deliberately a separate bank (not folded into
// V2EnvelopeFollowerBank) so the existing fast bank's fields, coefficients,
// and Process() body stay byte-for-byte unchanged -- this struct is
// additive only.
//
// Proposed time constants (attack 0.2s / release 1.0s) are an operator
// starting point for "LFO rate", not a measured spec -- flagged for operator
// tuning per the packet-12 report; not a hard design.md commitment.
struct V2SlowEnvelopeFollowerBank
{
    static constexpr size_t kNumTaps = 3;
    static constexpr uint8_t kFirstModIndex = 8;

    float m_levels[kNumTaps]{};
    float m_attackCoeff = 0.05f;
    float m_releaseCoeff = 0.01f;

    void setSampleRate(float sampleRate)
    {
        const float sr = sampleRate > 0.0f ? sampleRate : 44100.0f;
        const float attackSec = 0.2f;
        const float releaseSec = 1.0f;
        m_attackCoeff = 1.0f - std::exp(-1.0f / (attackSec * sr));
        m_releaseCoeff = 1.0f - std::exp(-1.0f / (releaseSec * sr));
    }

    void Process(float v1, float v2, float v3, PermanentModTapRack& taps)
    {
        const float targets[kNumTaps] = {
            std::fabs(v1),
            std::fabs(v2),
            std::fabs(v3),
        };

        for (size_t i = 0; i < kNumTaps; ++i)
        {
            const float target = std::min(std::max(targets[i], 0.0f), 1.0f);
            const float coeff = (target > m_levels[i]) ? m_attackCoeff : m_releaseCoeff;
            m_levels[i] += (target - m_levels[i]) * coeff;
            taps.SetTap(static_cast<uint8_t>(kFirstModIndex + i), m_levels[i]);
        }
    }
};
