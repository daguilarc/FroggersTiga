#pragma once

#include "DelayState.hpp"
#include "DesktopHostIO.hpp"
#include "HostParameterInventory.hpp"
#include "HostParameterPendingStore.hpp"
#include "HostParameterRouting.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

class HostParameterRegistry
{
public:
    static constexpr size_t kParameterCount = HostParameterInventory::kCount;

    void registerParameters(juce::AudioProcessor& processor, HostParameterPendingStore& pendingStore);

    void applyPending(DesktopHostIO& host, DelayState& delay, HostParameterPendingStore& pendingStore) const
    {
        HostParameterRouting::applyPending(host, delay, pendingStore);
    }

    void syncFromHost(DesktopHostIO& host, DelayState& delay, HostParameterPendingStore& pendingStore) const;

    static bool inventoryMatchesRegistryTable()
    {
        return HostParameterRouting::inventoryMatchesRegistryTable();
    }

    static void applyValue(const HostParameterInventory::Descriptor& entry,
                           float normalizedValue,
                           DesktopHostIO& host,
                           DelayState& delay)
    {
        HostParameterRouting::applyValue(entry, normalizedValue, host, delay);
    }

    static float readValue(const HostParameterInventory::Descriptor& entry,
                           DesktopHostIO& host,
                           const DelayState& delay)
    {
        return HostParameterRouting::readValue(entry, host, delay);
    }

    static juce::String displayNameFor(const HostParameterInventory::Descriptor& entry);

private:
    class ChangeForwarder : public juce::AudioProcessorParameter::Listener
    {
    public:
        ChangeForwarder(size_t index, HostParameterPendingStore& store)
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
        HostParameterPendingStore& m_store;
    };

    std::array<juce::AudioParameterFloat*, kParameterCount> m_parameters{};
    std::vector<std::unique_ptr<ChangeForwarder>> m_listeners;
};
