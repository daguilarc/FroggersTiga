#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/EncoderRingComponent.hpp"
#include "ui/ModLanePicker.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

class AdsrPagePanel : public juce::Component
{
public:
    static constexpr uint8_t kAdsrPage = 6;

    AdsrPagePanel();

    void bindCore(froggers_v2::FroggersV2ControlCore* core);
    void refresh();

    // onRandomize / onRandomizeMod: the per-page Randomize/Randmod header
    // buttons that used to invoke these hooks are removed (desktop-v2
    // "Randomization affordances are not duplicated" -- global Rand All /
    // Rand Mods in the command band are the single surface). The hooks stay
    // declared because PageCarouselComponent still assigns them; nothing in
    // this panel invokes them anymore. Follow-up scope: retire the hooks and
    // their PageCarouselComponent plumbing once that file is in a packet's
    // write scope.
    std::function<void()> onRandomize;
    std::function<void()> onRandomizeMod;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool encoderViewportShowsVerticalScrollbar() const;
    juce::Rectangle<int> modCellBoundsInPanel(int rowIndex) const;

private:
    static constexpr int kCellCapacity = froggers_v2::kUiSlots;

    int documentRowCount() const;
    bool detailGridOpen() const;
    void layoutRows();
    void layoutDetailGrid(juce::Rectangle<int> area);
    void pushTurn(uint8_t slot, float delta);
    void pushPress(uint8_t slot);
    void pushModAssign(uint8_t row, uint8_t internalSource);

    froggers_v2::FroggersV2ControlCore* m_core = nullptr;
    bool m_scrollBarsVisible = false;
    juce::Viewport m_encoderViewport;
    juce::Component m_encoderContent;
    juce::Viewport m_modColumnViewport;
    juce::Component m_modColumnContent;
    std::array<juce::Label, kCellCapacity> m_rowLabels{};
    std::array<EncoderRingComponent, kCellCapacity> m_rings{};
    std::array<ModLanePicker, kCellCapacity> m_modCells{};
};
