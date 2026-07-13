#pragma once

#include "AudioEngine.h"
#include "DesktopV2HostCallbacks.hpp"
#include "RecordButton.h"
#include "runtime/AudioRuntimePageComponent.h"
#include "runtime/ControllersRuntimePageComponent.h"
#include "runtime/DesktopV2RuntimeProjection.hpp"
#include "runtime/FilePatchRuntimePageComponent.h"
#include "control/FroggersV2AppCoreFacade.hpp"
#include "ui/PageCarouselComponent.hpp"
#include "ui/PerformanceBandV2.hpp"
#include "ui/GlobalStripV2.hpp"
#include "ui/GlobalOscilloscopeDisplay.hpp"
#include "ui/CvLaneHistoryStore.hpp"
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
    void drainMidiUiActions();
    void wireMidiCvCallbacks();
    bool shouldCaptureQwertyMidi() const;
    int noteForKeyCode(int keyCode) const;
    void syncQwertyKey(int note, bool down);
    void updateTransportUi();
    void handleRecordClick();
    void selectRuntimePage(RuntimePageKind page);
    void updateRuntimePageVisibility();

    std::optional<AudioEngine> m_audio;
    froggers_v2::FroggersV2AppCoreFacade m_facade;
    CvLaneHistoryStore m_cvLaneHistory;
    GlobalOscilloscopeDisplay m_globalOscilloscope;
    GlobalStripV2 m_globalStrip;
    PerformanceBandV2 m_performanceBand;
    PageCarouselComponent m_carousel;
    SequencerPanelComponent m_sequencerPanel;
    juce::TextButton m_play{"Play"};
    juce::TextButton m_stop{"Stop"};
    juce::TextButton m_fileButton{"File"};
    juce::TextButton m_audioButton{"Audio"};
    juce::TextButton m_midiButton{"MIDI"};
    RecordButton m_recordButton;
    FilePatchRuntimePageComponent m_filePatchPage;
    AudioRuntimePageComponent m_audioPage;
    ControllersRuntimePageComponent m_controllersPage;
    RuntimePageKind m_activeRuntimePage = RuntimePageKind::None;
    bool m_sequencerVisible = true;
    uint32_t m_lastUiVersion = 0;
    uint32_t m_lastModRoutesVersion = 0;
    desktop_v2::HostCallbackContext m_hostCallbacks;
    std::array<uint8_t, 128> m_qwertyHeldVelocity{};
};
