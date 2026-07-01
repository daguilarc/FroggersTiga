#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Bundled targets (when present): desktop-v2/Assets/IBMPlexSans-Regular.ttf
// and desktop-v2/Assets/IBMPlexSans-SemiBold.ttf via BinaryData + FROGGERS_V2_BUNDLED_IBM_PLEX.
class DesktopV2LookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float kBodyFontHeightPt = 11.0f;

    DesktopV2LookAndFeel();

    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getSliderPopupFont(juce::Slider&) override;
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;

private:
    juce::Font regularFont(float heightPt) const;
    juce::Font semiBoldFont(float heightPt) const;

    juce::Typeface::Ptr m_regularTypeface;
    juce::Typeface::Ptr m_semiBoldTypeface;
};
