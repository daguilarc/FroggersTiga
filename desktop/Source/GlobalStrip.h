#pragma once

#include "DelayState.hpp"
#include "DesktopHostIO.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class GlobalStrip : public juce::Component
{
public:
    GlobalStrip(DesktopHostIO& host, DelayState& delay);
    void resized() override;

private:
    DesktopHostIO& m_host;
    DelayState& m_delay;
    juce::TextButton m_randomizeAll{"Rand All"};
    juce::TextButton m_randomizeMod{"Rand Mods"};
    juce::TextButton m_randomizeVcoWaveform{"Rand waves"};
    juce::TextButton m_marbles{"Marbles"};
};
