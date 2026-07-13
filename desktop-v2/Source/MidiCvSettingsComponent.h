#pragma once

#include "AudioEngine.h"
#include "manifest/FroggersV2AppManifest.hpp"

#include <array>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

enum class MidiCvSettingsPresentation
{
    Dialog,
    RuntimePage
};

enum class MidiCvTargetRowKind : uint8_t
{
    Pitch = 0,
    Gate,
    CcBinding,
    ButtonBinding,
    QwertyChannel,
    ExternalClock,
    EncoderTurn,
    EncoderDrillIn
};

class MidiCvSettingsComponent : public juce::Component,
                                private juce::Timer
{
public:
    MidiCvSettingsComponent(AudioEngine& engine,
                            std::function<void()> onClose = {},
                            MidiCvSettingsPresentation presentation = MidiCvSettingsPresentation::Dialog);

    void resized() override;
    int preferredContentHeight() const;

private:
    struct TargetRowUi
    {
        const char* targetId = nullptr;
        const char* bindingRole = nullptr;
        uint8_t targetPage = froggers_v2::manifest::kControllerTargetNoPageRow;
        uint8_t targetRow = froggers_v2::manifest::kControllerTargetNoPageRow;
        MidiCvTargetRowKind kind = MidiCvTargetRowKind::Pitch;
        juce::Label label;
        juce::Label readbackLabel;
        juce::Label fanOutLabel;
        juce::ToggleButton enable;
        juce::Label chLabel;
        juce::Slider channel;
        juce::ComboBox kindCombo;
        juce::Slider number;
        juce::Label pageLabel;
        juce::ComboBox page;
        juce::Label rowLabel;
        juce::Slider row;
        juce::Label help;
    };

    void timerCallback() override;
    void refreshDeviceLists();
    void applyInputDevice();
    void applyOutputDevice();
    void updateStatus();
    void updateMappingReadbacks();
    void configureCcSlider(juce::Slider& slider);
    void syncTableFromUi();
    void syncUiFromTable();
    void layoutTargetRow(juce::Rectangle<int>& area, TargetRowUi& row);
    void initTargetRows();
    void wireTargetRowCallbacks();
    void addTargetRowComponents(TargetRowUi& row);
    int estimatedRowHeight(const TargetRowUi& row) const;

    AudioEngine& m_engine;
    std::function<void()> m_onClose;
    MidiCvSettingsPresentation m_presentation;

    juce::Label m_inSectionLabel;
    juce::Label m_inHelp;
    juce::Label m_inLabel;
    juce::ComboBox m_inDevice;
    juce::TextButton m_refresh{"Refresh devices"};
    juce::Label m_inStatus;

    juce::Label m_assignSectionLabel;
    juce::Label m_assignHelp;
    std::array<TargetRowUi, froggers_v2::manifest::kControllerTargetCount> m_targetRows;

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
