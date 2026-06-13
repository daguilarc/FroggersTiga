#pragma once

#include "AudioEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MidiSettingsComponent : public juce::Component,
                              private juce::Timer
{
public:
    MidiSettingsComponent(AudioEngine& engine, std::function<void()> onClose);

    void resized() override;

private:
    void timerCallback() override;
    void refreshDeviceLists();
    void applyInputDevice();
    void applyOutputDevice();
    void updateStatus();

    AudioEngine& m_engine;
    std::function<void()> m_onClose;

    juce::Label m_inSectionLabel;
    juce::Label m_inLabel;
    juce::ComboBox m_inDevice;
    juce::TextButton m_refresh{"Refresh devices"};
    juce::Label m_inChLabel;
    juce::Slider m_inChannel;
    juce::Label m_inCcLabel;
    juce::Slider m_inCc;
    juce::Label m_inLegend;
    juce::Label m_inStatus;

    juce::Label m_outSectionLabel;
    juce::Label m_outLabel;
    juce::ComboBox m_outDevice;
    juce::Label m_outHelp;
    juce::Label m_outCcLabel;
    juce::Slider m_outCc;
    juce::Label m_outChLabel;
    juce::Slider m_outChannel;

    juce::TextButton m_close{"Close"};
};
