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

#include <cstdio>
#include <vector>

namespace
{
bool intersects(const juce::Rectangle<int>& a, const juce::Rectangle<int>& b)
{
    return a.intersects(b);
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
        m_sequencer.bind(&m_sequencerState, &m_core, &m_bridge);
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

private:
    DesktopHostIO m_host;
    froggers_v2::FroggersV2ControlCore m_core;
    froggers_v2::FroggersV2HostBridge m_bridge{m_core, m_host};
    SequencerState m_sequencerState;
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
    shell.carousel().selectPage(0, false);
    shell.resized();
    if (shell.carousel().activePanelShowsVerticalScrollbar())
    {
        std::printf("FAIL: Audio page shows vertical scrollbar at default size\n");
        return false;
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
    if (!test_sequencer_steps_visible(shell))
    {
        return 1;
    }
    if (!test_direction_speed_not_truncated(shell))
    {
        return 1;
    }

    std::printf("PASS: LayoutBounds_test\n");
    return 0;
}
