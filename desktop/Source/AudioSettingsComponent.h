#pragma once

#include "AudioEngine.h"

#include <juce_gui_basics/juce_gui_basics.h>

class AudioSettingsComponent : public juce::Component,
                               private juce::Timer
{
public:
    AudioSettingsComponent(AudioEngine& engine, std::function<void()> onClose);

    void resized() override;

private:
    void timerCallback() override;
    void refreshDeviceLists();
    void applyOutputDevice();
    void applyInputDevice();
    void updateStatus();
    juce::AudioIODeviceType* getDeviceType() const;

    AudioEngine& m_engine;
    std::function<void()> m_onClose;

    juce::Label m_outLabel;
    juce::ComboBox m_outDevice;
    juce::TextButton m_testButton{"Test"};
    juce::Label m_inLabel;
    juce::ComboBox m_inDevice;
    juce::Label m_inHelp;
    juce::Label m_inMeterLabel;
    std::unique_ptr<juce::Component> m_inMeter;
    juce::TextButton m_refresh{"Refresh devices"};
    juce::Label m_status;
    juce::TextButton m_close{"Close"};
};
