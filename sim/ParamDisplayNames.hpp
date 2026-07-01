#pragma once

#include <cstdint>

namespace ParamDisplayNames
{
constexpr uint8_t kNumHostPages = 6;
constexpr uint8_t kNumRows = 8;
constexpr uint8_t kCrispyRow = 7;
constexpr uint8_t kDelayHostPage = 5;
constexpr uint8_t kRandomHostPage = 1;

inline const char* forHostPage(uint8_t hostPage)
{
    static const char* kPages[kNumHostPages] = {
        "Audio", "Random S&H", "Reverb", "Filter", "Drive", "Delay",
    };
    if (hostPage >= kNumHostPages)
    {
        return "";
    }
    return kPages[hostPage];
}

inline const char* forModSource(uint8_t modIndex)
{
    switch (modIndex)
    {
        case 0:
            return "MIDI CC 1";
        case 1:
            return "MIDI CC 2";
        case 4:
            return "VCO Envelope";
        case 5:
            return "Random S&H 1";
        case 6:
            return "Random S&H 2";
        default:
            return "";
    }
}

inline const char* forHostPageRow(uint8_t hostPage, uint8_t row)
{
    static const char* kTable[kNumHostPages][kNumRows] = {
        {"VCO1", "VCO2", "VCO3", "Cross-coupler", "Phase mod 1", "Phase mod 2", "Phase mod 3", "Crispy"},
        {"Step chance", "Deja vu 1", "Bag size 1", "Slew 1", "Deja vu 2", "Bag size 2", "Slew 2", "Crispy"},
        {"Wet/dry", "Room size", "Decay", "Pre-delay", "Damping", "Stereo width", "Diffusion", "Crispy"},
        {"Comb offset", "Peak freq", "Peak gain", "Peak Q", "Comb delay", "Comb feedback", "Comb LP", "Crispy"},
        {"Drive", "Shape", "SRR 1", "SRR 2", "XOR", "Bit depth", "Fuzz", "Crispy"},
        {"Delay time", "Send", "Feedback", "Stereo width", "Detune", "Mod depth", "Wet mix", "Crispy"},
    };
    if (hostPage >= kNumHostPages || row >= kNumRows)
    {
        return "";
    }
    return kTable[hostPage][row];
}

inline const char* forAudioPairAr(uint8_t index)
{
    static const char* kLabels[4] = {
        "Attack 1+2", "Release 1+2", "Attack 2+3", "Release 2+3",
    };
    if (index >= 4)
    {
        return "";
    }
    return kLabels[index];
}

enum class GlobalStripAction : uint8_t
{
    RandAll = 0,
    RandMods = 1,
    MarblesStep = 2,
    RandWaveforms = 3,
};

inline const char* forGlobalStrip(GlobalStripAction action)
{
    switch (action)
    {
        case GlobalStripAction::RandAll:
            return "Rand All";
        case GlobalStripAction::RandMods:
            return "Rand Mods";
        case GlobalStripAction::MarblesStep:
            return "Rand Resample";
        case GlobalStripAction::RandWaveforms:
            return "Rand waveforms";
    }
    return "";
}
} // namespace ParamDisplayNames
