#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MainComponent.h"
#include "PluginProcessor.h"

class FroggersTigaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit FroggersTigaAudioProcessorEditor(FroggersTigaAudioProcessor& processor);
    ~FroggersTigaAudioProcessorEditor() override = default;

    void resized() override;

private:
    MainComponent m_main;
    FroggersTigaAudioProcessor& m_processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FroggersTigaAudioProcessorEditor)
};
