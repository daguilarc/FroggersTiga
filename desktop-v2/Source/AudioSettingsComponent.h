#pragma once

#include "AudioEngine.h"
#include "ExportFormat.hpp"
#include "HostAudioConfig.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

enum class AudioSettingsPresentation
{
    Dialog,
    RuntimePage
};

class AudioSettingsComponent : public juce::Component,
                               private juce::Timer
{
public:
    AudioSettingsComponent(AudioEngine& engine,
                           std::function<void()> onClose = {},
                           AudioSettingsPresentation presentation = AudioSettingsPresentation::Dialog);

    void resized() override;

private:
    void timerCallback() override;
    void refreshDeviceLists();
    void applyOutputDevice();
    void applyInputDevice();
    void applySampleRate();
    void applyExportFormat();
    void syncExportFormatUi();
    void updateStatus();
    juce::AudioIODeviceType* getDeviceType() const;
    double selectedSampleRate() const;
    ExportFormat selectedExportFormat() const;

    AudioEngine& m_engine;
    std::function<void()> m_onClose;
    AudioSettingsPresentation m_presentation;

    juce::Label m_rateLabel;
    juce::ComboBox m_sampleRate;
    juce::Label m_outLabel;
    juce::ComboBox m_outDevice;
    juce::TextButton m_testButton{"Test"};
    juce::Label m_inLabel;
    juce::ComboBox m_inDevice;
    juce::Label m_inHelp;
    juce::Label m_inMeterLabel;
    std::unique_ptr<juce::Component> m_inMeter;
    juce::Label m_exportFormatLabel;
    juce::ToggleButton m_fmtWav{"WAV"};
    juce::ToggleButton m_fmtMp3{"MP3"};
    juce::ToggleButton m_fmtFlac{"FLAC"};
    juce::ToggleButton m_fmtOgg{"OGG"};
    juce::TextButton m_refresh{"Refresh devices"};
    juce::Label m_status;
    juce::TextButton m_close{"Close"};
};
