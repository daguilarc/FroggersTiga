#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "ui/CvLaneHistoryStore.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/EncoderRingComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

class AdsrPagePanel : public juce::Component
{
public:
    static constexpr uint8_t kAdsrPage = 6;

    AdsrPagePanel();

    void bindCore(froggers_v2::FroggersV2ControlCore* core);
    void bindLaneHistory(const CvLaneHistoryStore* store);
    void refresh();

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool encoderViewportShowsVerticalScrollbar() const;
    juce::Rectangle<int> modCellBoundsInPanel(int rowIndex) const;

    EncoderRingComponent& ringAt(size_t index) { return m_rings[index]; }
    const EncoderRingComponent& ringAt(size_t index) const { return m_rings[index]; }

private:
    static constexpr int kCellCapacity = froggers_v2::kUiSlots;

    int documentRowCount() const;
    bool detailGridOpen() const;
    void layoutRows();
    void layoutDetailGrid(juce::Rectangle<int> area);
    void pushTurn(uint8_t slot, float delta);
    void pushPress(uint8_t slot);
    void pushModDrillIn(uint8_t row);

    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    const CvLaneHistoryStore* m_laneHistory = nullptr;
    bool m_scrollBarsVisible = false;
    juce::Viewport m_encoderViewport;
    juce::Component m_encoderContent;
    std::array<juce::Label, kCellCapacity> m_rowLabels{};
    std::array<EncoderRingComponent, kCellCapacity> m_rings{};
};
