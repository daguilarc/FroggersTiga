#include "ui/GlobalOscilloscopeDisplay.hpp"

#include "manifest/FroggersV2AppManifest.hpp"
#include "ui/DesktopV2ScopeUiTimer.hpp"

#include <cstring>

namespace
{
using froggers_v2::manifest::kOscilloscopeTaps;
using froggers_v2::manifest::kPermanentModulationSources;
using froggers_v2::manifest::ModulationSource;

juce::Colour colourFromManifestRgb(uint32_t rgb)
{
    return juce::Colour(static_cast<juce::uint8>((rgb >> 16) & 0xffu),
                        static_cast<juce::uint8>((rgb >> 8) & 0xffu),
                        static_cast<juce::uint8>(rgb & 0xffu));
}

uint8_t permanentModIndexForStableId(const char* stableId)
{
    if (stableId == nullptr || stableId[0] == '\0')
    {
        return UINT8_MAX;
    }

    for (uint8_t i = 0; i < kPermanentModulationSources.size(); ++i)
    {
        const char* candidate = kPermanentModulationSources[i].stableId;
        if (candidate != nullptr && std::strcmp(candidate, stableId) == 0)
        {
            return i;
        }
    }

    constexpr const char* kOscilloscopePrefix = "oscilloscope_";
    if (std::strncmp(stableId, kOscilloscopePrefix, std::strlen(kOscilloscopePrefix)) == 0)
    {
        return permanentModIndexForStableId(stableId + std::strlen(kOscilloscopePrefix));
    }

    return UINT8_MAX;
}

bool sourceGroupMatches(const ModulationSource& source, GlobalOscilloscopeSourceGroup group)
{
    if (source.group == nullptr)
    {
        return false;
    }

    switch (group)
    {
        case GlobalOscilloscopeSourceGroup::Lfo:
            return std::strcmp(source.group, "lfo") == 0;
        case GlobalOscilloscopeSourceGroup::VcoPairBus:
            return std::strcmp(source.group, "vco-pair-bus") == 0;
        case GlobalOscilloscopeSourceGroup::VcoEnvelopeFollower:
            return std::strcmp(source.group, "vco-envelope-follower") == 0;
        case GlobalOscilloscopeSourceGroup::RandomMarbles:
            return std::strcmp(source.group, "random") == 0;
        case GlobalOscilloscopeSourceGroup::ExternalAudio:
            return std::strcmp(source.group, "external-audio") == 0;
        case GlobalOscilloscopeSourceGroup::VcoDefault:
            return false;
    }

    return false;
}

bool isExternalAudioSource(const ModulationSource& source)
{
    return source.group != nullptr && std::strcmp(source.group, "external-audio") == 0;
}
} // namespace

GlobalOscilloscopeDisplay::GlobalOscilloscopeDisplay()
{
    m_scope.setTraceMode(CvTraceMode::Continuous);
    addAndMakeVisible(m_scope);
    rebuildTraceBindings();
    startTimerHz(desktop_v2::kDesktopV2ScopeUiTimerHz);
}

void GlobalOscilloscopeDisplay::bindHost(DesktopHostIO* host)
{
    m_host = host;
}

void GlobalOscilloscopeDisplay::setAudioRunning(bool running)
{
    m_audioRunning = running;
    m_scope.setIdle(!running);
}

void GlobalOscilloscopeDisplay::setExternalAudioAvailable(bool available)
{
    if (m_externalAudioAvailable == available)
    {
        return;
    }
    m_externalAudioAvailable = available;
    rebuildTraceBindings();
}

void GlobalOscilloscopeDisplay::setSourceGroup(GlobalOscilloscopeSourceGroup group)
{
    if (m_sourceGroup == group)
    {
        return;
    }
    m_sourceGroup = group;
    rebuildTraceBindings();
}

GlobalOscilloscopeSourceGroup GlobalOscilloscopeDisplay::sourceGroup() const
{
    return m_sourceGroup;
}

size_t GlobalOscilloscopeDisplay::traceCount() const
{
    return m_bindingCount;
}

uint8_t GlobalOscilloscopeDisplay::traceModIndex(size_t trace) const
{
    if (trace >= m_bindingCount)
    {
        return UINT8_MAX;
    }
    return m_bindings[trace].modIndex;
}

void GlobalOscilloscopeDisplay::rebuildTraceBindings()
{
    m_bindingCount = 0;
    m_bindings.fill({});

    if (m_sourceGroup == GlobalOscilloscopeSourceGroup::VcoDefault)
    {
        for (const auto& tap : kOscilloscopeTaps)
        {
            if (!tap.active || m_bindingCount >= CvScopeDisplay::kMaxTraces)
            {
                continue;
            }
            const uint8_t modIndex = permanentModIndexForStableId(tap.stableId);
            if (modIndex == UINT8_MAX)
            {
                continue;
            }
            m_bindings[m_bindingCount] = TraceBinding{
                modIndex,
                colourFromManifestRgb(kPermanentModulationSources[modIndex].colorRgb),
                true,
            };
            ++m_bindingCount;
        }
    }
    else
    {
        for (uint8_t i = 0; i < kPermanentModulationSources.size(); ++i)
        {
            if (m_bindingCount >= CvScopeDisplay::kMaxTraces)
            {
                break;
            }
            const ModulationSource& source = kPermanentModulationSources[i];
            if (!sourceGroupMatches(source, m_sourceGroup))
            {
                continue;
            }
            if (isExternalAudioSource(source) && !m_externalAudioAvailable)
            {
                continue;
            }
            m_bindings[m_bindingCount] = TraceBinding{
                i,
                colourFromManifestRgb(source.colorRgb),
                true,
            };
            ++m_bindingCount;
        }
    }

    m_scope.setTraceCount(m_bindingCount);
    for (size_t trace = 0; trace < m_bindingCount; ++trace)
    {
        m_scope.setTraceColour(trace, m_bindings[trace].colour);
    }
}

void GlobalOscilloscopeDisplay::refreshTraces()
{
    if (!m_host || !m_audioRunning)
    {
        return;
    }

    for (size_t trace = 0; trace < m_bindingCount; ++trace)
    {
        const TraceBinding& binding = m_bindings[trace];
        if (!binding.active || binding.modIndex == UINT8_MAX)
        {
            continue;
        }
        const float sample = m_host->GetCvOut(binding.modIndex);
        m_scope.pushSample(trace, sample);
        m_scope.setTraceAudioRateModulated(trace, m_scope.hasAudioRateActivity(trace));
    }
}

void GlobalOscilloscopeDisplay::timerCallback()
{
    refreshTraces();
}

void GlobalOscilloscopeDisplay::resized()
{
    m_scope.setBounds(getLocalBounds());
}
