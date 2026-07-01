#pragma once

#include "AudioEngine.h"
#include "DesktopV2HostCallbacks.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "ui/GlobalStripV2.hpp"
#include "ui/PageCarouselComponent.hpp"
#include "ui/PerformanceBandV2.hpp"
#include "ui/VcoEfScopeDisplay.hpp"
#include "ui/SequencerPanelComponent.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <memory>
#include <optional>

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();

    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;

private:
    void timerCallback() override;
    void wireCallbacks();
    void pushSelectPage(uint8_t page);
    void pushRandomizeMod(uint8_t page);
    void syncHostModRoutesIfNeeded();
    void updateShiftFromKeyboard();
    void pushModSourceSamples();
    void pushExternalMidiMods();
    void drainMidiUiActions();
    void wireMidiCvCallbacks();
    bool shouldCaptureQwertyMidi() const;
    int noteForKeyCode(int keyCode) const;
    void syncQwertyKey(int note, bool down);
    void updateTransportUi();

    std::optional<AudioEngine> m_audio;
    froggers_v2::FroggersV2ControlCore m_core;
    froggers_v2::FroggersV2HostBridge m_bridge;
    VcoEfScopeDisplay m_vcoEfScope;
    PerformanceBandV2 m_performanceBand;
    PageCarouselComponent m_carousel;
    GlobalStripV2 m_globalStrip;
    SequencerPanelComponent m_sequencerPanel;
    juce::TextButton m_play{"Engine"};
    juce::TextButton m_stop{"Stop"};
    juce::TextButton m_audioSettings{"Audio"};
    juce::TextButton m_midiSettings{"MIDI"};
    bool m_sequencerVisible = true;
    uint32_t m_lastUiVersion = 0;
    uint32_t m_lastModRoutesVersion = 0;
    desktop_v2::HostCallbackContext m_hostCallbacks;
    std::array<uint8_t, 128> m_qwertyHeldVelocity{};
};
