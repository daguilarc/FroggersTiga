#include "VcoAdsrState.hpp"

#include <cmath>
#include <cstdio>

namespace
{
bool Near(float a, float b, float eps = 1e-5f)
{
    return std::fabs(a - b) <= eps;
}
} // namespace

int main()
{
    if (!(VcoAdsrState::kMaxReleaseSeconds >= 10.0f))
    {
        std::printf("FAIL: kMaxReleaseSeconds expected >= 10 got %f\n", VcoAdsrState::kMaxReleaseSeconds);
        return 1;
    }

    const float sampleRate = 48000.0f;
    VcoAdsrState adsr;
    adsr.init(sampleRate);

    adsr.setGate(true);
    float held = 0.0f;
    for (int i = 0; i < static_cast<int>(sampleRate); ++i)
    {
        held = adsr.apply(0, 1.0f, 0.0f, 1.0f, 0.5f);
    }

    if (!Near(held, 1.0f, 1e-3f))
    {
        std::printf("FAIL: gate high should hold at 1.0 got %f\n", held);
        return 1;
    }

    adsr.setGate(false);
    float fastRelease = held;
    for (int i = 0; i < 4800; ++i)
    {
        fastRelease = adsr.apply(0, 1.0f, 0.0f, 1.0f, 0.0f);
    }

    adsr.init(sampleRate);
    adsr.setGate(true);
    for (int i = 0; i < static_cast<int>(sampleRate); ++i)
    {
        adsr.apply(0, 1.0f, 0.0f, 1.0f, 1.0f);
    }
    adsr.setGate(false);
    float slowRelease = 1.0f;
    for (int i = 0; i < 4800; ++i)
    {
        slowRelease = adsr.apply(0, 1.0f, 0.0f, 1.0f, 1.0f);
    }

    if (!(fastRelease < slowRelease))
    {
        std::printf("FAIL: release min should fall faster than release max (fast=%f slow=%f)\n",
                    fastRelease,
                    slowRelease);
        return 1;
    }

    adsr.init(sampleRate);
    adsr.setGate(true);
    float fastAttack = 0.0f;
    for (int i = 0; i < 4800; ++i)
    {
        fastAttack = adsr.apply(1, 1.0f, 0.0f, 1.0f, 0.5f);
    }

    adsr.init(sampleRate);
    adsr.setGate(true);
    float slowAttack = 0.0f;
    for (int i = 0; i < 4800; ++i)
    {
        slowAttack = adsr.apply(1, 1.0f, 1.0f, 1.0f, 0.5f);
    }

    if (!(fastAttack > slowAttack))
    {
        std::printf("FAIL: attack min should rise faster than attack max (fast=%f slow=%f)\n",
                    fastAttack,
                    slowAttack);
        return 1;
    }

    adsr.init(sampleRate);
    const float v0 = adsr.apply(0, 0.5f, 0.5f, 1.0f, 0.5f);
    const float v1 = adsr.apply(1, 0.5f, 0.5f, 1.0f, 0.5f);
    if (!Near(v0, 0.0f) || !Near(v1, 0.0f))
    {
        std::printf("FAIL: idle voices should pass zero envelope level\n");
        return 1;
    }

    // Sustain: a gate held long enough to finish attack should converge to
    // the sustain knob's level, not to 1.0 (task 7.5 -- true ASR).
    adsr.init(sampleRate);
    adsr.setGate(true);
    float heldAtHalfSustain = 0.0f;
    for (int i = 0; i < static_cast<int>(sampleRate); ++i)
    {
        heldAtHalfSustain = adsr.apply(0, 1.0f, 0.0f, 0.5f, 0.5f);
    }
    if (!Near(heldAtHalfSustain, 0.5f, 1e-3f))
    {
        std::printf("FAIL: gate high with sustain=0.5 should hold at 0.5 got %f\n", heldAtHalfSustain);
        return 1;
    }

    // Normalized attack: the ATTACK DURATION (samples to reach the sustain
    // level) must be the same regardless of the sustain level, for the same
    // attack knob -- i.e. attack is level-invariant, not amplitude-invariant.
    {
        const float attackKnob = 0.35f;
        int stepsToSustainAtFull = -1;
        VcoAdsrState full;
        full.init(sampleRate);
        full.setGate(true);
        for (int i = 0; i < static_cast<int>(sampleRate); ++i)
        {
            const float level = full.apply(0, 1.0f, attackKnob, 1.0f, 0.2f);
            if (stepsToSustainAtFull < 0 && Near(level, 1.0f, 1e-6f))
            {
                stepsToSustainAtFull = i;
            }
        }

        int stepsToSustainAtHalf = -1;
        VcoAdsrState half;
        half.init(sampleRate);
        half.setGate(true);
        for (int i = 0; i < static_cast<int>(sampleRate); ++i)
        {
            const float level = half.apply(0, 1.0f, attackKnob, 0.5f, 0.2f);
            if (stepsToSustainAtHalf < 0 && Near(level, 0.5f, 1e-6f))
            {
                stepsToSustainAtHalf = i;
            }
        }

        if (stepsToSustainAtFull < 0 || stepsToSustainAtHalf < 0)
        {
            std::printf("FAIL: attack never reached its sustain level (full=%d half=%d)\n",
                        stepsToSustainAtFull,
                        stepsToSustainAtHalf);
            return 1;
        }
        if (std::abs(stepsToSustainAtFull - stepsToSustainAtHalf) > 1)
        {
            std::printf("FAIL: attack duration should be level-invariant (full=%d steps half=%d steps)\n",
                        stepsToSustainAtFull,
                        stepsToSustainAtHalf);
            return 1;
        }
    }

    // Release falls from a held sustain level (not from 1.0) back to 0.
    {
        VcoAdsrState env;
        env.init(sampleRate);
        env.setGate(true);
        float atHold = 0.0f;
        for (int i = 0; i < static_cast<int>(sampleRate); ++i)
        {
            atHold = env.apply(0, 1.0f, 0.0f, 0.6f, 0.0f);
        }
        if (!Near(atHold, 0.6f, 1e-3f))
        {
            std::printf("FAIL: expected hold at sustain 0.6 before release, got %f\n", atHold);
            return 1;
        }
        env.setGate(false);
        float afterOneStep = env.apply(0, 1.0f, 0.0f, 0.6f, 0.0f);
        if (!(afterOneStep < atHold))
        {
            std::printf("FAIL: release should fall starting from the held level (before=%f after=%f)\n",
                        atHold,
                        afterOneStep);
            return 1;
        }
        float released = afterOneStep;
        for (int i = 0; i < 4800; ++i)
        {
            released = env.apply(0, 1.0f, 0.0f, 0.6f, 0.0f);
        }
        if (!Near(released, 0.0f, 1e-3f))
        {
            std::printf("FAIL: release should reach 0 got %f\n", released);
            return 1;
        }
    }

    // Sustain = 0 degenerates cleanly: no NaN, envelope stays silent through
    // attack/hold and a subsequent release still lands at 0.
    {
        VcoAdsrState env;
        env.init(sampleRate);
        env.setGate(true);
        for (int i = 0; i < 4800; ++i)
        {
            const float level = env.apply(0, 1.0f, 0.5f, 0.0f, 0.5f);
            if (std::isnan(level) || !Near(level, 0.0f, 1e-4f))
            {
                std::printf("FAIL: sustain=0 should stay at 0 with no NaN, got %f at step %d\n", level, i);
                return 1;
            }
        }
        env.setGate(false);
        float releasedFromZero = 1.0f;
        for (int i = 0; i < 4800; ++i)
        {
            releasedFromZero = env.apply(0, 1.0f, 0.5f, 0.0f, 0.5f);
        }
        if (std::isnan(releasedFromZero) || !Near(releasedFromZero, 0.0f, 1e-4f))
        {
            std::printf("FAIL: sustain=0 release should stay at 0 with no NaN, got %f\n", releasedFromZero);
            return 1;
        }
    }

    std::printf("VcoAdsrState_test OK\n");
    return 0;
}
