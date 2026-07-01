#include "PluginEditorV2.h"

#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/DesktopV2LookAndFeel.hpp"

namespace
{
DesktopV2LookAndFeel g_desktopV2LookAndFeel;
}

FroggersTigaAudioProcessorEditorV2::FroggersTigaAudioProcessorEditorV2(
    FroggersTigaAudioProcessorV2& processor)
    : AudioProcessorEditor(&processor)
    , m_main(processor.getAudioEngine(), processor.getControlCore(), processor.getHostBridge())
    , m_processor(processor)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&g_desktopV2LookAndFeel);
    addAndMakeVisible(m_main);
    setResizable(true, true);
    setResizeLimits(DesktopV2ChromeLayout::kHostedEditorMinWidth,
                    DesktopV2ChromeLayout::kHostedEditorMinHeight,
                    8192,
                    4320);
    setSize(DesktopV2ChromeLayout::kDefaultWidth, DesktopV2ChromeLayout::kDefaultHeight);
}

void FroggersTigaAudioProcessorEditorV2::resized()
{
    m_main.setBounds(getLocalBounds());
}
