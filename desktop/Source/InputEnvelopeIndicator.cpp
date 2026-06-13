#include "InputEnvelopeIndicator.h"

void InputEnvelopeIndicator::setActive(bool active)
{
    if (m_active == active)
    {
        return;
    }
    m_active = active;
    if (!m_active)
    {
        m_level = 0.0f;
    }
    repaint();
}

void InputEnvelopeIndicator::setLevel(float level)
{
    const float target = juce::jlimit(0.0f, 1.0f, level);
    if (!m_active)
    {
        if (m_level != 0.0f)
        {
            m_level = 0.0f;
            repaint();
        }
        return;
    }

    const float next = m_level + 0.35f * (target - m_level);
    if (std::abs(next - m_level) < 0.001f && std::abs(target - next) < 0.001f)
    {
        return;
    }
    m_level = next;
    repaint();
}

void InputEnvelopeIndicator::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    if (!m_active)
    {
        g.setColour(juce::Colour(0xff3d444d));
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(juce::Colour(0xff6e7681));
        const float midX = bounds.getCentreX();
        g.fillRect(midX - 1.0f, bounds.getY() + 2.0f, 2.0f, bounds.getHeight() - 4.0f);
        return;
    }

    g.setColour(juce::Colour(0xff3d444d));
    g.fillRoundedRectangle(bounds, 2.0f);

    const float alpha = m_level > 0.02f ? 1.0f : 0.55f;
    g.setColour(juce::Colour(0xff5a9fd4).withAlpha(alpha));
    const float fillW = m_level > 0.0f ? juce::jmax(2.0f, bounds.getWidth() * m_level) : 0.0f;
    if (fillW > 0.0f)
    {
        g.fillRoundedRectangle(bounds.withWidth(fillW), 2.0f);
    }
}
