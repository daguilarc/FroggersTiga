#pragma once

#include "RGen.hpp"
#include "VcvHostModSource.hpp"
#include "VcvModJack.hpp"

#include <array>
#include <cstdint>

namespace VcvVcoArExtension
{
constexpr uint8_t kVoiceCount = 3;
constexpr uint8_t kRowsPerVoice = 2;
constexpr uint8_t kAttackReleaseRows = kVoiceCount * kRowsPerVoice;
constexpr uint8_t kCrispyRow = kAttackReleaseRows;
constexpr uint8_t kRowCount = kCrispyRow + 1;

inline const char* rowLabel(uint8_t row)
{
    static const char* kLabels[kRowCount] = {
        "Atk VCO1",
        "Rel VCO1",
        "Atk VCO2",
        "Rel VCO2",
        "Atk VCO3",
        "Rel VCO3",
        "Crispy",
    };
    if (row >= kRowCount)
    {
        return "";
    }
    return kLabels[row];
}

struct Row
{
    float base = 0.5f;
    uint8_t internalRoute = 255;
    float routeDepth = 0.0f;
    bool cvConnected = false;
    float cvVoltage = 0.0f;
};

struct Snapshot
{
    std::array<Row, kRowCount> rows{};
    bool randomizeRequested = false;
    bool randmodRequested = false;
};

inline Snapshot defaultSnapshot()
{
    Snapshot snapshot;
    for (uint8_t row = 0; row < kRowCount; row++)
    {
        snapshot.rows[row].base = row == kCrispyRow ? 0.0f : 0.5f;
    }
    return snapshot;
}

inline void randomizeBaseRows(Snapshot& snapshot)
{
    RGen rgen;
    for (uint8_t row = 0; row < kRowCount; row++)
    {
        snapshot.rows[row].base = row == kCrispyRow ? 0.0f : rgen.UniGenRange(0.0f, 1.0f);
    }
}

inline void randomizeRoutes(Snapshot& snapshot)
{
    RGen rgen;
    for (uint8_t row = 0; row < kRowCount; row++)
    {
        snapshot.rows[row].routeDepth = rgen.UniGenRange(0.0f, 1.0f);
        snapshot.rows[row].internalRoute = PickVcvRandomModIndex(rgen);
    }
}
} // namespace VcvVcoArExtension
