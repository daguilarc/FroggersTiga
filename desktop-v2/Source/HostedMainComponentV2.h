#pragma once

#include "control/FroggersV2AppCoreFacade.hpp"
#include "DesktopV2HostCallbacks.hpp"
#include "runtime/HostedRuntimeStatusPanel.h"
#include "ui/PageCarouselComponent.hpp"
#include "ui/PerformanceBandV2.hpp"
#include "ui/GlobalStripV2.hpp"
#include "ui/GlobalOscilloscopeDisplay.hpp"
#include "ui/SequencerPanelComponent.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

// VST editor: no RecordExportCluster — audio export is standalone-only.
class HostedMainComponentV2 : public juce::Component,
                              private juce::Timer
{
public:
    explicit HostedMainComponentV2(froggers_v2::FroggersV2AppCoreFacade& facade);

    void setHostedProcessor(juce::AudioProcessor* processor);
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;

private:
    void timerCallback() override;
    void wireCallbacks();
    void pushSelectPage(uint8_t page);
    void updateShiftFromKeyboard();

    froggers_v2::FroggersV2AppCoreFacade& m_facade;
    GlobalOscilloscopeDisplay m_globalOscilloscope;
    GlobalStripV2 m_globalStrip;
    PerformanceBandV2 m_performanceBand;
    PageCarouselComponent m_carousel;
    SequencerPanelComponent m_sequencerPanel;
    HostedRuntimeStatusPanel m_hostedStatusPanel;
    bool m_sequencerVisible = true;
    uint32_t m_lastUiVersion = 0;
    uint32_t m_lastModRoutesVersion = 0;
    desktop_v2::HostCallbackContext m_hostCallbacks;
};
