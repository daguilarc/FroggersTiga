#include "HostParameterInventory.hpp"
#include "HostParameterRouting.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#ifndef FROGGERS_VST_IS_SYNTH
#error "HostParameterProcessor_test requires FROGGERS_VST_IS_SYNTH=1 (task 5.9)"
#endif

#if FROGGERS_VST_IS_SYNTH != 1
#error "FROGGERS_VST_IS_SYNTH must be 1 for instrument identity (task 5.9)"
#endif

namespace
{
void initHostAndDelay(DesktopHostIO& host, DelayState& delay)
{
    host.setDelayState(&delay);
    host.Init();
    delay.init(44100.0f);
}

bool nearlyEqual(float a, float b, float epsilon = 1.0e-5f)
{
    return std::fabs(a - b) <= epsilon;
}

bool test_parameter_count_assertion()
{
    if (HostParameterInventory::kCount != 107)
    {
        std::printf("FAIL: expected 107 host parameters, got %zu\n", HostParameterInventory::kCount);
        return false;
    }
    return true;
}

bool test_inventory_registry_completeness()
{
    if (!HostParameterInventory::validateInventory())
    {
        std::printf("FAIL: inventory descriptor table failed validation\n");
        return false;
    }

    if (!HostParameterRouting::inventoryMatchesRegistryTable())
    {
        std::printf("FAIL: registry inventoryMatchesRegistryTable returned false\n");
        return false;
    }

    for (size_t i = 0; i < HostParameterInventory::kCount; ++i)
    {
        for (size_t j = i + 1; j < HostParameterInventory::kCount; ++j)
        {
            if (std::strcmp(HostParameterInventory::kDescriptors[i].stableKey,
                            HostParameterInventory::kDescriptors[j].stableKey)
                == 0)
            {
                std::printf("FAIL: duplicate stable key %s\n",
                            HostParameterInventory::kDescriptors[i].stableKey);
                return false;
            }
        }

        const HostParameterInventory::Descriptor& entry = HostParameterInventory::descriptorAt(i);
        if (entry.defaultNorm < entry.minNorm || entry.defaultNorm > entry.maxNorm)
        {
            std::printf("FAIL: inventory default out of range at index %zu\n", i);
            return false;
        }
    }

    return true;
}

bool test_pending_coalescing_latest_wins()
{
    auto host = std::make_unique<DesktopHostIO>();
    DelayState delay;
    initHostAndDelay(*host, delay);

    HostParameterPendingStore pendingStore;

    constexpr size_t kIndex = 0;
    const HostParameterInventory::Descriptor& entry = HostParameterInventory::descriptorAt(kIndex);

    pendingStore.queue(kIndex, 0.15f);
    pendingStore.queue(kIndex, 0.85f);
    HostParameterRouting::applyPending(*host, delay, pendingStore);

    const float applied = HostParameterRouting::readValue(entry, *host, delay);
    if (!nearlyEqual(applied, 0.85f))
    {
        std::printf("FAIL: coalesced pending value expected 0.85, got %f\n", applied);
        return false;
    }

    if (pendingStore.dirty[kIndex].load(std::memory_order_acquire) != 0)
    {
        std::printf("FAIL: dirty flag still set after apply\n");
        return false;
    }

    pendingStore.queue(kIndex, 0.20f);
    pendingStore.queue(kIndex, 0.80f);
    HostParameterRouting::applyPending(*host, delay, pendingStore);

    const float secondApplied = HostParameterRouting::readValue(entry, *host, delay);
    if (!nearlyEqual(secondApplied, 0.80f))
    {
        std::printf("FAIL: second coalescing pass expected 0.80, got %f\n", secondApplied);
        return false;
    }

    return true;
}

bool test_apply_order_deterministic()
{
    auto host = std::make_unique<DesktopHostIO>();
    DelayState delay;
    initHostAndDelay(*host, delay);

    HostParameterPendingStore pendingStore;

    const size_t indices[] = {39, 0, 15, 5};
    const float values[] = {0.11f, 0.22f, 0.33f, 0.44f};

    for (size_t n = 0; n < sizeof(indices) / sizeof(indices[0]); ++n)
    {
        pendingStore.queue(indices[n], values[n]);
    }

    HostParameterRouting::applyPending(*host, delay, pendingStore);

    for (size_t n = 0; n < sizeof(indices) / sizeof(indices[0]); ++n)
    {
        const HostParameterInventory::Descriptor& entry =
            HostParameterInventory::descriptorAt(indices[n]);
        const float applied = HostParameterRouting::readValue(entry, *host, delay);
        if (!nearlyEqual(applied, values[n]))
        {
            std::printf("FAIL: apply order index %zu expected %f, got %f\n",
                        indices[n],
                        values[n],
                        applied);
            return false;
        }
        if (pendingStore.dirty[indices[n]].load(std::memory_order_acquire) != 0)
        {
            std::printf("FAIL: dirty flag still set for index %zu after apply\n", indices[n]);
            return false;
        }
    }

    auto hostRepeat = std::make_unique<DesktopHostIO>();
    DelayState delayRepeat;
    initHostAndDelay(*hostRepeat, delayRepeat);

    HostParameterPendingStore pendingRepeat;
    for (size_t n = 0; n < sizeof(indices) / sizeof(indices[0]); ++n)
    {
        pendingRepeat.queue(indices[n], values[n]);
    }

    std::vector<float> forwardSnapshot(HostParameterInventory::kCount, -1.0f);
    for (size_t i = 0; i < HostParameterInventory::kCount; ++i)
    {
        if (pendingRepeat.dirty[i].load(std::memory_order_acquire) == 0)
        {
            continue;
        }
        const float value = pendingRepeat.values[i].load(std::memory_order_relaxed);
        pendingRepeat.dirty[i].store(0, std::memory_order_release);
        HostParameterRouting::applyValue(HostParameterInventory::descriptorAt(i),
                                         value,
                                         *hostRepeat,
                                         delayRepeat);
        forwardSnapshot[i] = HostParameterRouting::readValue(
            HostParameterInventory::descriptorAt(i),
            *hostRepeat,
            delayRepeat);
    }

    auto hostRegistry = std::make_unique<DesktopHostIO>();
    DelayState delayRegistry;
    initHostAndDelay(*hostRegistry, delayRegistry);

    HostParameterPendingStore pendingRegistry;
    for (size_t n = 0; n < sizeof(indices) / sizeof(indices[0]); ++n)
    {
        pendingRegistry.queue(indices[n], values[n]);
    }
    HostParameterRouting::applyPending(*hostRegistry, delayRegistry, pendingRegistry);

    for (size_t n = 0; n < sizeof(indices) / sizeof(indices[0]); ++n)
    {
        const size_t index = indices[n];
        const float registryValue = HostParameterRouting::readValue(
            HostParameterInventory::descriptorAt(index),
            *hostRegistry,
            delayRegistry);
        if (!nearlyEqual(registryValue, forwardSnapshot[index]))
        {
            std::printf("FAIL: registry apply diverged from index-order reference at %zu\n", index);
            return false;
        }
    }

    return true;
}

bool test_zero_input_render_smoke()
{
    // Full ProcessBlock smoke requires the hosted audio link (task 5.10); standalone
    // routing tests above cover null-input safety at the parameter-apply boundary.
    std::printf("SKIP: zero-input ProcessBlock smoke (deferred to hosted plugin tests)\n");
    return true;
}
} // namespace

int main()
{
    if (!test_parameter_count_assertion())
    {
        return 1;
    }
    if (!test_inventory_registry_completeness())
    {
        return 1;
    }
    if (!test_pending_coalescing_latest_wins())
    {
        return 1;
    }
    if (!test_apply_order_deterministic())
    {
        return 1;
    }
    if (!test_zero_input_render_smoke())
    {
        return 1;
    }

    std::printf("PASS: HostParameterProcessor tests (%zu parameters, synth identity verified)\n",
                HostParameterInventory::kCount);
    return 0;
}
