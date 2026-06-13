#pragma once

#include <cmath>
#include <cstdint>

inline float Fuegoize(float value, float fuegKnob, uint8_t row)
{
    if (fuegKnob <= 0.0f)
    {
        return value;
    }

    const float fuegoizationAmount = fuegKnob;
    const uint16_t mask = (1u << static_cast<uint16_t>(std::round(fuegoizationAmount * 8.0f))) - 1u;
    const uint16_t inputInt = static_cast<uint16_t>(value * 255.0f);
    const float inputRemainder = value * 255.0f - static_cast<float>(inputInt);
    uint16_t lowerBits = inputInt & mask;

    lowerBits ^= static_cast<uint16_t>((lowerBits << 3) & mask);
    lowerBits ^= static_cast<uint16_t>((lowerBits >> 5) & mask);
    lowerBits ^= static_cast<uint16_t>((lowerBits << 1) & mask);
    const uint8_t sh = static_cast<uint8_t>(
        1u + (row % static_cast<uint8_t>((mask + 1u) ? (mask + 1u) : 1u)));
    lowerBits ^= static_cast<uint16_t>((lowerBits >> sh) & mask);

    const uint16_t outInt = (inputInt & ~mask) | lowerBits;
    return (static_cast<float>(outInt) + inputRemainder) / 255.0f;
}
