#pragma once

#include "AudioEngine.h"
#include "AudioSettingsComponent.h"
#include "runtime/DesktopV2AudioStateProjection.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class AudioRuntimePageComponent : public juce::Component,
                                  private juce::Timer
{
public:
    explicit AudioRuntimePageComponent(AudioEngine& engine);

    void resized() override;
    void refreshProjection();

private:
    void timerCallback() override;
    void layoutProjectionLabels(juce::Rectangle<int>& area);

    AudioEngine& m_engine;
    juce::Label m_title;
    juce::Label m_outputChannelsLabel;
    juce::Label m_inputChannelsLabel;
    juce::Label m_blockSizeLabel;
    juce::Label m_busLayoutLabel;
    juce::Label m_outputStateLabel;
    juce::Label m_inputStateLabel;
    AudioSettingsComponent m_settings;
};
