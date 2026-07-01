#include "ui/VcoEfScopeDisplay.hpp"

VcoEfScopeDisplay::VcoEfScopeDisplay()
{
    m_scope.setTraceCount(kTraceCount);
    m_scope.setTraceColour(0, juce::Colour(0xffe06c75));
    m_scope.setTraceColour(1, juce::Colour(0xff3fb950));
    m_scope.setTraceColour(2, juce::Colour(0xff58a6ff));
    m_scope.setTraceMode(CvTraceMode::Continuous);
    addAndMakeVisible(m_scope);
}

void VcoEfScopeDisplay::bindHost(DesktopHostIO* host)
{
    m_host = host;
}

void VcoEfScopeDisplay::refresh(bool audioRunning)
{
    if (!m_host)
    {
        return;
    }
    m_scope.setIdle(!audioRunning);
    for (uint8_t i = 0; i < kTraceCount; ++i)
    {
        m_scope.pushSample(i, m_host->GetCvOut(static_cast<uint8_t>(kVco1EfIndex + i)));
    }
}

void VcoEfScopeDisplay::resized()
{
    m_scope.setBounds(getLocalBounds());
}
