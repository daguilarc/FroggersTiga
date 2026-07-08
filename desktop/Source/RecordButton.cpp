#include "RecordButton.h"

namespace
{
constexpr float kCircleDiameter = 18.0f;
constexpr float kCircleLeftPad = 2.0f;
constexpr float kTextGap = 6.0f;
} // namespace

RecordButton::RecordButton() = default;

void RecordButton::setRecording(bool recording)
{
    if (m_recording != recording)
    {
        m_recording = recording;
        repaint();
    }
}

void RecordButton::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const juce::Rectangle<float> circle(
        bounds.getX() + kCircleLeftPad,
        bounds.getCentreY() - kCircleDiameter * 0.5f,
        kCircleDiameter,
        kCircleDiameter);

    g.setColour(juce::Colour(0xff8b0000));
    g.fillEllipse(circle);

    if (m_recording)
    {
        g.setColour(juce::Colour(0xffff4444));
        g.fillEllipse(circle.reduced(kCircleDiameter * 0.18f));
    }
    else
    {
        g.setColour(juce::Colour(0xffda3633));
        g.fillEllipse(circle.reduced(kCircleDiameter * 0.32f));
    }

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    const float textX = circle.getRight() + kTextGap;
    g.drawText(
        "Record audio",
        juce::Rectangle<float>(textX, bounds.getY(), bounds.getRight() - textX, bounds.getHeight()),
        juce::Justification::centredLeft);
}

void RecordButton::mouseUp(const juce::MouseEvent& event)
{
    juce::Component::mouseUp(event);
    if (event.mouseWasClicked() && onClick)
    {
        onClick();
    }
}
