#include "ui/PageCarouselComponent.hpp"

#include "ui/DesktopV2ChromeLayout.hpp"
#include "V2ParamDisplayNames.hpp"

PageCarouselComponent::PageCarouselComponent()
{
    m_moduleTitle.setJustificationType(juce::Justification::centred);
    m_moduleTitle.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    addAndMakeVisible(m_moduleTitle);

    m_prev.onClick = [this]() { advancePage(-1); };
    m_next.onClick = [this]() { advancePage(1); };
    addAndMakeVisible(m_prev);
    addAndMakeVisible(m_next);

    addChildComponent(m_submodulePanel);
    addChildComponent(m_adsrPanel);

    m_submodulePanel.onRandomize = [this](uint8_t page) {
        if (onRandomize)
        {
            onRandomize(page);
        }
    };
    m_submodulePanel.onRandomizeMod = [this](uint8_t page) {
        if (onRandomizeMod)
        {
            onRandomizeMod(page);
        }
    };

    m_adsrPanel.onRandomize = [this]() {
        if (onRandomize)
        {
            onRandomize(AdsrPagePanel::kAdsrPage);
        }
    };
    m_adsrPanel.onRandomizeMod = [this]() {
        if (onRandomizeMod)
        {
            onRandomizeMod(AdsrPagePanel::kAdsrPage);
        }
    };
}

void PageCarouselComponent::bindCore(froggers_v2::FroggersV2ControlCore* core)
{
    m_core = core;
    m_submodulePanel.bindCore(core);
    m_adsrPanel.bindCore(core);
    refresh();
}

void PageCarouselComponent::setActivePage(uint8_t page)
{
    selectPage(page);
}

void PageCarouselComponent::selectPage(uint8_t page)
{
    m_page = static_cast<uint8_t>(page % froggers_v2::kNumHostPages);
    updateTitle();

    const bool isAdsr = m_page == AdsrPagePanel::kAdsrPage;
    m_submodulePanel.setVisible(!isAdsr);
    m_adsrPanel.setVisible(isAdsr);
    if (!isAdsr)
    {
        m_submodulePanel.setPage(m_page);
    }

    if (onPageChanged)
    {
        onPageChanged(m_page);
    }
    refresh();
}

void PageCarouselComponent::advancePage(int delta)
{
    const int next = (static_cast<int>(m_page) + delta + froggers_v2::kNumHostPages) % froggers_v2::kNumHostPages;
    selectPage(static_cast<uint8_t>(next));
}

void PageCarouselComponent::updateTitle()
{
    const juce::String title =
        juce::String("Module: ") + V2ParamDisplayNames::forHostPage(m_page);
    m_moduleTitle.setText(title, juce::dontSendNotification);
}

void PageCarouselComponent::refresh()
{
    if (m_page == AdsrPagePanel::kAdsrPage)
    {
        m_adsrPanel.refresh();
    }
    else
    {
        m_submodulePanel.refresh();
    }
}

void PageCarouselComponent::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(DesktopV2ChromeLayout::kCarouselHeaderH);

    const int arrowW = DesktopV2ChromeLayout::kArrowButtonSize;
    const int gap = DesktopV2ChromeLayout::kSectionGap;
    const int titleW =
        juce::jmax(DesktopV2ChromeLayout::gridPx(12),
                   m_moduleTitle.getFont().getStringWidth(m_moduleTitle.getText())
                       + DesktopV2ChromeLayout::gridPx(1));
    const int groupW = arrowW + gap + titleW + gap + arrowW;
    auto group = header.withSizeKeepingCentre(groupW, header.getHeight());

    m_prev.setBounds(group.removeFromLeft(arrowW));
    group.removeFromLeft(gap);
    m_next.setBounds(group.removeFromRight(arrowW));
    group.removeFromRight(gap);
    m_moduleTitle.setBounds(group);
    m_moduleTitle.setJustificationType(juce::Justification::centred);

    m_submodulePanel.setBounds(area);
    m_adsrPanel.setBounds(area);
}
