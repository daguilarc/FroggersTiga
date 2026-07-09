#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "SequencerState.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/GlobalOscilloscopeDisplay.hpp"
#include "ui/GlobalStripV2.hpp"
#include "ui/PageCarouselComponent.hpp"
#include "ui/PerformanceBandV2.hpp"
#include "ui/SequencerPanelComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
bool intersects(const juce::Rectangle<int>& a, const juce::Rectangle<int>& b)
{
    return a.intersects(b);
}

bool nearlyEqual(float a, float b, float eps = 1.0e-4f)
{
    return std::fabs(a - b) <= eps;
}

class LayoutTestShell : public juce::Component
{
public:
    LayoutTestShell()
    {
        m_globalOscilloscope.bindHost(&m_host);
        m_globalStrip.bind(&m_host, &m_core);
        m_carousel.bindHost(&m_host, &m_core);
        m_performanceBand.bind(&m_core);
        m_performanceBand.bindHost(&m_host);
        // Bind the panel to the SAME SequencerState instance the bridge
        // operates on (m_host.m_sequencer), not a disconnected standalone
        // one -- otherwise captureLiveToSequencerStep/recallSequencerStep
        // (which go through m_bridge -> m_host.m_sequencer) would silently
        // write into a different object than the one this shell's tests
        // inspect.
        m_sequencer.bind(&m_host.m_sequencer, &m_core, &m_bridge);
        m_globalStrip.resolveRandSeqScope = [this]() { return m_sequencer.getRandSeqScope(); };

        addAndMakeVisible(m_globalOscilloscope);
        addAndMakeVisible(m_globalStrip);
        addAndMakeVisible(m_performanceBand);
        addAndMakeVisible(m_carousel);
        addAndMakeVisible(m_sequencer);

        m_carousel.selectPage(0, false);
        setSize(DesktopV2ChromeLayout::kDefaultWidth, DesktopV2ChromeLayout::kDefaultHeight);
    }

    void resized() override
    {
        using namespace DesktopV2ChromeLayout;
        auto area = getLocalBounds().reduced(kChromePad);
        auto transport = area.removeFromTop(kTransportRowH);
        m_globalOscilloscope.setBounds(transport);
        area.removeFromTop(kSectionGap);
        m_globalStrip.setBounds(area.removeFromTop(kGlobalCommandBandH));
        area.removeFromTop(kSectionGap);
        m_sequencer.setBounds(area.removeFromBottom(kSequencerH));
        area.removeFromBottom(kSectionGap);
        m_performanceBand.setBounds(area.removeFromTop(kPerformanceBandH));
        area.removeFromTop(kSectionGap);
        m_carousel.setBounds(area);
    }

    juce::Rectangle<int> globalStripBoundsInRoot() const
    {
        return getLocalArea(&m_globalStrip, m_globalStrip.getLocalBounds());
    }

    PageCarouselComponent& carousel()
    {
        return m_carousel;
    }

    SequencerPanelComponent& sequencer()
    {
        return m_sequencer;
    }

    SequencerState& sequencerState()
    {
        return m_host.m_sequencer;
    }

    froggers_v2::FroggersV2ControlCore& core()
    {
        return m_core;
    }

private:
    DesktopHostIO m_host;
    froggers_v2::FroggersV2ControlCore m_core;
    froggers_v2::FroggersV2HostBridge m_bridge{m_core, m_host};
    GlobalOscilloscopeDisplay m_globalOscilloscope;
    GlobalStripV2 m_globalStrip;
    PerformanceBandV2 m_performanceBand;
    PageCarouselComponent m_carousel;
    SequencerPanelComponent m_sequencer;
};

bool test_no_global_strip_mod_overlap(LayoutTestShell& shell)
{
    shell.resized();
    shell.carousel().selectPage(0, false);
    shell.resized();

    const juce::Rectangle<int> globalBounds = shell.globalStripBoundsInRoot();
    for (int row = 0; row < 8; ++row)
    {
        const juce::Rectangle<int> modBounds =
            shell.getLocalArea(&shell.carousel(), shell.carousel().modCellBoundsInCarousel(row));
        if (modBounds.isEmpty())
        {
            continue;
        }
        if (intersects(globalBounds, modBounds))
        {
            std::printf("FAIL: global strip intersects mod cell row %d\n", row);
            return false;
        }
    }
    return true;
}

bool test_audio_page_no_scrollbar(LayoutTestShell& shell)
{
    // Packet 1.4(b): the Audio module page (page 0) must fit its encoder/mod
    // rows within the carousel viewport at the default 1280x920 chrome size
    // without a vertical scrollbar.
    shell.carousel().selectPage(0, false);
    shell.resized();
    if (shell.carousel().activePanelShowsVerticalScrollbar())
    {
        std::printf("FAIL: Audio page shows vertical scrollbar at default size\n");
        return false;
    }
    return true;
}

bool test_mod_cell_width_capped(LayoutTestShell& shell)
{
    // Packet 1.4(a): moduleRowColumns().modW must never exceed kModCellW —
    // this is the authoritative (non-advisory) check for the modW-expansion
    // defect that check_desktop_v2_operator_truth.sh only greps for.
    static constexpr uint8_t kPagesToCheck[] = {0, 1};
    for (const uint8_t page : kPagesToCheck)
    {
        shell.carousel().selectPage(page, false);
        shell.resized();
        for (int row = 0; row < DesktopV2ChromeLayout::kVisibleEncoderSlots; ++row)
        {
            const juce::Rectangle<int> modBounds = shell.carousel().modCellBoundsInCarousel(row);
            if (modBounds.isEmpty())
            {
                continue;
            }
            if (modBounds.getWidth() > DesktopV2ChromeLayout::kModCellW)
            {
                std::printf("FAIL: page %u mod cell row %d width %d exceeds kModCellW %d\n",
                            static_cast<unsigned>(page),
                            row,
                            modBounds.getWidth(),
                            DesktopV2ChromeLayout::kModCellW);
                return false;
            }
        }
    }
    return true;
}

bool test_sequencer_steps_visible(LayoutTestShell& shell)
{
    const juce::Rectangle<int> sequencerBounds =
        shell.getLocalArea(&shell.sequencer(), shell.sequencer().getLocalBounds());
    for (int i = 0; i < SequencerState::kSlotCount; ++i)
    {
        const juce::Rectangle<int> stepBounds =
            shell.getLocalArea(&shell.sequencer(), shell.sequencer().getStepBounds(i));
        if (!sequencerBounds.contains(stepBounds))
        {
            std::printf("FAIL: sequencer step %d not fully visible\n", i);
            return false;
        }
    }
    return true;
}

bool test_direction_speed_not_truncated(LayoutTestShell& shell)
{
    const juce::Rectangle<int> directionBounds =
        shell.getLocalArea(&shell.sequencer(), shell.sequencer().directionButtonBounds());
    const juce::Rectangle<int> speedBounds =
        shell.getLocalArea(&shell.sequencer(), shell.sequencer().speedButtonBounds());
    if (directionBounds.getWidth() < DesktopV2ChromeLayout::gridPx(4))
    {
        std::printf("FAIL: direction button truncated\n");
        return false;
    }
    if (speedBounds.getWidth() < DesktopV2ChromeLayout::gridPx(5))
    {
        std::printf("FAIL: speed button truncated\n");
        return false;
    }
    return true;
}

bool test_no_duplicate_step_scope_control(LayoutTestShell& shell)
{
    // desktop-v2-sequencer-operator-loop "Sequencer scope UI is not
    // duplicated" / desktop-v2-sequencing "Sequencer toolbar omits duplicate
    // global randomization": the sequencer toolbar must not render its own
    // All Steps / Current Step radio group, and the toolbar must still fit
    // (with room to spare) inside the fixed sequencer height now that the
    // duplicate control is gone.
    juce::Rectangle<int> maxStepBottom;
    for (int i = 0; i < SequencerState::kSlotCount; ++i)
    {
        maxStepBottom = maxStepBottom.getUnion(shell.sequencer().getStepBounds(i));
    }
    if (maxStepBottom.getBottom() > DesktopV2ChromeLayout::kSequencerH)
    {
        std::printf(
            "FAIL: step row bottom %d exceeds kSequencerH %d\n",
            maxStepBottom.getBottom(),
            DesktopV2ChromeLayout::kSequencerH);
        return false;
    }
    const juce::Rectangle<int> dirBounds = shell.sequencer().directionButtonBounds();
    if (dirBounds.getBottom() > DesktopV2ChromeLayout::kSequencerH)
    {
        std::printf("FAIL: toolbar row extends past kSequencerH (direction button)\n");
        return false;
    }
    for (int i = 0; i < shell.sequencer().getNumChildComponents(); ++i)
    {
        if (auto* toggle = dynamic_cast<juce::ToggleButton*>(shell.sequencer().getChildComponent(i)))
        {
            const juce::String text = toggle->getButtonText();
            if (text == "All steps" || text == "Step")
            {
                std::printf(
                    "FAIL: sequencer toolbar still renders a duplicate scope toggle (%s)\n",
                    text.toRawUTF8());
                return false;
            }
        }
    }
    return true;
}

bool test_write_seq_click_writes_into_clicked_step(LayoutTestShell& shell)
{
    // desktop-v2-sequencer-operator-loop "Click step while armed writes
    // snapshot": clicking a step while Write Seq. is armed and transport is
    // stopped writes the current live state into that step -- both when
    // navigating to a different step and when re-clicking the already
    // selected one.
    SequencerState& seq = shell.sequencerState();
    froggers_v2::FroggersV2ControlCore& core = shell.core();

    seq.m_playing = false;
    seq.m_writeSeqArm = true;
    seq.m_editStep = 0;
    for (auto& slot : seq.m_slots)
    {
        slot = {};
    }

    core.bus().push(froggers_v2::MessageIn::ParamTurn(0, 0, 9.0f));
    core.processBus();
    const float liveScene = core.effectiveRow(0, 0).sceneLeft;

    // Click a different step (5) while armed: it becomes written with the
    // current live state, and the departed step (0) is saved too.
    shell.sequencer().setEditStep(5);

    if (!seq.m_slots[5].written)
    {
        std::printf("FAIL: clicking step 5 while armed did not write it\n");
        return false;
    }
    if (!nearlyEqual(seq.m_slots[5].payload.sceneCenter[0][0][0], liveScene))
    {
        std::printf("FAIL: step 5 snapshot did not capture live state on click\n");
        return false;
    }
    if (!seq.m_slots[0].written)
    {
        std::printf("FAIL: departed step 0 was not saved on navigate\n");
        return false;
    }

    // Re-clicking the already-selected step (5) while armed writes it again.
    core.bus().push(froggers_v2::MessageIn::ParamTurn(0, 0, -3.0f));
    core.processBus();
    const float liveSceneAfterTurn = core.effectiveRow(0, 0).sceneLeft;
    shell.sequencer().setEditStep(5);
    if (!nearlyEqual(seq.m_slots[5].payload.sceneCenter[0][0][0], liveSceneAfterTurn))
    {
        std::printf("FAIL: clicking the already-selected step did not re-write it\n");
        return false;
    }
    return true;
}

bool test_long_press_cancel_before_threshold(LayoutTestShell& shell)
{
    // desktop-v2-sequencing "Short hold does not clear a written step":
    // releasing before the long-press threshold cancels the pending clear.
    SequencerState& seq = shell.sequencerState();
    seq.m_slots[3] = {};
    seq.m_slots[3].written = true;
    seq.m_slots[3].payload.gate = true;

    shell.sequencer().beginLongPressForTest(3);
    if (!shell.sequencer().isLongPressPendingForTest())
    {
        std::printf("FAIL: beginLongPress did not start a pending clear\n");
        return false;
    }
    shell.sequencer().cancelLongPressForTest(3);
    if (shell.sequencer().isLongPressPendingForTest())
    {
        std::printf("FAIL: cancelLongPress did not stop the pending clear timer\n");
        return false;
    }
    if (!seq.m_slots[3].written)
    {
        std::printf("FAIL: short hold cleared a written step\n");
        return false;
    }
    return true;
}

bool test_long_press_fires_clear_after_threshold(LayoutTestShell& shell)
{
    // desktop-v2-sequencing "Long press clears a written step": once the
    // long-press threshold is reached, the step is marked unwritten and its
    // saved snapshot/lock values are cleared.
    SequencerState& seq = shell.sequencerState();
    seq.m_slots[4] = {};
    seq.m_slots[4].written = true;
    seq.m_slots[4].payload.gate = true;

    shell.sequencer().beginLongPressForTest(4);
    shell.sequencer().fireLongPressForTest();
    if (seq.m_slots[4].written)
    {
        std::printf("FAIL: long press did not clear the step\n");
        return false;
    }
    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    LayoutTestShell shell;

    if (!test_no_global_strip_mod_overlap(shell))
    {
        return 1;
    }
    if (!test_audio_page_no_scrollbar(shell))
    {
        return 1;
    }
    if (!test_mod_cell_width_capped(shell))
    {
        return 1;
    }
    if (!test_sequencer_steps_visible(shell))
    {
        return 1;
    }
    if (!test_direction_speed_not_truncated(shell))
    {
        return 1;
    }
    if (!test_no_duplicate_step_scope_control(shell))
    {
        return 1;
    }
    if (!test_write_seq_click_writes_into_clicked_step(shell))
    {
        return 1;
    }
    if (!test_long_press_cancel_before_threshold(shell))
    {
        return 1;
    }
    if (!test_long_press_fires_clear_after_threshold(shell))
    {
        return 1;
    }

    std::printf("PASS: LayoutBounds_test\n");
    return 0;
}
