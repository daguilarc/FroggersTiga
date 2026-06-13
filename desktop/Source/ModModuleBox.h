#pragma once

#include "CvScopeDisplay.h"
#include "DesktopHostIO.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class ModModuleBox : public juce::Component,
                     public juce::SettableTooltipClient
{
public:
    ModModuleBox(juce::String label, uint8_t modIndex, DesktopHostIO& host);

    uint8_t getModIndex() const;
    juce::Rectangle<float> getOutputJackScreenBounds() const;
    void refresh(bool audioRunning);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String m_label;
    uint8_t m_modIndex;
    DesktopHostIO& m_host;
    CvScopeDisplay m_scope;
    juce::Rectangle<int> m_jackBounds;
    float m_lastLevel = 0.0f;
    bool m_audioRunning = false;
};
