#include "control/FroggersV2ControlCore.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/GlobalOscilloscopeDisplay.hpp"
#include "ui/GlobalStripV2.hpp"
#include "ui/PageCarouselComponent.hpp"
#include "ui/PerformanceBandV2.hpp"

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

bool textFitsWidth(const juce::Font& font, const juce::String& text, int width, int chromePad)
{
    return font.getStringWidthFloat(text) + static_cast<float>(chromePad)
        <= static_cast<float>(width) + 0.5f;
}

bool anyBoundsOverlap(const std::vector<juce::Rectangle<int>>& bounds)
{
    for (size_t i = 0; i < bounds.size(); ++i)
    {
        for (size_t j = i + 1; j < bounds.size(); ++j)
        {
            if (bounds[i].intersects(bounds[j]))
            {
                return true;
            }
        }
    }
    return false;
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

        addAndMakeVisible(m_globalOscilloscope);
        addAndMakeVisible(m_globalStrip);
        addAndMakeVisible(m_performanceBand);
        addAndMakeVisible(m_carousel);

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
        m_performanceBand.setBounds(area.removeFromTop(kPerformanceBandH));
        area.removeFromTop(kSectionGap);
        m_carousel.setBounds(area);
    }

    juce::Rectangle<int> globalStripBoundsInRoot() const
    {
        return getLocalArea(&m_globalStrip, m_globalStrip.getLocalBounds());
    }

    GlobalStripV2& globalStrip()
    {
        return m_globalStrip;
    }

    PerformanceBandV2& performanceBand()
    {
        return m_performanceBand;
    }

    PageCarouselComponent& carousel()
    {
        return m_carousel;
    }

    froggers_v2::FroggersV2ControlCore& core()
    {
        return m_core;
    }

private:
    DesktopHostIO m_host;
    froggers_v2::FroggersV2ControlCore m_core;
    GlobalOscilloscopeDisplay m_globalOscilloscope;
    GlobalStripV2 m_globalStrip;
    PerformanceBandV2 m_performanceBand;
    PageCarouselComponent m_carousel;
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

bool test_global_strip_grid_at_1280(LayoutTestShell& shell)
{
    // Packet 17.4 / D14 + Packet 18: honest two-row global strip at default
    // 1280x920 — scope radios on dedicated non-overlapping columns, Crunchy
    // ring fills to the right edge (Shift removed), scope label widths fit
    // full text. Manual visual QA remains UNVALIDATED (Packet 14).
    using namespace DesktopV2ChromeLayout;
    shell.setSize(kDefaultWidth, kDefaultHeight);
    shell.resized();

    GlobalStripV2& strip = shell.globalStrip();
    const juce::Rectangle<int> stripLocal = strip.getLocalBounds();
    if (strip.crunchyRingBoundsForTest().getRight() != stripLocal.getRight())
    {
        std::printf(
            "FAIL: Crunchy ring right edge %d leaves dead space before strip right %d\n",
            strip.crunchyRingBoundsForTest().getRight(),
            stripLocal.getRight());
        return false;
    }

    const juce::Rectangle<int> scopes[] = {
        strip.scopeAllScenesBoundsForTest(),
        strip.scopeCurrentSceneBoundsForTest(),
    };
    const int scopeMins[] = {
        kGlobalScopeSceneAllW,
        kGlobalScopeSceneCurrentW,
    };
    const char* scopeLabels[] = {
        "All Scenes",
        "Current Scene",
    };
    std::vector<juce::Rectangle<int>> scopeBounds(scopes, scopes + 2);
    if (anyBoundsOverlap(scopeBounds))
    {
        std::printf("FAIL: global strip scope radios overlap at 1280x920\n");
        return false;
    }

    const juce::Font scopeFont = [](juce::LookAndFeel& laf) {
        juce::TextButton probe("probe");
        return laf.getTextButtonFont(probe, DesktopV2ChromeLayout::kTextButtonH);
    }(shell.getLookAndFeel());
    // ToggleButton chrome is wider than TextButton; pad covers tick + margins.
    constexpr int kToggleChromePad = 28;
    for (int i = 0; i < 2; ++i)
    {
        if (scopes[i].getWidth() < scopeMins[i])
        {
            std::printf("FAIL: scope[%d] width %d < minimum %d\n",
                        i,
                        scopes[i].getWidth(),
                        scopeMins[i]);
            return false;
        }
        if (!textFitsWidth(scopeFont, scopeLabels[i], scopes[i].getWidth(), kToggleChromePad))
        {
            std::printf("FAIL: scope label '%s' does not fit width %d\n",
                        scopeLabels[i],
                        scopes[i].getWidth());
            return false;
        }
    }

    std::vector<juce::Rectangle<int>> stripChildren;
    for (int i = 0; i < strip.getNumChildComponents(); ++i)
    {
        juce::Component* child = strip.getChildComponent(i);
        if (child != nullptr && !child->getBounds().isEmpty())
        {
            stripChildren.push_back(child->getBounds());
        }
    }
    if (anyBoundsOverlap(stripChildren))
    {
        std::printf("FAIL: global strip child controls overlap at 1280x920\n");
        return false;
    }
    return true;
}

bool test_performance_band_grid_at_1280(LayoutTestShell& shell)
{
    // Packet 17.4 / D14: performance band labeled controls do not overlap and
    // scene/marbles labels fit without ellipsis at 1280x920. Manual
    // visual QA remains UNVALIDATED (Packet 14).
    using namespace DesktopV2ChromeLayout;
    shell.setSize(kDefaultWidth, kDefaultHeight);
    shell.resized();

    PerformanceBandV2& band = shell.performanceBand();
    band.refresh();

    const juce::Rectangle<int> labeled[] = {
        band.sceneLabelBoundsForTest(),
        band.blendLabelLBoundsForTest(),
        band.blendLabelRBoundsForTest(),
        band.marblesLabel1BoundsForTest(),
        band.marblesLabel2BoundsForTest(),
        band.scene1BoundsForTest(),
        band.sceneBlendBoundsForTest(),
    };
    std::vector<juce::Rectangle<int>> labeledBounds(labeled, labeled + 7);
    if (anyBoundsOverlap(labeledBounds))
    {
        std::printf("FAIL: performance band controls overlap at 1280x920\n");
        return false;
    }

    juce::Label probeLabel;
    const juce::Font labelFont = shell.getLookAndFeel().getLabelFont(probeLabel);
    // PerformanceBandV2 sets marbles labels to 9pt explicitly.
    const juce::Font marblesFont(juce::FontOptions(9.0f));
    constexpr int kLabelChromePad = 4;

    struct LabeledWidthCheck
    {
        juce::String text;
        juce::Rectangle<int> bounds;
        int chromePad;
        const juce::Font* font;
    };
    const LabeledWidthCheck checks[] = {
        {"Scene", band.sceneLabelBoundsForTest(), kLabelChromePad, &labelFont},
        {"S1", band.blendLabelLBoundsForTest(), kLabelChromePad, &labelFont},
        {"S2", band.blendLabelRBoundsForTest(), kLabelChromePad, &labelFont},
        {"Random S&H 1", band.marblesLabel1BoundsForTest(), kLabelChromePad, &marblesFont},
        {"Random S&H 2", band.marblesLabel2BoundsForTest(), kLabelChromePad, &marblesFont},
    };

    for (const LabeledWidthCheck& check : checks)
    {
        if (!textFitsWidth(*check.font, check.text, check.bounds.getWidth(), check.chromePad))
        {
            std::printf("FAIL: performance label '%s' does not fit width %d\n",
                        check.text.toRawUTF8(),
                        check.bounds.getWidth());
            return false;
        }
    }

    if (band.marblesLabel1BoundsForTest().getWidth() < kPerfMarblesColW)
    {
        std::printf("FAIL: performance band label columns narrower than minima\n");
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
    if (!test_global_strip_grid_at_1280(shell))
    {
        return 1;
    }
    if (!test_performance_band_grid_at_1280(shell))
    {
        return 1;
    }

    std::printf("PASS: LayoutBounds_test\n");
    return 0;
}
