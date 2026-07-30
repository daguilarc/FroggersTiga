#pragma once

// synth_froggers::dsp::VcoEnvelopeFollowers -- packet 3 task 3.3 (DSP port).
// openspec/changes/froggers-sheaf-app/tasks.md section 3, item 3.3. A
// **copy** (design D3) of the cited Froggers formula.
//
// Ported from sim/V2EnvelopeFollowerBank.hpp:
//   - attack 0.01 s / release 0.05 s, coeff = 1 - exp(-1/(t*sr))  (:22-25)
//   - the five per-block targets                                  (:30-35)
//
// Only taps 3, 4, 5 (|v1|, |v2|, |v3| -- array slots 0-2 of the frozen
// bank's 5-wide kNumTaps) are ported: those are the ones that feed the D5
// modulation slate (slots 9-11, "VCO 1/2/3 envelope follower"). Taps 6-7 in
// the frozen bank (targets[3] = |v1+v2|*0.5, targets[4] = |v2+v3|*0.5, the
// two PAIR envelope followers) are superseded by design D5's slate and are
// deliberately NOT ported -- porting them would create modulation sources
// nothing in this app consumes.

#include <algorithm>
#include <cmath>

namespace synth_froggers::dsp {

struct VcoEnvelopeFollowers
{
    static constexpr int kNumTaps = 3;

    float levels[kNumTaps]{};
    float attackCoeff = 0.05f;
    float releaseCoeff = 0.01f;

    // sim/V2EnvelopeFollowerBank.hpp:19-26 (setSampleRate).
    void SetSampleRate(float sampleRate)
    {
        const float sr = sampleRate > 0.0f ? sampleRate : 44100.0f;
        constexpr float kAttackSeconds = 0.01f;
        constexpr float kReleaseSeconds = 0.05f;
        attackCoeff = 1.0f - std::exp(-1.0f / (kAttackSeconds * sr));
        releaseCoeff = 1.0f - std::exp(-1.0f / (kReleaseSeconds * sr));
    }

    // sim/V2EnvelopeFollowerBank.hpp:28-45 (Process), restricted to taps
    // 3-5 (|v1|, |v2|, |v3|); the pair-sum taps 6-7 are not computed at all.
    // Returns {|v1| follower, |v2| follower, |v3| follower}.
    void Process(float v1, float v2, float v3, float (&out)[kNumTaps])
    {
        const float targets[kNumTaps] = {std::fabs(v1), std::fabs(v2), std::fabs(v3)};
        for (int i = 0; i < kNumTaps; ++i)
        {
            const float target = std::min(std::max(targets[i], 0.0f), 1.0f);
            const float coeff = (target > levels[i]) ? attackCoeff : releaseCoeff;
            levels[i] += (target - levels[i]) * coeff;
            out[i] = levels[i];
        }
    }
};

// Packet 6 (tasks.md section 6, task 6.2; design D5 slot 14, "external audio
// envelope follower"). The external-audio source is a single channel (task
// 2.6's confirmed one-input-channel contract), so it needs exactly one of
// VcoEnvelopeFollowers's three identical per-tap formulas, not all three --
// this is that same formula (sim/V2EnvelopeFollowerBank.hpp:19-35),
// generalized to one channel instead of duplicating VcoEnvelopeFollowers's
// 3-wide array for a single tap or wastefully feeding one signal into all
// three of its lanes.
struct SingleEnvelopeFollower
{
    float level = 0.0f;
    float attackCoeff = 0.05f;
    float releaseCoeff = 0.01f;

    void SetSampleRate(float sampleRate)
    {
        const float sr = sampleRate > 0.0f ? sampleRate : 44100.0f;
        constexpr float kAttackSeconds = 0.01f;
        constexpr float kReleaseSeconds = 0.05f;
        attackCoeff = 1.0f - std::exp(-1.0f / (kAttackSeconds * sr));
        releaseCoeff = 1.0f - std::exp(-1.0f / (kReleaseSeconds * sr));
    }

    float Process(float v)
    {
        const float target = std::min(std::max(std::fabs(v), 0.0f), 1.0f);
        const float coeff = (target > level) ? attackCoeff : releaseCoeff;
        level += (target - level) * coeff;
        return level;
    }
};

}  // namespace synth_froggers::dsp
