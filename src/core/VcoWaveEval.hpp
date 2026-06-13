#pragma once

#include "SDDSine.hpp"

#include <cmath>

inline float EvalWaveMorph(float phaseWrapped01, float morph)
{
    if (!std::isfinite(morph))
    {
        morph = 0.0f;
    }
    const float sine = SDDSine::Evaluate(phaseWrapped01);
    const float saw = 2.0f * phaseWrapped01 - 1.0f;
    const float square = (phaseWrapped01 < 0.5f) ? 1.0f : -1.0f;
    if (morph <= 0.5f)
    {
        const float t = morph * 2.0f;
        return sine * (1.0f - t) + saw * t;
    }
    const float t = (morph - 0.5f) * 2.0f;
    return saw * (1.0f - t) + square * t;
}
