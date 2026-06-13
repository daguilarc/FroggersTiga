#pragma once

#include <cstdint>

namespace ParamDisplayNames
{
constexpr uint8_t kNumHostPages = 6;
constexpr uint8_t kNumRows = 8;
constexpr uint8_t kCrunchRow = 7;
constexpr uint8_t kDelayHostPage = 5;

inline const char* forHostPageRow(uint8_t hostPage, uint8_t row)
{
    static const char* kTable[kNumHostPages][kNumRows] = {
        {"VCO1", "VCO2", "VCO3", "Cross-coupler", "Phase mod 1", "Phase mod 2", "VCO level", "Crunch"},
        {"Step chance", "Deja vu 1", "Bag size 1", "Slew 1", "Deja vu 2", "Bag size 2", "Slew 2", "Crunch"},
        {"Wet/dry", "Room size", "Decay", "Pre-delay", "Damping", "Stereo width", "Diffusion", "Crunch"},
        {"Comb offset", "Peak freq", "Peak gain", "Peak Q", "Comb delay", "Comb feedback", "Comb LP", "Crunch"},
        {"Drive", "Shape", "SRR 1", "SRR 2", "XOR", "Bit depth", "Fuzz", "Crunch"},
        {"Delay time", "Send", "Feedback", "Stereo width", "Detune", "Mod depth", "Wet mix", "Crunch"},
    };
    if (hostPage >= kNumHostPages || row >= kNumRows)
    {
        return "";
    }
    return kTable[hostPage][row];
}
} // namespace ParamDisplayNames
