#include "ui/ModLanePicker.hpp"

#include "manifest/FroggersV2AppManifest.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "V2ModSourceCatalog.hpp"
#include "control/FroggersV2ControlCore.hpp"

namespace
{
constexpr uint8_t kMenuNone = 255;

const char* labelForInternal(uint8_t internal)
{
    if (internal >= froggers_v2::kNumModSources)
    {
        return "";
    }
    return froggers_v2::manifest::kPermanentModulationSources[internal].displayName;
}

uint8_t internalIndexForMenuId(int menuId)
{
    if (menuId == kMenuNone)
    {
        return froggers_v2::kNoSelection;
    }
    if (menuId < 0 || menuId >= static_cast<int>(froggers_v2::kNumModSources))
    {
        return froggers_v2::kNoSelection;
    }
    return static_cast<uint8_t>(menuId);
}

int menuIdForInternal(uint8_t internal)
{
    return static_cast<int>(internal);
}

bool isExternalAudioLane(uint8_t lane)
{
    return lane == 13 || lane == 14;
}
} // namespace

ModLanePicker::ModLanePicker()
{
    rebuildMenu();
    m_dropdown.onChange = [this]() {
        if (!onAssign)
        {
            return;
        }
        onAssign(m_row, internalIndexForMenuId(m_dropdown.getSelectedId()));
    };
    addAndMakeVisible(m_dropdown);
}

void ModLanePicker::setRow(uint8_t row)
{
    m_row = row;
}

void ModLanePicker::setAssignedSource(uint8_t internalSourceIndex)
{
    m_assigned = internalSourceIndex;
    if (m_assigned == froggers_v2::kNoSelection)
    {
        m_dropdown.setSelectedId(kMenuNone, juce::dontSendNotification);
    }
    else
    {
        m_dropdown.setSelectedId(menuIdForInternal(m_assigned), juce::dontSendNotification);
    }
    repaint();
}

void ModLanePicker::setExternalAudioAvailable(bool available)
{
    if (m_externalAudioAvailable == available)
    {
        return;
    }
    m_externalAudioAvailable = available;
    rebuildMenu();
}

void ModLanePicker::refresh()
{
    rebuildMenu();
    repaint();
}

void ModLanePicker::rebuildMenu()
{
    m_dropdown.clear(juce::dontSendNotification);
    m_dropdown.addItem("None", kMenuNone);
    for (uint8_t lane = 0; lane < froggers_v2::kNumModSources; ++lane)
    {
        const char* label = permanentModSourceDisplayName(lane);
        m_dropdown.addItem(label, static_cast<int>(lane));
        if (isExternalAudioLane(lane))
        {
            m_dropdown.setItemEnabled(static_cast<int>(lane), m_externalAudioAvailable);
        }
    }
    if (m_assigned == froggers_v2::kNoSelection)
    {
        m_dropdown.setSelectedId(kMenuNone, juce::dontSendNotification);
    }
    else
    {
        m_dropdown.setSelectedId(menuIdForInternal(m_assigned), juce::dontSendNotification);
    }
}

juce::Colour ModLanePicker::sourceColour(uint8_t internalSourceIndex) const
{
    if (internalSourceIndex >= froggers_v2::kNumModSources)
    {
        return juce::Colour(0xff484f58);
    }
    const uint32_t rgb =
        froggers_v2::manifest::kPermanentModulationSources[internalSourceIndex].colorRgb;
    return juce::Colour(static_cast<juce::uint32>(0xff000000u | rgb));
}

void ModLanePicker::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const bool lit = m_assigned != froggers_v2::kNoSelection;
    g.setColour(lit ? sourceColour(m_assigned).withAlpha(0.35f) : juce::Colour(0xff21262d));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(lit ? sourceColour(m_assigned) : juce::Colour(0xff484f58));
    g.drawRoundedRectangle(bounds, 4.0f, lit ? 2.0f : 1.0f);

    if (lit)
    {
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(
            labelForInternal(m_assigned),
            bounds.removeFromTop(static_cast<float>(DesktopV2ChromeLayout::kModLabelStripH)),
            juce::Justification::centred);
    }
    else
    {
        bounds.removeFromTop(static_cast<float>(DesktopV2ChromeLayout::kModLabelStripH));
    }
}

void ModLanePicker::resized()
{
    auto area = juce::Rectangle<int>(0, 0, getWidth(), DesktopV2ChromeLayout::kModCellHeight).reduced(2);
    area.removeFromTop(DesktopV2ChromeLayout::kModLabelStripH);
    m_dropdown.setBounds(area);
}
