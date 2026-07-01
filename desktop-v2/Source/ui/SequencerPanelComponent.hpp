#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "SequencerState.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstdint>
#include <functional>

class SequencerPanelComponent : public juce::Component
{
public:
    SequencerPanelComponent();

    void bind(SequencerState* sequencer,
              froggers_v2::FroggersV2ControlCore* core,
              froggers_v2::FroggersV2HostBridge* bridge);
    void refresh();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class StepCell : public juce::TextButton
    {
    public:
        int stepIndex = -1;
        std::function<void(int, const juce::MouseEvent&)> onStepMouseDown;
        std::function<void(int)> onStepDoubleClick;
        std::function<void(int)> onStepClick;

        void mouseDown(const juce::MouseEvent& event) override
        {
            if (onStepMouseDown)
            {
                onStepMouseDown(stepIndex, event);
            }
        }

        void mouseUp(const juce::MouseEvent& event) override
        {
            if (event.mods.isPopupMenu() || event.getNumberOfClicks() >= 2)
            {
                return;
            }
            if (onStepClick)
            {
                onStepClick(stepIndex);
            }
        }

        void mouseDoubleClick(const juce::MouseEvent& event) override
        {
            juce::ignoreUnused(event);
            if (onStepDoubleClick)
            {
                onStepDoubleClick(stepIndex);
            }
        }
    };

    void setEditStep(int step);
    void toggleStepGate(int step);
    void showStepContextMenu(int step);
    void pushResetStep(int step);
    void pushRandSequencerStep(int step, uint8_t scope);
    void pushDiceRand();
    static juce::Drawable* makeChevron(bool left);
    static juce::Drawable* makeDiceFace();

    SequencerState* m_sequencer = nullptr;
    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    froggers_v2::FroggersV2HostBridge* m_bridge = nullptr;
    bool m_patternScope = false;

    juce::Label m_title;
    juce::DrawableButton m_prevStep{
        "prev",
        juce::DrawableButton::ImageFitted};
    juce::DrawableButton m_nextStep{
        "next",
        juce::DrawableButton::ImageFitted};
    juce::DrawableButton m_dice{
        "dice",
        juce::DrawableButton::ImageFitted};
    juce::ToggleButton m_scopeStep{"Step"};
    juce::ToggleButton m_scopePattern{"Pattern"};
    std::array<StepCell, SequencerState::kMaxSteps> m_steps{};
};
