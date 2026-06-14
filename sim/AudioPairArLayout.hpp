#pragma once

#include <cstdint>

namespace AudioPairArLayout
{
constexpr uint8_t kCellCount = 4;
constexpr uint8_t kAudioHostPage = 0;
constexpr uint8_t kModRowBase = 8;

constexpr int kBandTopPad = 6;
constexpr int kColumnPad = 4;
constexpr int kJackSize = 20;
constexpr int kKnobSize = 38;
constexpr int kLabelRowH = 14;
constexpr int kStackGap = 2;
constexpr int kPairArLabelZoneH = 80;
constexpr int kBandHeight =
    kJackSize + kStackGap + kKnobSize + kStackGap + kPairArLabelZoneH;
} // namespace AudioPairArLayout
