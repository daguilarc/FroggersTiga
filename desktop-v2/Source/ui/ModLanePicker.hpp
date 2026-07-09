#pragma once

#include "control/FroggersV2ControlCore.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>
#include <functional>

class ModLanePicker : public juce::Component
{
public:
    ModLanePicker();

    void setPage(uint8_t page);
    void setRow(uint8_t row);
    void setAssignedSource(uint8_t internalSourceIndex);
    void setExternalAudioAvailable(bool available);
    void refresh();

    // Whether the given manifest lane (0-based index into
    // manifest::kPermanentModulationSources) can be manually assigned to the
    // currently bound page/row, per manifest eligibility + external-audio
    // availability. Exposed publicly so it can be exercised directly by tests
    // (and consumed by the popup route menu).
    bool isLaneAssignable(uint8_t lane) const;

    std::function<void(uint8_t row, uint8_t internalSourceIndex)> onAssign;

    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    void showRouteMenu();
    juce::Colour sourceColour(uint8_t internalSourceIndex) const;

    uint8_t m_page = 0;
    uint8_t m_row = 0;
    uint8_t m_assigned = froggers_v2::kNoSelection;
    bool m_externalAudioAvailable = false;
};
