#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class PairArRotatedLabel : public juce::Component
{
public:
    void setText(juce::String text)
    {
        if (m_text == text)
        {
            return;
        }
        m_text = std::move(text);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (m_text.isEmpty())
        {
            return;
        }

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));

        const juce::Rectangle<float> area = getLocalBounds().toFloat();
        const float centreX = area.getCentreX();
        const float centreY = area.getCentreY();
        g.addTransform(juce::AffineTransform::rotation(
            juce::MathConstants<float>::halfPi, centreX, centreY));
        g.drawText(m_text, area, juce::Justification::centred);
    }

private:
    juce::String m_text;
};
