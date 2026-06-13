#include "WaveMorphButton.h"

#include "VcoWaveEval.hpp"

namespace
{
const juce::Colour kKnobBlue{0xff5a9fd4};

void appendMorphWavePath(juce::Path& path, juce::Rectangle<float> bounds, float morph)
{
    const float x0 = bounds.getX();
    const float width = bounds.getWidth();
    const float yMid = bounds.getCentreY();
    const float amp = bounds.getHeight() * 0.35f;
    for (int i = 0; i <= 24; ++i)
    {
        const float t = static_cast<float>(i) / 24.0f;
        const float x = x0 + t * width;
        const float wave = EvalWaveMorph(t, morph);
        const float y = yMid - amp * wave;
        if (i == 0)
        {
            path.startNewSubPath(x, y);
        }
        else
        {
            path.lineTo(x, y);
        }
    }
}
} // namespace

WaveMorphButton::WaveMorphButton()
{
    setTooltip("Click to cycle wave morph (sine ↔ saw ↔ square)");
}

void WaveMorphButton::setMorph(float morph01)
{
    m_morph = morph01 < 0.0f ? 0.0f : (morph01 > 1.0f ? 1.0f : morph01);
    repaint();
}

void WaveMorphButton::setOnClick(std::function<void()> handler)
{
    m_onClick = std::move(handler);
}

void WaveMorphButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const juce::Colour fillColour = kKnobBlue.withAlpha(m_hovered ? 0.22f : 0.12f);
    const juce::Colour borderColour = m_hovered ? kKnobBlue.brighter(0.15f) : kKnobBlue;

    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.5f);

    juce::Path path;
    appendMorphWavePath(path, bounds.reduced(3.0f), m_morph);
    g.setColour(kKnobBlue.withAlpha(0.95f));
    g.strokePath(path, juce::PathStrokeType(1.5f));
}

void WaveMorphButton::mouseUp(const juce::MouseEvent& event)
{
    juce::Component::mouseUp(event);
    if (event.mouseWasClicked() && m_onClick)
    {
        m_onClick();
    }
}

void WaveMorphButton::mouseEnter(const juce::MouseEvent& event)
{
    juce::Component::mouseEnter(event);
    m_hovered = true;
    repaint();
}

void WaveMorphButton::mouseExit(const juce::MouseEvent& event)
{
    juce::Component::mouseExit(event);
    m_hovered = false;
    repaint();
}
