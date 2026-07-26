#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "DesktopHostIO.hpp"
#include "ui/EncoderRingComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class GlobalStripV2 : public juce::Component
{
public:
    GlobalStripV2();

    void bind(DesktopHostIO* host, froggers_v2::FroggersV2ControlCore* core);
    void refresh();

    // Test hook: drives the exact production path without a live JUCE click event.
    void triggerRandModsForTest() { pushRandMods(); }

    juce::Rectangle<int> crunchyRingBoundsForTest() const { return m_crunchyRing.getBounds(); }
    juce::Rectangle<int> scopeAllScenesBoundsForTest() const { return m_scopeAllScenes.getBounds(); }
    juce::Rectangle<int> scopeCurrentSceneBoundsForTest() const
    {
        return m_scopeCurrentScene.getBounds();
    }

    void resized() override;

private:
    void pushRandAll();
    void pushRandMods();

    DesktopHostIO* m_host = nullptr;
    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    bool m_allScenesScope = true;

    juce::TextButton m_randAll;
    juce::TextButton m_randMods;
    juce::TextButton m_randWaveforms;
    juce::TextButton m_randResample;
    juce::Label m_crunchyLabel;
    EncoderRingComponent m_crunchyRing;
    juce::ToggleButton m_scopeAllScenes{"All Scenes"};
    juce::ToggleButton m_scopeCurrentScene{"Current Scene"};
};
