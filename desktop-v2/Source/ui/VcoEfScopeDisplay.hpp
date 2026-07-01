#pragma once

#include "CvScopeDisplay.h"
#include "DesktopHostIO.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>

class VcoEfScopeDisplay : public juce::Component
{
public:
    VcoEfScopeDisplay();

    void bindHost(DesktopHostIO* host);
    void refresh(bool audioRunning);

    void resized() override;

private:
    static constexpr uint8_t kVco1EfIndex = 7;
    static constexpr uint8_t kTraceCount = 3;

    DesktopHostIO* m_host = nullptr;
    CvScopeDisplay m_scope;
};
