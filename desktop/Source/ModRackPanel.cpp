#include "ModRackPanel.h"

#include "HostPanelLayout.hpp"
#include "ParamDisplayNames.hpp"

#include "DesktopChromeLayout.hpp"

namespace
{
constexpr const char* kMarbles1Tooltip =
    "Random S&H 1 — held random mod CV (0–100%). Steps on Rand Resample. Green brightness tracks CV level (full at ~55%) while playing.";
constexpr const char* kMarbles2Tooltip =
    "Random S&H 2 — held random mod CV (0–100%). Steps on Rand Resample. Green brightness tracks CV level (full at ~55%) while playing.";

bool isMidiCcModIndex(uint8_t modIndex)
{
    return modIndex == 0 || modIndex == 1;
}

bool includeModRackCell(const HostPanelLayout::ModRackCellSpec& spec)
{
#if defined(JucePlugin_Build_VST) || defined(JucePlugin_Build_VST3) || defined(JucePlugin_Build_AU) \
    || defined(JucePlugin_Build_AUv3)
    return spec.includeVst;
#else
    return spec.includeDesktop;
#endif
}
} // namespace

ModRackPanel::ModRackPanel(DesktopHostIO& host)
    : m_host(host)
{
    for (const HostPanelLayout::ModRackCellSpec& spec : HostPanelLayout::kModRackCatalog)
    {
        if (!includeModRackCell(spec))
        {
            continue;
        }

        auto box = std::make_unique<ModModuleBox>(
            ParamDisplayNames::forModSource(spec.modIndex),
            spec.modIndex,
            m_host);
        if (spec.modIndex == 5)
        {
            box->setTooltip(kMarbles1Tooltip);
        }
        else if (spec.modIndex == 6)
        {
            box->setTooltip(kMarbles2Tooltip);
        }
        addAndMakeVisible(*box);
        m_boxes.push_back(std::move(box));
    }
    setSize(1200, 72);
}

void ModRackPanel::refresh(bool audioRunning)
{
    for (const std::unique_ptr<ModModuleBox>& box : m_boxes)
    {
        const uint8_t modIndex = box->getModIndex();
        if (isMidiCcModIndex(modIndex))
        {
            box->setPatchEnabled(m_host.IsModSourceAvailable(modIndex));
        }
        else
        {
            box->setPatchEnabled(true);
        }
        box->refresh(audioRunning);
    }
}

void ModRackPanel::collectOutputPorts(std::vector<PatchCableOverlay::OutputPort>& ports) const
{
    for (const std::unique_ptr<ModModuleBox>& box : m_boxes)
    {
        PatchCableOverlay::OutputPort port;
        port.modIndex = box->getModIndex();
        port.patchEnabled = !isMidiCcModIndex(port.modIndex)
                                || m_host.IsModSourceAvailable(port.modIndex);
        port.screenBounds = box->getOutputJackScreenBounds();
        ports.push_back(port);
    }
}

void ModRackPanel::resized()
{
    using namespace DesktopChromeLayout;

    auto area = getLocalBounds().reduced(4);
    const int boxCount = static_cast<int>(m_boxes.size());
    if (boxCount == 0)
    {
        return;
    }

    const int gapCount = boxCount - 1;
    const int rackW = boxCount * kModBoxWidth + gapCount * kModBoxGap;
    int boxW = kModBoxWidth;
    if (area.getWidth() < rackW)
    {
        const int shrinkW = juce::jmax(
            kModBoxMinWidth,
            (area.getWidth() - kModBoxGap * gapCount) / boxCount);
        boxW = shrinkW;
    }

    const int groupW = boxCount * boxW + gapCount * kModBoxGap;
    int x = area.getX() + (area.getWidth() - groupW) / 2;
    for (const std::unique_ptr<ModModuleBox>& box : m_boxes)
    {
        box->setBounds(x, area.getY(), boxW, area.getHeight());
        x += boxW + kModBoxGap;
    }
}
