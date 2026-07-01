#include "HostParameterRegistryV2.hpp"

#include <memory>
#include <vector>

void HostParameterRegistryV2::syncFromHost(DesktopHostIO& host,
                                           DelayState& delay,
                                           SequencerState& sequencer,
                                           froggers_v2::FroggersV2ControlCore& core,
                                           HostParameterPendingStoreV2& pendingStore) const
{
    for (size_t i = 0; i < kParameterCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        if (m_parameters[i] != nullptr)
        {
            const float value = HostParameterRoutingV2::readValue(entry, host, delay, sequencer, core);
            m_parameters[i]->setValueNotifyingHost(value);
            pendingStore.dirty[i].store(0, std::memory_order_release);
        }
    }
    core.setGlobalCrunchy(host.GetGlobalCrunchy());
}

void HostParameterRegistryV2::registerParameters(juce::AudioProcessor& processor,
                                               HostParameterPendingStoreV2& pendingStore)
{
    for (size_t i = 0; i < kParameterCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        auto* parameter = new juce::AudioParameterFloat(
            juce::ParameterID{entry.stableId, 1},
            entry.displayName,
            juce::NormalisableRange<float>(entry.minNorm, entry.maxNorm),
            entry.defaultNorm);
        m_parameters[i] = parameter;
        processor.addParameter(parameter);

        auto listener = std::make_unique<ChangeForwarder>(i, pendingStore);
        parameter->addListener(listener.get());
        m_listeners.push_back(std::move(listener));
    }
}
