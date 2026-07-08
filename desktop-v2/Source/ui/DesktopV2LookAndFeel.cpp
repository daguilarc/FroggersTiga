#include "DesktopV2LookAndFeel.hpp"

#ifndef FROGGERS_V2_BUNDLED_IBM_PLEX
#define FROGGERS_V2_BUNDLED_IBM_PLEX 0
#endif

#if FROGGERS_V2_BUNDLED_IBM_PLEX
#include "BinaryData.h"
#endif

namespace
{
constexpr const char* kRegularFontFile = "IBMPlexSans-Regular.ttf";
constexpr const char* kSemiBoldFontFile = "IBMPlexSans-SemiBold.ttf";
constexpr const char* kFamilyIbmPlex = "IBM Plex Sans";

bool isHelveticaFamily(const juce::String& name)
{
    return name.containsIgnoreCase("Helvetica");
}

juce::Typeface::Ptr typefaceFromBundled(const char* filename)
{
#if FROGGERS_V2_BUNDLED_IBM_PLEX
    int size = 0;
    const char* data = BinaryData::getNamedResource(filename, size);
    if (data != nullptr && size > 0)
        return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
#else
    juce::ignoreUnused(filename);
#endif
    return nullptr;
}

juce::StringArray systemSansFamilies()
{
    juce::StringArray families;
    families.add(kFamilyIbmPlex);
#if JUCE_MAC
    families.add("SF Pro Text");
    families.add(".AppleSystemUIFont");
#elif JUCE_WINDOWS
    families.add("Segoe UI");
#else
    families.add("Ubuntu");
    families.add("DejaVu Sans");
#endif
    families.add("Arial");
    return families;
}

juce::Typeface::Ptr typefaceFromSystemFamilies(const juce::StringArray& families,
                                               const int styleFlags)
{
    for (const auto& family : families)
    {
        const juce::Font probe(juce::FontOptions(family, 16.0f, styleFlags));
        const auto name = probe.getTypefaceName();
        if (name.isEmpty() || isHelveticaFamily(name))
            continue;

        auto face = juce::Font::getDefaultTypefaceForFont(probe);
        if (face != nullptr)
            return face;
    }
    return nullptr;
}

juce::Typeface::Ptr resolveTypeface(const char* bundledFile, const int styleFlags)
{
    if (auto bundled = typefaceFromBundled(bundledFile))
        return bundled;

    return typefaceFromSystemFamilies(systemSansFamilies(), styleFlags);
}
} // namespace

DesktopV2LookAndFeel::DesktopV2LookAndFeel()
{
    m_regularTypeface = resolveTypeface(kRegularFontFile, juce::Font::plain);
    m_semiBoldTypeface = resolveTypeface(kSemiBoldFontFile, juce::Font::bold);
    if (m_semiBoldTypeface == nullptr)
        m_semiBoldTypeface = m_regularTypeface;

    if (m_regularTypeface != nullptr)
        setDefaultSansSerifTypeface(m_regularTypeface);
}

juce::Font DesktopV2LookAndFeel::regularFont(const float heightPt) const
{
    if (m_regularTypeface != nullptr)
        return juce::Font(juce::FontOptions(m_regularTypeface).withHeight(heightPt));

    return juce::Font(juce::FontOptions(kFamilyIbmPlex, heightPt, juce::Font::plain));
}

juce::Font DesktopV2LookAndFeel::semiBoldFont(const float heightPt) const
{
    if (m_semiBoldTypeface != nullptr)
        return juce::Font(juce::FontOptions(m_semiBoldTypeface).withHeight(heightPt));

    return juce::Font(juce::FontOptions(kFamilyIbmPlex, heightPt, juce::Font::bold));
}

juce::Font DesktopV2LookAndFeel::getLabelFont(juce::Label&)
{
    return regularFont(kBodyFontHeightPt);
}

juce::Font DesktopV2LookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return regularFont(kBodyFontHeightPt);
}

juce::Font DesktopV2LookAndFeel::getTextButtonFont(juce::TextButton&, const int buttonHeight)
{
    return regularFont(juce::jmin(14.0f, static_cast<float>(buttonHeight) * 0.55f));
}

juce::Font DesktopV2LookAndFeel::getPopupMenuFont()
{
    return regularFont(kBodyFontHeightPt);
}

juce::Font DesktopV2LookAndFeel::getSliderPopupFont(juce::Slider&)
{
    return regularFont(kBodyFontHeightPt);
}

juce::Typeface::Ptr DesktopV2LookAndFeel::getTypefaceForFont(const juce::Font& font)
{
    if (font.isBold() && m_semiBoldTypeface != nullptr)
        return m_semiBoldTypeface;

    if (m_regularTypeface != nullptr)
        return m_regularTypeface;

    return LookAndFeel_V4::getTypefaceForFont(font);
}

void DesktopV2LookAndFeel::drawToggleButton(juce::Graphics& g,
                                            juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    if (button.getRadioGroupId() == 0)
    {
        LookAndFeel_V4::drawToggleButton(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        return;
    }

    const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const juce::Rectangle<float> circle(bounds.getCentreX() - diameter * 0.5f,
                                        bounds.getCentreY() - diameter * 0.5f,
                                        diameter,
                                        diameter);

    const juce::Colour fill = button.getToggleState()
                                  ? juce::Colour(0xff58a6ff)
                                  : juce::Colour(0xff30363d);
    const juce::Colour outline = shouldDrawButtonAsHighlighted ? juce::Colours::white.withAlpha(0.85f)
                                                               : juce::Colour(0xff484f58);
    g.setColour(outline);
    g.drawEllipse(circle, 1.5f);
    g.setColour(fill);
    g.fillEllipse(circle.reduced(3.0f));

    if (button.getToggleState())
    {
        g.setColour(juce::Colours::white);
        g.fillEllipse(circle.reduced(diameter * 0.32f));
    }

    g.setColour(button.findColour(juce::ToggleButton::textColourId));
    g.setFont(regularFont(kBodyFontHeightPt));
    g.drawFittedText(button.getButtonText(),
                     button.getLocalBounds().withTrimmedLeft(static_cast<int>(diameter) + 4),
                     juce::Justification::centredLeft,
                     1);
}
