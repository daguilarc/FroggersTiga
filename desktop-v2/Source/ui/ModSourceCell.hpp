#pragma once

#include "control/FroggersV2ControlCore.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>

class ModSourceCell : public juce::Component
{
public:
    ModSourceCell();

    void setRow(uint8_t row);
    void setAssignedSource(uint8_t internalSourceIndex);
    void refresh();

    std::function<void(uint8_t row, uint8_t internalSourceIndex)> onAssign;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void rebuildMenu();
    juce::Colour sourceColour(uint8_t internalSourceIndex) const;

    uint8_t m_row = 0;
    uint8_t m_assigned = froggers_v2::kNoSelection;
    juce::ComboBox m_dropdown;
};
