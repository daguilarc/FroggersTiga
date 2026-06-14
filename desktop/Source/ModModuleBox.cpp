#include "ModModuleBox.h"

namespace
{
constexpr uint8_t kMarblesMod1 = 5;
constexpr uint8_t kMarblesMod2 = 6;
constexpr float kLedOnThreshold = 0.55f;

bool isMarblesMod(uint8_t modIndex)
{
    return modIndex == kMarblesMod1 || modIndex == kMarblesMod2;
}
} // namespace

ModModuleBox::ModModuleBox(juce::String label, uint8_t modIndex, DesktopHostIO& host)
    : m_label(std::move(label))
    , m_modIndex(modIndex)
    , m_host(host)
{
    if (!isMarblesMod(modIndex))
    {
        addAndMakeVisible(m_scope);
        m_scope.setTraceMode(CvTraceMode::Continuous);
        m_scope.setShowGrid(false);
    }
    setSize(128, 68);
}

uint8_t ModModuleBox::getModIndex() const
{
    return m_modIndex;
}

juce::Rectangle<float> ModModuleBox::getOutputJackScreenBounds() const
{
    return localAreaToGlobal(m_jackBounds).toFloat();
}

void ModModuleBox::refresh(bool audioRunning)
{
    if (isMarblesMod(m_modIndex))
    {
        m_audioRunning = audioRunning;
        m_lastLevel = m_host.GetCvOut(m_modIndex);
        repaint();
        return;
    }

    m_scope.setIdle(!m_patchEnabled);
    m_scope.pushSample(m_host.GetCvOut(m_modIndex));
}

void ModModuleBox::setPatchEnabled(bool enabled)
{
    if (m_patchEnabled == enabled)
    {
        return;
    }
    m_patchEnabled = enabled;
    repaint();
}

void ModModuleBox::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(m_patchEnabled ? 0.9f : 0.36f));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText(m_label, getLocalBounds().removeFromTop(10), juce::Justification::centred);

    auto area = getLocalBounds().reduced(2);
    area.removeFromTop(12);
    const int indicatorH = 44;
    auto indicatorArea = area.removeFromTop(indicatorH);

    if (isMarblesMod(m_modIndex))
    {
        const float cx = indicatorArea.getCentreX();
        const float cy = indicatorArea.getCentreY();
        const float radius = 8.0f;
        const bool on = m_audioRunning && m_lastLevel > kLedOnThreshold;
        g.setColour(on ? juce::Colour(0xff3fb950) : juce::Colour(0xff21262d));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    }

    g.setColour(m_patchEnabled ? juce::Colour(0xff3d4450) : juce::Colour(0xff3d4450).withAlpha(0.4f));
    g.fillEllipse(m_jackBounds.toFloat());
    g.setColour(m_patchEnabled ? juce::Colour(0xffc8d0dc) : juce::Colour(0xffc8d0dc).withAlpha(0.35f));
    g.drawEllipse(m_jackBounds.toFloat(), 2.0f);
}

void ModModuleBox::resized()
{
    auto area = getLocalBounds().reduced(2);
    area.removeFromTop(12);
    const int scopeH = 44;
    if (!isMarblesMod(m_modIndex))
    {
        m_scope.setBounds(area.removeFromTop(scopeH));
    }
    else
    {
        area.removeFromTop(scopeH);
    }
    m_jackBounds = area.withSizeKeepingCentre(14, 14);
}
