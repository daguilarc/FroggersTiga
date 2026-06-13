#pragma once

#include "DesktopHostIO.hpp"
#include "ModModuleBox.h"
#include "PatchCableOverlay.h"

#include <juce_gui_basics/juce_gui_basics.h>

class ModRackPanel : public juce::Component
{
public:
    explicit ModRackPanel(DesktopHostIO& host);

    void refresh(bool audioRunning);
    void collectOutputPorts(std::vector<PatchCableOverlay::OutputPort>& ports) const;
    void resized() override;

private:
    DesktopHostIO& m_host;
    ModModuleBox m_midi;
    ModModuleBox m_vcoFeat;
    ModModuleBox m_marbles1;
    ModModuleBox m_marbles2;
};
