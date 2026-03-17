#pragma once

#include <cmath>

struct SDDSine
{
    static constexpr float x_twoPi = 6.28318530717958647692f;

    static float Evaluate(float phase)
    {
        float wrappedPhase = phase - std::floor(phase);
        return std::sin(x_twoPi * wrappedPhase);
    }
};
