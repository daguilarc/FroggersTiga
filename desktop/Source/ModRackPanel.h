#pragma once

#include "DesktopHostIO.hpp"
#include "ModModuleBox.h"
#include "PatchCableOverlay.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

class ModRackPanel : public juce::Component
{
public:
    explicit ModRackPanel(DesktopHostIO& host);

    void refresh(bool audioRunning);
    void collectOutputPorts(std::vector<PatchCableOverlay::OutputPort>& ports) const;
    void resized() override;

private:
    DesktopHostIO& m_host;
    std::vector<std::unique_ptr<ModModuleBox>> m_boxes;
};
