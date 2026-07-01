#pragma once

#include "AudioEngine.h"
#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "ui/GlobalStripV2.hpp"
#include "ui/PageCarouselComponent.hpp"
#include "ui/PerformanceBandV2.hpp"
#include "ui/VcoEfScopeDisplay.hpp"
#include "ui/SequencerPanelComponent.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

#include <optional>

class HostedMainComponentV2 : public juce::Component,
                              private juce::Timer
{
public:
    HostedMainComponentV2(AudioEngine& audio,
                          froggers_v2::FroggersV2ControlCore& core,
                          froggers_v2::FroggersV2HostBridge& bridge);

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

    AudioEngine& m_audio;
    froggers_v2::FroggersV2ControlCore& m_core;
    froggers_v2::FroggersV2HostBridge& m_bridge;
    VcoEfScopeDisplay m_vcoEfScope;
    PerformanceBandV2 m_performanceBand;
    PageCarouselComponent m_carousel;
    GlobalStripV2 m_globalStrip;
    SequencerPanelComponent m_sequencerPanel;
    bool m_sequencerVisible = true;
    uint32_t m_lastUiVersion = 0;
    uint32_t m_lastModRoutesVersion = 0;
};
