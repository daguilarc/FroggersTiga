#include "ui/ModLanePicker.hpp"

#include "manifest/FroggersV2AppManifest.hpp"
#include "control/FroggersV2ControlCore.hpp"

namespace
{
constexpr int kMenuNone = 1;      // PopupMenu item id for "None" (ids must be > 0)
constexpr int kMenuLaneBase = 2;  // lane L → item id kMenuLaneBase + L

const char* labelForInternal(uint8_t internal)
{
    if (internal >= froggers_v2::kNumModSources)
    {
        return "None";
    }
    return froggers_v2::manifest::kPermanentModulationSources[internal].displayName;
}
} // namespace

ModLanePicker::ModLanePicker() = default;

void ModLanePicker::setPage(uint8_t page)
{
    m_page = page;
}

void ModLanePicker::setRow(uint8_t row)
{
    m_row = row;
}

void ModLanePicker::setAssignedSource(uint8_t internalSourceIndex)
{
    m_assigned = internalSourceIndex;
    repaint();
}

void ModLanePicker::setExternalAudioAvailable(bool available)
{
    if (m_externalAudioAvailable == available)
    {
        return;
    }
    m_externalAudioAvailable = available;
    repaint();
}

void ModLanePicker::refresh()
{
    repaint();
}

bool ModLanePicker::isLaneAssignable(uint8_t lane) const
{
    if (lane >= froggers_v2::kNumModSources)
    {
        return false;
    }
    // The manifest's isModSourceEligibleForRow() also gates on
    // ModulationSource::randomizable, which is false for both external-audio
    // lanes (they are deliberately excluded from Rand All / Rand Mods). That
    // earlier gate makes the function's own external-audio-availability check
    // unreachable for those two lanes, so external-audio manual-assignment
    // availability is applied directly from the manifest's
    // isExternalAudioModLane() here instead of relying on
    // isModSourceEligibleForRow() for that part.
    if (froggers_v2::manifest::isExternalAudioModLane(lane))
    {
        return m_externalAudioAvailable;
    }
    return froggers_v2::manifest::isModSourceEligibleForRow(m_page, m_row, lane, m_externalAudioAvailable);
}

void ModLanePicker::showRouteMenu()
{
    juce::PopupMenu menu;
    menu.addItem(kMenuNone, "None", true, m_assigned == froggers_v2::kNoSelection);
    for (uint8_t lane = 0; lane < froggers_v2::kNumModSources; ++lane)
    {
        const bool available = isLaneAssignable(lane);
        const bool ticked = m_assigned == lane;
        // Unavailable lanes (e.g. external audio with no input) stay visible but
        // disabled, so the operator can still see the full manifest rack.
        menu.addItem(kMenuLaneBase + static_cast<int>(lane),
                     labelForInternal(lane),
                     available,
                     ticked);
    }

    juce::Component::SafePointer<ModLanePicker> safeSelf(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                       [safeSelf](int result) {
                           if (safeSelf == nullptr || result == 0 || !safeSelf->onAssign)
                           {
                               return;
                           }
                           const uint8_t source = result == kMenuNone
                               ? froggers_v2::kNoSelection
                               : static_cast<uint8_t>(result - kMenuLaneBase);
                           safeSelf->onAssign(safeSelf->m_row, source);
                       });
}

void ModLanePicker::mouseUp(const juce::MouseEvent&)
{
    showRouteMenu();
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
    // Compact manifest route summary chip (not a full-row combobox): a lit
    // source-coloured pill with the route label, or a dim "None" placeholder.
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const bool lit = m_assigned != froggers_v2::kNoSelection;
    g.setColour(lit ? sourceColour(m_assigned).withAlpha(0.35f) : juce::Colour(0xff21262d));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(lit ? sourceColour(m_assigned) : juce::Colour(0xff484f58));
    g.drawRoundedRectangle(bounds, 4.0f, lit ? 2.0f : 1.0f);

    g.setColour(lit ? juce::Colours::white.withAlpha(0.9f) : juce::Colour(0xff8b949e));
    g.setFont(juce::Font(juce::FontOptions(10.0f, lit ? juce::Font::bold : juce::Font::plain)));
    g.drawText(labelForInternal(m_assigned), bounds, juce::Justification::centred);
}
