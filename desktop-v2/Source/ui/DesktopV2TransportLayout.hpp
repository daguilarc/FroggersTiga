#pragma once

#include "DesktopV2ChromeLayout.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace DesktopV2ChromeLayout
{
inline void layoutStandaloneTransportRow(juce::Rectangle<int>& transport,
                                         juce::Component& play,
                                         juce::Component& stop,
                                         juce::Component& scope,
                                         juce::Component* recordButton = nullptr)
{
    play.setBounds(transport.removeFromLeft(kTransportPlayStopW));
    transport.removeFromLeft(kTransportGapSm);
    stop.setBounds(transport.removeFromLeft(kTransportPlayStopW));
    transport.removeFromLeft(kTransportGapMd);

    if (recordButton != nullptr)
    {
        recordButton->setBounds(transport.removeFromLeft(kRecordButtonMinWidth));
    }
    else
    {
        transport.removeFromLeft(kRecordButtonMinWidth);
    }

    const int scopeW = juce::jmin(transport.getWidth(), kTransportScopeMaxWidth);
    scope.setBounds(transport.getX(), transport.getY(), scopeW, transport.getHeight());
    transport.removeFromLeft(scopeW);
}

inline void layoutRuntimePageRail(juce::Rectangle<int>& rail,
                                  juce::Component& fileButton,
                                  juce::Component& audioButton,
                                  juce::Component& midiButton)
{
    fileButton.setBounds(rail.removeFromTop(kRuntimePageButtonH));
    rail.removeFromTop(kTransportGapSm);
    audioButton.setBounds(rail.removeFromTop(kRuntimePageButtonH));
    rail.removeFromTop(kTransportGapSm);
    midiButton.setBounds(rail.removeFromTop(kRuntimePageButtonH));
}
} // namespace DesktopV2ChromeLayout
