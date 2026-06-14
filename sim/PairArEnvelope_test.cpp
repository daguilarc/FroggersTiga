#include "PairArEnvelope.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    PairArEnvelope env;
    const float sampleRate = 48000.0f;

    float fastRise = 0.0f;
    for (int i = 0; i < 4800; i++)
    {
        fastRise = env.Step(1.0f, 0.0f, 1.0f, sampleRate);
    }

    env.Reset();
    float slowRise = 0.0f;
    for (int i = 0; i < 4800; i++)
    {
        slowRise = env.Step(1.0f, 1.0f, 1.0f, sampleRate);
    }

    if (!(fastRise > slowRise))
    {
        std::printf("FAIL: attack min should rise faster than attack max (fast=%f slow=%f)\n",
                    fastRise,
                    slowRise);
        return 1;
    }

    for (int i = 0; i < 48000; i++)
    {
        env.Step(1.0f, 0.0f, 1.0f, sampleRate);
    }

    float fastFall = env.level;
    for (int i = 0; i < 4800; i++)
    {
        fastFall = env.Step(0.0f, 0.0f, 0.0f, sampleRate);
    }

    env.Reset();
    for (int i = 0; i < 48000; i++)
    {
        env.Step(1.0f, 0.0f, 1.0f, sampleRate);
    }

    float slowFall = env.level;
    for (int i = 0; i < 4800; i++)
    {
        slowFall = env.Step(0.0f, 0.0f, 1.0f, sampleRate);
    }

    if (!(fastFall < slowFall))
    {
        std::printf("FAIL: release min should fall faster than release max (fast=%f slow=%f)\n",
                    fastFall,
                    slowFall);
        return 1;
    }

    std::printf("PairArEnvelope_test OK\n");
    return 0;
}
