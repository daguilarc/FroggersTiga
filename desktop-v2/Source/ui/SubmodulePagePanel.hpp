#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/EncoderRingComponent.hpp"
#include "ui/ModLanePicker.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

class SubmodulePagePanel : public juce::Component
{
public:
    SubmodulePagePanel();

    void setPage(uint8_t page);
    void bindCore(froggers_v2::FroggersV2ControlCore* core);
    void refresh();

    std::function<void(uint8_t page)> onRandomize;
    std::function<void(uint8_t page)> onRandomizeMod;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool encoderViewportShowsVerticalScrollbar() const;
    juce::Rectangle<int> modCellBoundsInPanel(int rowIndex) const;

private:
    uint8_t rowCountForPage(uint8_t page) const;
    int documentRowCount() const;
    void layoutRows();
    void wireEncoderCallbacks();
    void pushTurn(uint8_t slot, float delta);
    void pushPress(uint8_t slot);
    void pushModAssign(uint8_t row, uint8_t internalSource);

    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    uint8_t m_page = 0;
    bool m_scrollBarsVisible = false;
    juce::TextButton m_randomize{"Randomize"};
    juce::TextButton m_randomizeMod{"Randmod"};
    juce::Viewport m_encoderViewport;
    juce::Component m_encoderContent;
    juce::Viewport m_modColumnViewport;
    juce::Component m_modColumnContent;
    std::array<juce::Label, DesktopV2ChromeLayout::kVisibleEncoderSlots> m_rowLabels{};
    std::array<EncoderRingComponent, DesktopV2ChromeLayout::kVisibleEncoderSlots> m_rings{};
    std::array<ModLanePicker, DesktopV2ChromeLayout::kVisibleEncoderSlots> m_modCells{};
};
