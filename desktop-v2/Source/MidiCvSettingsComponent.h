#pragma once

#include "AudioEngine.h"

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

class MidiCvSettingsComponent : public juce::Component,
                                private juce::Timer
{
public:
    MidiCvSettingsComponent(AudioEngine& engine, std::function<void()> onClose);

    void resized() override;

private:
    struct BindingRowUi
    {
        juce::Label label;
        juce::ToggleButton enable{"On"};
        juce::Label chLabel;
        juce::Slider channel;
        juce::ComboBox kind;
        juce::Slider number;
    };

    void timerCallback() override;
    void refreshDeviceLists();
    void applyInputDevice();
    void applyOutputDevice();
    void updateStatus();
    void updatePitchTargetLabel();
    void configureCcSlider(juce::Slider& slider);
    void syncTableFromUi();
    void syncUiFromTable();
    void layoutBindingRow(juce::Rectangle<int>& area, BindingRowUi& row);

    AudioEngine& m_engine;
    std::function<void()> m_onClose;

    juce::Label m_inSectionLabel;
    juce::Label m_inHelp;
    juce::Label m_inLabel;
    juce::ComboBox m_inDevice;
    juce::TextButton m_refresh{"Refresh devices"};
    juce::Label m_inStatus;

    juce::Label m_assignSectionLabel;
    juce::Label m_assignHelp;
    juce::ToggleButton m_pitchEnable{"Pitch"};
    juce::Label m_pitchPageLabel;
    juce::ComboBox m_pitchPage;
    juce::Label m_pitchRowLabel;
    juce::Slider m_pitchRow;
    juce::Label m_pitchTargetLabel;
    juce::ToggleButton m_gateEnable{"Gate"};
    juce::Label m_gateHelp;
    juce::Label m_extModALabel;
    juce::ToggleButton m_extModAEnable{"On"};
    juce::Label m_extModAChLabel;
    juce::Slider m_extModAChannel;
    juce::Label m_extModACcLabel;
    juce::Slider m_extModACc;
    juce::Label m_extModBLabel;
    juce::ToggleButton m_extModBEnable{"On"};
    juce::Label m_extModBChLabel;
    juce::Slider m_extModBChannel;
    juce::Label m_extModBCcLabel;
    juce::Slider m_extModBCc;
    std::array<BindingRowUi, 4> m_bindingRows;
    juce::ToggleButton m_qwertyEnable{"QWERTY virtual MIDI"};
    juce::Label m_qwertyChLabel;
    juce::Slider m_qwertyChannel;
    juce::ToggleButton m_externalClock{"External MIDI clock"};

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
