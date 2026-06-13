#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class WaveMorphButton : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    WaveMorphButton();

    void setMorph(float morph01);
    void setOnClick(std::function<void()> handler);
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    float m_morph = 0.0f;
    bool m_hovered = false;
    std::function<void()> m_onClick;
};
