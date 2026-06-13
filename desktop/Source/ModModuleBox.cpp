#include "ModModuleBox.h"

namespace
{
constexpr uint8_t kMarblesMod1 = 5;
constexpr uint8_t kMarblesMod2 = 6;
constexpr int kStepOversample = 4;

bool isMarblesMod(uint8_t modIndex)
{
    return modIndex == kMarblesMod1 || modIndex == kMarblesMod2;
}

void pushOversampled(CvScopeDisplay& scope, float level, int count)
{
    for (int i = 0; i < count; ++i)
    {
        scope.pushSample(level);
    }
}
} // namespace

ModModuleBox::ModModuleBox(juce::String label, uint8_t modIndex, DesktopHostIO& host)
    : m_label(std::move(label))
    , m_modIndex(modIndex)
    , m_host(host)
{
    addAndMakeVisible(m_scope);
    if (isMarblesMod(modIndex))
    {
        m_scope.setTraceMode(CvTraceMode::StepHold);
        m_scope.setShowGrid(true);
    }
    else
    {
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
        if (!audioRunning)
        {
            m_scope.pushSample(m_host.GetCvOut(m_modIndex));
            m_scope.setIdle(true);
            return;
        }
        m_scope.setIdle(false);
        float rangeMin = 0.0f;
        float rangeMax = 0.0f;
        if (m_host.consumeModScopeRange(m_modIndex, rangeMin, rangeMax))
        {
            if (rangeMin != rangeMax)
            {
                pushOversampled(m_scope, rangeMin, kStepOversample);
                pushOversampled(m_scope, rangeMax, kStepOversample);
            }
            else
            {
                m_scope.pushSample(rangeMax);
            }
            return;
        }
        m_scope.pushSample(m_host.GetCvOut(m_modIndex));
        return;
    }

    m_scope.setIdle(false);
    m_scope.pushSample(m_host.GetCvOut(m_modIndex));
}

void ModModuleBox::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText(m_label, getLocalBounds().removeFromTop(10), juce::Justification::centred);

    g.setColour(juce::Colour(0xff3d4450));
    g.fillEllipse(m_jackBounds.toFloat());
    g.setColour(juce::Colour(0xffc8d0dc));
    g.drawEllipse(m_jackBounds.toFloat(), 2.0f);
}

void ModModuleBox::resized()
{
    auto area = getLocalBounds().reduced(2);
    area.removeFromTop(12);
    const int scopeH = 44;
    m_scope.setBounds(area.removeFromTop(scopeH));
    m_jackBounds = area.withSizeKeepingCentre(14, 14);
}
