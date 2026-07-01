#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "DesktopHostIO.hpp"
#include "ui/EncoderRingComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class GlobalStripV2 : public juce::Component
{
public:
    GlobalStripV2();

    void bind(DesktopHostIO* host, froggers_v2::FroggersV2ControlCore* core);
    void setShiftHeld(bool held);
    void refresh();

    void resized() override;

private:
    void pushShift(bool held);
    void pushRandAll();

    DesktopHostIO* m_host = nullptr;
    froggers_v2::FroggersV2ControlCore* m_core = nullptr;

    juce::TextButton m_randAll;
    juce::TextButton m_randMods;
    juce::TextButton m_randWaveforms;
    juce::TextButton m_randResample;
    juce::Label m_crunchyLabel;
    EncoderRingComponent m_crunchyRing;
    juce::ToggleButton m_shift{"Shift"};
    juce::TextButton m_lfo{"LFO"};
    juce::TextButton m_vco{"VCO"};
};
