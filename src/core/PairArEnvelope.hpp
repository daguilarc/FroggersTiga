#pragma once

#include <algorithm>
#include <cmath>

struct PairArEnvelope
{
    float level = 0.0f;

    static float KnobToOnePoleCoeff(float knob, float sampleRate)
    {
        const float clampedKnob = std::min(std::max(knob, 0.0f), 1.0f);
        const float minSec = 0.001f;
        const float maxSec = 2.0f;
        const float sec = minSec * std::pow(maxSec / minSec, clampedKnob);
        const float tau = sec * sampleRate;
        return (tau > 1.0f) ? (1.0f / tau) : 1.0f;
    }

    float Step(float target, float attackKnob, float releaseKnob, float sampleRate)
    {
        const float clampedTarget = std::min(std::max(target, 0.0f), 1.0f);
        const float coeff = (clampedTarget > level)
                                ? KnobToOnePoleCoeff(attackKnob, sampleRate)
                                : KnobToOnePoleCoeff(releaseKnob, sampleRate);
        level += (clampedTarget - level) * coeff;
        return level;
    }

    void Reset()
    {
        level = 0.0f;
    }
};
