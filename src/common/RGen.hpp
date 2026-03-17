#pragma once

#include <cstddef>
#include <cstdint>

struct RGen
{
    RGen()
    {
    }

    static uint32_t s_state;

    static uint32_t NextUInt()
    {
        uint32_t x = s_state;
        if (x == 0u)
        {
            x = 0x6d2b79f5u;
        }

        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        s_state = x;
        return x;
    }

    float NormGen()
    {
        float output = 0.0f;
        for (size_t i = 0; i < 6; i++)
        {
            output += UniGen();
        }

        return (output - 3.0f) * 1.41421356237f;
    }

    float NormGen(float mu, float sigma)
    {
        return mu + sigma * NormGen();
    }

    float UniGen()
    {
        return static_cast<float>(NextUInt() >> 8) * (1.0f / 16777216.0f);
    }

    float UniGenRange(float min, float max)
    {
        return min + (max - min) * UniGen();
    }

    size_t RangeGen(size_t max)
    {
        if (max == 0)
        {
            return 0;
        }

        return static_cast<size_t>(NextUInt() % max);
    }
};

inline uint32_t RGen::s_state = 0xa341316cu;
