#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "DesktopHostIO.hpp"
#include "ui/EncoderRingComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class GlobalStripV2 : public juce::Component
{
public:
    GlobalStripV2();

    void bind(DesktopHostIO* host, froggers_v2::FroggersV2ControlCore* core);
    void setShiftHeld(bool held);
    void refresh();

    std::function<uint8_t()> resolveRandSeqScope;

    // Exposes the global-command band's All Steps / Current Step scope radio
    // state so the sequencer panel's Rand-seq dice can resolve Pattern scope
    // when All Steps is selected (desktop-v2-operator-truth-repair Packet 6
    // carryover C1: the panel-local toggle Packet 5 removed left the dice
    // stuck at Step scope; this is the single authority it now reads).
    bool allStepsScope() const { return m_allStepsScope; }

    // Test hooks: mirror SequencerPanelComponent's *ForTest convention by
    // driving the exact production path without a live JUCE click event.
    void triggerRandModsForTest() { pushRandMods(); }
    void setAllStepsScopeForTest(bool allSteps)
    {
        m_allStepsScope = allSteps;
        m_scopeAllSteps.setToggleState(allSteps, juce::dontSendNotification);
        m_scopeCurrentStep.setToggleState(!allSteps, juce::dontSendNotification);
    }

    void resized() override;

private:
    void pushShift(bool held);
    void pushRandAll();
    void pushRandMods();

    DesktopHostIO* m_host = nullptr;
    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    bool m_allScenesScope = true;
    bool m_allStepsScope = false;

    juce::TextButton m_randAll;
    juce::TextButton m_randMods;
    juce::TextButton m_randWaveforms;
    juce::TextButton m_randResample;
    juce::Label m_crunchyLabel;
    EncoderRingComponent m_crunchyRing;
    juce::ToggleButton m_shift{"Shift"};
    juce::ToggleButton m_scopeAllScenes{"All Scenes"};
    juce::ToggleButton m_scopeCurrentScene{"Current Scene"};
    juce::ToggleButton m_scopeAllSteps{"All Steps"};
    juce::ToggleButton m_scopeCurrentStep{"Current Step"};
};
