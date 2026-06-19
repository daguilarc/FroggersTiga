#include "HostParameterRegistry.hpp"

#include "ParamDisplayNames.hpp"

#include <memory>
#include <vector>

juce::String HostParameterRegistry::displayNameFor(const HostParameterInventory::Descriptor& entry)
{
    using Axis = HostParameterInventory::Axis;
    switch (entry.axis)
    {
        case Axis::PageKnob:
        case Axis::PageModDepth:
            return juce::String(ParamDisplayNames::forHostPageRow(entry.page, entry.row))
                + (entry.axis == Axis::PageModDepth ? " depth" : "");
        case Axis::DelayKnob:
        case Axis::DelayModDepth:
            return juce::String(ParamDisplayNames::forHostPageRow(HostParameterInventory::kDelayPage, entry.row))
                + (entry.axis == Axis::DelayModDepth ? " depth" : "");
        case Axis::PairArKnob:
        case Axis::PairArModDepth:
            return juce::String(ParamDisplayNames::forAudioPairAr(entry.index))
                + (entry.axis == Axis::PairArModDepth ? " depth" : "");
        case Axis::VcoMorph:
            return "VCO" + juce::String(entry.index + 1) + " morph";
    }
    return {};
}

void HostParameterRegistry::syncFromHost(DesktopHostIO& host,
                                         DelayState& delay,
                                         HostParameterPendingStore& pendingStore) const
{
    for (size_t i = 0; i < kParameterCount; ++i)
    {
        const HostParameterInventory::Descriptor& entry = HostParameterInventory::descriptorAt(i);
        if (m_parameters[i] != nullptr)
        {
            m_parameters[i]->setValueNotifyingHost(readValue(entry, host, delay));
            pendingStore.dirty[i].store(0, std::memory_order_release);
        }
    }
}

void HostParameterRegistry::registerParameters(juce::AudioProcessor& processor,
                                               HostParameterPendingStore& pendingStore)
{
    for (size_t i = 0; i < kParameterCount; ++i)
    {
        const HostParameterInventory::Descriptor& entry = HostParameterInventory::descriptorAt(i);
        auto* parameter = new juce::AudioParameterFloat(
            juce::ParameterID{entry.stableKey, 1},
            displayNameFor(entry),
            juce::NormalisableRange<float>(entry.minNorm, entry.maxNorm),
            entry.defaultNorm);
        m_parameters[i] = parameter;
        processor.addParameter(parameter);

        auto listener = std::make_unique<ChangeForwarder>(i, pendingStore);
        parameter->addListener(listener.get());
        m_listeners.push_back(std::move(listener));
    }
}
