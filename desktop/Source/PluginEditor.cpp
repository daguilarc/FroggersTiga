#include "PluginEditor.h"

#include "DesktopChromeLayout.hpp"

FroggersTigaAudioProcessorEditor::FroggersTigaAudioProcessorEditor(
    FroggersTigaAudioProcessor& processor)
    : AudioProcessorEditor(&processor)
    , m_main(processor.getAudioEngine())
    , m_processor(processor)
{
    addAndMakeVisible(m_main);
    setResizable(true, true);
    setResizeLimits(1024, 600, 8192, 4320);
    setSize(DesktopChromeLayout::kDefaultWidth, DesktopChromeLayout::kDefaultHeight);
}

void FroggersTigaAudioProcessorEditor::resized()
{
    m_main.setBounds(getLocalBounds());
}
