#pragma once

#include "HostParameterInventory.hpp"

#include <array>
#include <atomic>
#include <cstddef>

struct HostParameterPendingStore
{
    static constexpr size_t kCapacity = HostParameterInventory::kCount;

    std::array<std::atomic<float>, kCapacity> values{};
    std::array<std::atomic<uint8_t>, kCapacity> dirty{};

    void queue(size_t index, float normalizedValue)
    {
        if (index >= kCapacity)
        {
            return;
        }
        values[index].store(normalizedValue, std::memory_order_relaxed);
        dirty[index].store(1, std::memory_order_release);
    }
};
