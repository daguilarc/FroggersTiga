#include "GlobalStrip.h"

#include "ParamDisplayNames.hpp"

GlobalStrip::GlobalStrip(DesktopHostIO& host, DelayState& delay)
    : m_host(host)
    , m_delay(delay)
{
    m_randomizeAll.setButtonText(
        ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::RandAll));
    m_randomizeMod.setButtonText(
        ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::RandMods));
    m_randomizeVcoWaveform.setButtonText(
        ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::RandWaveforms));
    m_marbles.setButtonText(
        ParamDisplayNames::forGlobalStrip(ParamDisplayNames::GlobalStripAction::MarblesStep));
    m_randomizeAll.onClick = [this]() { m_host.EnqueueRandomizeAllPages(); };
    m_randomizeMod.onClick = [this]() { m_host.EnqueueRandomizeAllMod(); };
    m_randomizeVcoWaveform.onClick = [this]() { m_host.RandomizeVcoMorphs(); };
    m_marbles.onClick = [this]() { m_host.PressButton(0); };

    m_randomizeAll.setTooltip("Randomize all pages + Delay knobs");
    m_randomizeMod.setTooltip("Randomize mod routes on all pages");
    m_randomizeVcoWaveform.setTooltip("Randomize VCO waveform morph (sine/saw/square blend)");
    m_marbles.setTooltip("Resample both random S&H channels (draws from bags)");

    for (juce::Component* c : {static_cast<juce::Component*>(&m_randomizeAll),
                              static_cast<juce::Component*>(&m_randomizeMod),
                              static_cast<juce::Component*>(&m_randomizeVcoWaveform),
                              static_cast<juce::Component*>(&m_marbles)})
    {
        addAndMakeVisible(c);
    }

    setSize(1200, 36);
}

void GlobalStrip::resized()
{
    auto area = getLocalBounds().reduced(4);
    constexpr int kGap = 4;
    juce::TextButton* buttons[] = {
        &m_randomizeAll, &m_randomizeMod, &m_randomizeVcoWaveform, &m_marbles};
    int totalW = 0;
    int widths[4];
    for (int i = 0; i < 4; ++i)
    {
        widths[i] = buttons[i]->getBestWidthForHeight(area.getHeight());
        totalW += widths[i];
    }
    totalW += kGap * 3;

    int x = area.getX() + (area.getWidth() - totalW) / 2;
    for (int i = 0; i < 4; ++i)
    {
        buttons[i]->setBounds(x, area.getY(), widths[i], area.getHeight());
        x += widths[i] + kGap;
    }
}
