#pragma once

#include "AudioEngine.h"
#include "GlobalStrip.h"
#include "InputEnvelopeIndicator.h"
#include "ModRackPanel.h"
#include "PanelBackend.hpp"
#include "PatchCableOverlay.h"
#include "RecordExportCluster.h"
#include "SubModulePanel.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <memory>
#include <vector>

class MainComponent : public juce::Component,
                      private juce::Timer,
                      private juce::MidiKeyboardState::Listener
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void updateTransportUi();
    void syncPatchPorts();
    bool shouldCaptureQwertyMidi() const;
    void handleRecordClick();
    void handleNoteOn(juce::MidiKeyboardState* source,
                      int midiChannel,
                      int midiNoteNumber,
                      float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source,
                       int midiChannel,
                       int midiNoteNumber,
                       float velocity) override;

    AudioEngine m_audio;
    ModRackPanel m_modRack;
    PatchCableOverlay m_cableOverlay;
    GlobalStrip m_strip;
    juce::TextButton m_play{"Play"};
    juce::TextButton m_stop{"Stop"};
    juce::ToggleButton m_externalInput{"Ext. In."};
    InputEnvelopeIndicator m_inputEnvelope;
    juce::Label m_routeHint;
    RecordExportCluster m_recordCluster;
    juce::TextButton m_audioSettings{"Audio"};
    juce::TextButton m_midiSettings{"MIDI"};
    std::array<std::unique_ptr<DesktopPanelBackend>, 5> m_coreBackends;
    std::unique_ptr<DelayHostBackend> m_delayBackend;
    std::array<std::unique_ptr<SubModulePanel>, 6> m_panels;
    juce::MidiKeyboardState m_keyboardState;
};
