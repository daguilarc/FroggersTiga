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
    void configureCcSlider(juce::Slider& slider);

    AudioEngine& m_engine;
    std::function<void()> m_onClose;

    juce::Label m_inSectionLabel;
    juce::Label m_inLabel;
    juce::ComboBox m_inDevice;
    juce::TextButton m_refresh{"Refresh devices"};
    juce::Label m_inCc1GroupLabel;
    juce::Label m_inCh1Label;
    juce::Slider m_inChannel1;
    juce::Label m_inCc1Label;
    juce::Slider m_inCc1;
    juce::ToggleButton m_inCc1Enable{"On"};
    juce::Label m_inCc2GroupLabel;
    juce::Label m_inCh2Label;
    juce::Slider m_inChannel2;
    juce::Label m_inCc2Label;
    juce::Slider m_inCc2;
    juce::ToggleButton m_inCc2Enable{"On"};
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
