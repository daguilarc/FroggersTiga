#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "HostedMainComponentV2.h"
#include "PluginProcessorV2.h"

class FroggersTigaAudioProcessorEditorV2 : public juce::AudioProcessorEditor
{
public:
    explicit FroggersTigaAudioProcessorEditorV2(FroggersTigaAudioProcessorV2& processor);
    ~FroggersTigaAudioProcessorEditorV2() override = default;

    void resized() override;

private:
    HostedMainComponentV2 m_main;
    FroggersTigaAudioProcessorV2& m_processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FroggersTigaAudioProcessorEditorV2)
};
