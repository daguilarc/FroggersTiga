#pragma once

#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "ParamDisplayNames.hpp"

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
    juce::TextButton m_randomizeVcoWaveform{
        ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::RandWaveforms)};
    juce::TextButton m_marbles{
        ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::MarblesStep)};
};
