#pragma once

#include "AudioEngine.h"
#include "MidiCvSettingsComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

class ControllersRuntimePageComponent : public juce::Component,
                                        private juce::Timer
{
public:
    explicit ControllersRuntimePageComponent(AudioEngine& engine);

    void resized() override;

private:
    void timerCallback() override;
    void refreshStatus();

    AudioEngine& m_engine;
    juce::Label m_title;
    juce::Label m_inputStateLabel;
    juce::Label m_persistenceLabel;
    juce::Label m_fanOutLabel;
    juce::Viewport m_viewport;
    MidiCvSettingsComponent m_settings;
};
