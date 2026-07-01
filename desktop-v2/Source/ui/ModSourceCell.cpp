#include "ui/ModSourceCell.hpp"

#include "V2ModSourceCatalog.hpp"
#include "V2ModTapBank.hpp"
#include "control/FroggersV2ControlCore.hpp"

namespace
{
constexpr uint8_t kMenuNone = 255;

uint8_t engineIndexForInternal(uint8_t internal)
{
    return static_cast<uint8_t>(internal + V2ModTapBank::kFirstIndex);
}

const char* labelForInternal(uint8_t internal)
{
    if (internal == froggers_v2::kModSourceMidiCcA)
    {
        return "MIDI CC A";
    }
    if (internal == froggers_v2::kModSourceMidiCcB)
    {
        return "MIDI CC B";
    }
    return V2ModSourceLabel(engineIndexForInternal(internal));
}

uint8_t internalIndexForMenuId(int menuId)
{
    if (menuId == kMenuNone)
    {
        return froggers_v2::kNoSelection;
    }
    if (menuId == froggers_v2::kMenuModMidiCcA)
    {
        return froggers_v2::kModSourceMidiCcA;
    }
    if (menuId == froggers_v2::kMenuModMidiCcB)
    {
        return froggers_v2::kModSourceMidiCcB;
    }
    const int internal = menuId - static_cast<int>(V2ModTapBank::kFirstIndex);
    if (internal < 0 || internal >= static_cast<int>(froggers_v2::kNumModSources))
    {
        return froggers_v2::kNoSelection;
    }
    return static_cast<uint8_t>(internal);
}

int menuIdForInternal(uint8_t internal)
{
    if (internal == froggers_v2::kModSourceMidiCcA)
    {
        return froggers_v2::kMenuModMidiCcA;
    }
    if (internal == froggers_v2::kModSourceMidiCcB)
    {
        return froggers_v2::kMenuModMidiCcB;
    }
    return engineIndexForInternal(internal);
}
} // namespace

ModSourceCell::ModSourceCell()
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

void ModSourceCell::setRow(uint8_t row)
{
    m_row = row;
}

void ModSourceCell::setAssignedSource(uint8_t internalSourceIndex)
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

void ModSourceCell::refresh()
{
    repaint();
}

void ModSourceCell::rebuildMenu()
{
    m_dropdown.clear(juce::dontSendNotification);
    m_dropdown.addItem("None", kMenuNone);
    for (uint8_t engine = V2ModTapBank::kFirstIndex; engine <= V2ModTapBank::kLastIndex; ++engine)
    {
        if (engine >= 10 && engine <= 12)
        {
            continue;
        }
        m_dropdown.addItem(V2ModSourceLabel(engine), static_cast<int>(engine));
    }
    m_dropdown.addItem("MIDI CC A", froggers_v2::kMenuModMidiCcA);
    m_dropdown.addItem("MIDI CC B", froggers_v2::kMenuModMidiCcB);
    m_dropdown.setSelectedId(kMenuNone, juce::dontSendNotification);
}

juce::Colour ModSourceCell::sourceColour(uint8_t internalSourceIndex) const
{
    static constexpr uint32_t kColours[froggers_v2::kNumModSources] = {
        0xffe06c75,
        0xff3fb950,
        0xff58a6ff,
        0xffd2a8ff,
        0xfff778ba,
        0xffa5d6ff,
        0xff3fb950,
        0xff3fb950,
        0xffffa657,
        0xffff7b72,
    };
    if (internalSourceIndex >= froggers_v2::kNumModSources)
    {
        return juce::Colour(0xff484f58);
    }
    return juce::Colour(kColours[internalSourceIndex]);
}

void ModSourceCell::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const bool lit = m_assigned != froggers_v2::kNoSelection;
    g.setColour(lit ? sourceColour(m_assigned).withAlpha(0.35f) : juce::Colour(0xff21262d));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(lit ? sourceColour(m_assigned) : juce::Colour(0xff484f58));
    g.drawRoundedRectangle(bounds, 4.0f, lit ? 2.0f : 1.0f);

    if (!lit)
    {
        return;
    }
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText(labelForInternal(m_assigned), bounds.removeFromTop(14.0f), juce::Justification::centred);
}

void ModSourceCell::resized()
{
    auto area = getLocalBounds().reduced(2);
    area.removeFromTop(14);
    m_dropdown.setBounds(area);
}
