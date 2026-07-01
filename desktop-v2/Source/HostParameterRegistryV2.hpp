#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "HostParameterInventoryV2.hpp"
#include "HostParameterPendingStoreV2.hpp"
#include "HostParameterRoutingV2.hpp"
#include "SequencerState.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

class HostParameterRegistryV2
{
public:
    static constexpr size_t kParameterCount = HostParameterInventoryV2::kCount;

    void registerParameters(juce::AudioProcessor& processor, HostParameterPendingStoreV2& pendingStore);

    void applyPending(DesktopHostIO& host,
                      DelayState& delay,
                      SequencerState& sequencer,
                      froggers_v2::FroggersV2ControlCore& core,
                      HostParameterPendingStoreV2& pendingStore) const
    {
        HostParameterRoutingV2::applyPending(host, delay, sequencer, core, pendingStore);
    }

    void syncFromHost(DesktopHostIO& host,
                      DelayState& delay,
                      SequencerState& sequencer,
                      froggers_v2::FroggersV2ControlCore& core,
                      HostParameterPendingStoreV2& pendingStore) const;

    static bool inventoryMatchesRegistryTable()
    {
        return HostParameterRoutingV2::inventoryMatchesRegistryTable();
    }

private:
    class ChangeForwarder : public juce::AudioProcessorParameter::Listener
    {
    public:
        ChangeForwarder(size_t index, HostParameterPendingStoreV2& store)
            : m_index(index)
            , m_store(store)
        {
        }

        void parameterValueChanged(int, float newValue) override
        {
            m_store.queue(m_index, newValue);
        }

        void parameterGestureChanged(int, bool) override
        {
        }

    private:
        size_t m_index;
        HostParameterPendingStoreV2& m_store;
    };

    std::array<juce::AudioParameterFloat*, kParameterCount> m_parameters{};
    std::vector<std::unique_ptr<ChangeForwarder>> m_listeners;
};
