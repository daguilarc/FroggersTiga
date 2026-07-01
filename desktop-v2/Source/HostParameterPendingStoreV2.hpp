#pragma once

#include "HostParameterInventoryV2.hpp"

#include <array>
#include <atomic>
#include <cstddef>

struct HostParameterPendingStoreV2
{
    static constexpr size_t kCapacity = HostParameterInventoryV2::kCount;

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
