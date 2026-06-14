#include "ModRackPanel.h"

#include "ParamDisplayNames.hpp"

#include "DesktopChromeLayout.hpp"

ModRackPanel::ModRackPanel(DesktopHostIO& host)
    : m_host(host)
    , m_midi("MIDI", 0, m_host)
    , m_vcoFeat("VCO Envelope", 4, m_host)
    , m_marbles1(ParamDisplayNames::forModSource(5), 5, m_host)
    , m_marbles2(ParamDisplayNames::forModSource(6), 6, m_host)
{
    for (juce::Component* box :
         {static_cast<juce::Component*>(&m_midi),
          static_cast<juce::Component*>(&m_vcoFeat),
          static_cast<juce::Component*>(&m_marbles1),
          static_cast<juce::Component*>(&m_marbles2)})
    {
        addAndMakeVisible(box);
    }
    m_marbles1.setTooltip(
        "Random 1 S&H — held random mod CV (0–100%). Steps on Random press. Green LED when CV > 55% while playing.");
    m_marbles2.setTooltip(
        "Random 2 S&H — held random mod CV (0–100%). Steps on Random press. Green LED when CV > 55% while playing.");
    setSize(1200, 72);
}

void ModRackPanel::refresh(bool audioRunning)
{
    m_midi.refresh(audioRunning);
    m_vcoFeat.refresh(audioRunning);
    m_marbles1.refresh(audioRunning);
    m_marbles2.refresh(audioRunning);
}

void ModRackPanel::collectOutputPorts(std::vector<PatchCableOverlay::OutputPort>& ports) const
{
    const ModModuleBox* boxes[] = {&m_midi, &m_vcoFeat, &m_marbles1, &m_marbles2};
    for (const ModModuleBox* box : boxes)
    {
        PatchCableOverlay::OutputPort port;
        port.modIndex = box->getModIndex();
        port.screenBounds = box->getOutputJackScreenBounds();
        ports.push_back(port);
    }
}

void ModRackPanel::resized()
{
    using namespace DesktopChromeLayout;

    auto area = getLocalBounds().reduced(4);
    const int rackW = kModRackGroupWidth;
    int boxW = kModBoxWidth;
    if (area.getWidth() < rackW)
    {
        const int shrinkW = juce::jmax(
            kModBoxMinWidth,
            (area.getWidth() - kModBoxGap * 3) / 4);
        boxW = shrinkW;
    }

    const int groupW = 4 * boxW + 3 * kModBoxGap;
    int x = area.getX() + (area.getWidth() - groupW) / 2;
    ModModuleBox* boxes[] = {&m_midi, &m_vcoFeat, &m_marbles1, &m_marbles2};
    for (int i = 0; i < 4; ++i)
    {
        boxes[i]->setBounds(x, area.getY(), boxW, area.getHeight());
        x += boxW + kModBoxGap;
    }
}
