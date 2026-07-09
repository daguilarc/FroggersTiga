#pragma once

#include "DesktopHostIO.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "ui/AdsrPagePanel.hpp"
#include "ui/SubmodulePagePanel.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class PageCarouselComponent : public juce::Component
{
public:
    PageCarouselComponent();

    void bindCore(froggers_v2::FroggersV2ControlCore* core);
    void bindHost(DesktopHostIO* host, froggers_v2::FroggersV2ControlCore* core);
    void setActivePage(uint8_t page);
    void selectPage(uint8_t page, bool fireCallback = true);
    void refresh();
    void setShiftHeld(bool held);

    SubmodulePagePanel& submodulePanel();
    AdsrPagePanel& adsrPanel();
    bool activePanelShowsVerticalScrollbar() const;
    juce::Rectangle<int> modCellBoundsInCarousel(int rowIndex) const;

    std::function<void(uint8_t page)> onPageChanged;
    std::function<void(uint8_t page)> onRandomize;
    std::function<void(uint8_t page)> onRandomizeMod;

    void resized() override;

private:
    void updateTitle();
    void advancePage(int delta);

    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    uint8_t m_page = 0;
    juce::Label m_moduleTitle;
    juce::TextButton m_prev{"<"};
    juce::TextButton m_next{">"};
    SubmodulePagePanel m_submodulePanel;
    AdsrPagePanel m_adsrPanel;
};
