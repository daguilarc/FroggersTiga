#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "manifest/FroggersV2AppManifest.hpp"
#include "SequencerState.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstdint>
#include <functional>

class SequencerPanelComponent : public juce::Component,
                                private juce::Timer
{
public:
    SequencerPanelComponent();

    void bind(SequencerState* sequencer,
              froggers_v2::FroggersV2ControlCore* core,
              froggers_v2::FroggersV2HostBridge* bridge);
    void refresh();
    uint8_t getRandSeqScope() const;

    // Queried by pushDiceRand() to resolve whether the Rand-seq dice should
    // target Pattern scope (every written step) or Step scope (just the
    // current/playhead step). Wired by the owner to GlobalStripV2's
    // allStepsScope(), the global-command band's single scope authority
    // (desktop-v2-operator-truth-repair Packet 6 carryover C1).
    std::function<bool()> resolveAllStepsScope;

    juce::Rectangle<int> getStepBounds(int stepIndex) const;
    juce::Rectangle<int> directionButtonBounds() const;
    juce::Rectangle<int> speedButtonBounds() const;

    // Step-click orchestration (write-on-click when Write Seq. is armed and
    // stopped; navigate-and-recall otherwise). Public so a StepCell's click
    // handler can reach it, and so tests can drive the exact production path
    // without simulating real JUCE mouse events.
    void setEditStep(int step);

    // Long-press-to-clear test hooks: mirror what a StepCell's mouseDown /
    // mouseUp send to the panel, without needing a live JUCE event/timer
    // loop in a unit test.
    void beginLongPressForTest(int step);
    void cancelLongPressForTest(int step);
    void fireLongPressForTest();
    bool isLongPressPendingForTest() const;

    // Rand-seq dice test hook: mirrors the other *ForTest members above by
    // driving the exact production path (m_dice.onClick) without needing a
    // live JUCE click event.
    void pushDiceRandForTest();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class StepCell : public juce::TextButton
    {
    public:
        int stepIndex = -1;
        std::function<void(int, const juce::MouseEvent&)> onStepMouseDown;
        std::function<void(int)> onStepMouseUp;
        std::function<void(int)> onStepDoubleClick;
        std::function<void(int)> onStepClick;

        void setCaptureFlash(bool active)
        {
            if (m_captureFlash != active)
            {
                m_captureFlash = active;
                repaint();
            }
        }

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            juce::TextButton::paintButton(g, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            if (!m_captureFlash)
            {
                return;
            }
            g.setColour(juce::Colour(0xccffd33d));
            g.drawRect(getLocalBounds().reduced(1), 2);
        }

        void mouseDown(const juce::MouseEvent& event) override
        {
            if (onStepMouseDown)
            {
                onStepMouseDown(stepIndex, event);
            }
        }

        void mouseUp(const juce::MouseEvent& event) override
        {
            // Always cancel any pending long-press first: releasing before
            // the long-press threshold SHALL cancel the clear, regardless of
            // click type (single, double, or context-menu release).
            if (onStepMouseUp)
            {
                onStepMouseUp(stepIndex);
            }
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

    private:
        bool m_captureFlash = false;
    };

    void timerCallback() override;
    void toggleStepGate(int step);
    void clearStep(int step);
    void showStepContextMenu(int step);
    void pushResetStep(int step);
    void pushRandSequencerStep(int step, uint8_t scope);
    void pushDiceRand();
    void triggerCaptureFlash(int step);
    void cycleDirection();
    void cycleSpeed();
    void beginLongPress(int step);
    void cancelLongPress(int step);
    void layoutClockTransport(juce::Rectangle<int> row, int gap, int& x);
    void layoutNavRandScope(juce::Rectangle<int> row, int gap, int x);
    static int sequencerToolbarMinWidth();
    static juce::Drawable* makeChevron(bool left);
    static juce::Drawable* makeDiceFace();
    static const char* directionLabel(uint8_t direction);
    static const char* speedLabel(uint8_t speedChoice);

    SequencerState* m_sequencer = nullptr;
    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    froggers_v2::FroggersV2HostBridge* m_bridge = nullptr;
    int m_captureFlashStep = -1;
    int m_longPressStep = -1;
    bool m_longPressFired = false;
    juce::uint32 m_captureFlashMs = 0;

    juce::Label m_bpmLabel;
    juce::Slider m_bpm;
    juce::TextButton m_directionBtn;
    juce::TextButton m_speedBtn;
    juce::TextButton m_seqPlay{"Start Sequence"};
    juce::ToggleButton m_writeSeq{"Write Seq."};
    juce::DrawableButton m_prevStep{
        "prev",
        juce::DrawableButton::ImageFitted};
    juce::DrawableButton m_nextStep{
        "next",
        juce::DrawableButton::ImageFitted};
    juce::DrawableButton m_dice{
        "dice",
        juce::DrawableButton::ImageFitted};
    juce::Label m_randSeqLabel;
    std::array<StepCell, SequencerState::kSlotCount> m_steps{};
};
