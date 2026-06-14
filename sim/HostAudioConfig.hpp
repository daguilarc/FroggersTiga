#pragma once

#include <cstddef>

namespace HostAudioConfig
{
constexpr double kSupportedRates[] = {44100.0, 48000.0};
constexpr int kNumSupportedRates = 2;
constexpr double kDefaultSampleRate = 44100.0;
constexpr double kMaxSampleRate = 48000.0;
constexpr int kMaxRecordingMinutes = 30;

constexpr size_t maxRecordingSamples()
{
    return static_cast<size_t>(kMaxSampleRate) * 60 * kMaxRecordingMinutes;
}

inline int indexForRate(double sampleRate)
{
    for (int i = 0; i < kNumSupportedRates; ++i)
    {
        if (sampleRate == kSupportedRates[i])
        {
            return i;
        }
    }
    return 0;
}

inline double nearestSupportedRate(double sampleRate)
{
    int bestIndex = 0;
    double bestDelta = 1.0e9;
    for (int i = 0; i < kNumSupportedRates; ++i)
    {
        const double delta = sampleRate > kSupportedRates[i]
                                 ? sampleRate - kSupportedRates[i]
                                 : kSupportedRates[i] - sampleRate;
        if (delta < bestDelta)
        {
            bestDelta = delta;
            bestIndex = i;
        }
    }
    return kSupportedRates[bestIndex];
}
} // namespace HostAudioConfig
