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
