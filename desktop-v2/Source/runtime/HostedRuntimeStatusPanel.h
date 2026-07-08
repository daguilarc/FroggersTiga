#pragma once

#include "AudioEngine.h"
#include "runtime/DesktopV2AudioStateProjection.hpp"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class HostedRuntimeStatusPanel : public juce::Component,
                                 private juce::Timer
{
public:
    HostedRuntimeStatusPanel(AudioEngine& engine, juce::AudioProcessor* processor = nullptr);

    void resized() override;
    void setProcessor(juce::AudioProcessor* processor);

private:
    void timerCallback() override;
    void refreshLabels();
    void toggleExpanded();

    AudioEngine& m_engine;
    juce::AudioProcessor* m_processor = nullptr;
    bool m_expanded = false;
    juce::TextButton m_toggle{"Host Status"};
    juce::Label m_audioTitle;
    juce::Label m_audioBusLabel;
    juce::Label m_audioChannelsLabel;
    juce::Label m_audioRateLabel;
    juce::Label m_audioBlockLabel;
    juce::Label m_audioInputStateLabel;
    juce::Label m_audioOutputStateLabel;
    juce::Label m_midiTitle;
    juce::Label m_midiSummaryLabel;
};
