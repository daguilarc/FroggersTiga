#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class InputEnvelopeIndicator : public juce::Component,
                               public juce::SettableTooltipClient
{
public:
    void setActive(bool active);
    void setLevel(float level);
    void paint(juce::Graphics& g) override;

private:
    bool m_active = false;
    float m_level = 0.0f;
};
