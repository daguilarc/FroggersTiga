#pragma once

#include "CvScopeDisplay.h"
#include "DesktopHostIO.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstddef>
#include <cstdint>

enum class GlobalOscilloscopeSourceGroup : uint8_t
{
    VcoDefault,
    Lfo,
    VcoPairBus,
    VcoEnvelopeFollower,
    RandomMarbles,
    ExternalAudio,
};

class GlobalOscilloscopeDisplay : public juce::Component,
                                  private juce::Timer
{
public:
    GlobalOscilloscopeDisplay();

    void bindHost(DesktopHostIO* host);
    void setAudioRunning(bool running);
    void setExternalAudioAvailable(bool available);
    void setSourceGroup(GlobalOscilloscopeSourceGroup group);
    GlobalOscilloscopeSourceGroup sourceGroup() const;

    // Test/inspection accessors: expose the fixed-capacity trace bindings so
    // coverage can assert trace count and manifest-tap mapping without
    // reaching into private state.
    size_t traceCount() const;
    uint8_t traceModIndex(size_t trace) const;

    void resized() override;

private:
    struct TraceBinding
    {
        uint8_t modIndex = UINT8_MAX;
        juce::Colour colour;
        bool active = false;
    };

    void timerCallback() override;
    void rebuildTraceBindings();
    void refreshTraces();

    DesktopHostIO* m_host = nullptr;
    bool m_audioRunning = false;
    bool m_externalAudioAvailable = false;
    GlobalOscilloscopeSourceGroup m_sourceGroup = GlobalOscilloscopeSourceGroup::VcoDefault;
    CvScopeDisplay m_scope;
    std::array<TraceBinding, CvScopeDisplay::kMaxTraces> m_bindings{};
    size_t m_bindingCount = 0;
};
