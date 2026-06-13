#include "ModRackPanel.h"

#include "DesktopChromeLayout.hpp"

ModRackPanel::ModRackPanel(DesktopHostIO& host)
    : m_host(host)
    , m_midi("MIDI", 0, m_host)
    , m_vcoFeat("VCO Envelope", 4, m_host)
    , m_marbles1("Marbles 1", 5, m_host)
    , m_marbles2("Marbles 2", 6, m_host)
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
        "Random CV — press Marbles to step. Flat between steps is normal; SLW adds a ramp. PROB may skip. Start Play for live trace.");
    m_marbles2.setTooltip(
        "Random CV — press Marbles to step. Flat between steps is normal; SLW adds a ramp. PROB may skip. Start Play for live trace.");
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
