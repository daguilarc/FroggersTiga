#pragma once

#include "AudioEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

class FilePatchRuntimePageComponent : public juce::Component
{
public:
    explicit FilePatchRuntimePageComponent(AudioEngine& engine);

    void resized() override;
    void refreshState();

private:
    void handleSave();
    void handleLoad();
    void handleRevert();

    AudioEngine& m_engine;
    juce::Label m_title;
    juce::Label m_identityLabel;
    juce::Label m_dirtyLabel;
    juce::Label m_saveResultLabel;
    juce::Label m_loadResultLabel;
    juce::Label m_revertResultLabel;
    juce::Label m_mappingResultLabel;
    juce::TextButton m_save{"Save"};
    juce::TextButton m_load{"Load"};
    juce::TextButton m_revert{"Revert"};
    juce::TextEditor m_log;
};
