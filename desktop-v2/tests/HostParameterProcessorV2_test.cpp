#include "HostParameterInventoryV2.hpp"
#include "HostParameterRoutingV2.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#ifndef FROGGERS_VST_IS_SYNTH
#error "HostParameterProcessorV2_test requires FROGGERS_VST_IS_SYNTH=1"
#endif

#if FROGGERS_VST_IS_SYNTH != 1
#error "FROGGERS_VST_IS_SYNTH must be 1 for instrument identity"
#endif

#ifndef FROGGERS_EXPECT_HOST_PARAM_COUNT_V2
#define FROGGERS_EXPECT_HOST_PARAM_COUNT_V2 142
#endif

namespace
{
void initHostAndDelay(DesktopHostIO& host, DelayState& delay)
{
    host.m_hostKind = SimHostKind::VstV2;
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
    if (HostParameterInventoryV2::kCount
        != static_cast<size_t>(FROGGERS_EXPECT_HOST_PARAM_COUNT_V2))
    {
        std::printf("FAIL: expected %d host parameters, got %zu\n",
                    FROGGERS_EXPECT_HOST_PARAM_COUNT_V2,
                    HostParameterInventoryV2::kCount);
        return false;
    }
    return true;
}

bool test_inventory_registry_completeness()
{
    if (!HostParameterInventoryV2::validateInventory())
    {
        std::printf("FAIL: inventory descriptor table failed validation\n");
        return false;
    }

    if (!HostParameterRoutingV2::inventoryMatchesRegistryTable())
    {
        std::printf("FAIL: registry inventoryMatchesRegistryTable returned false\n");
        return false;
    }

    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        for (size_t j = i + 1; j < HostParameterInventoryV2::kCount; ++j)
        {
            const HostParameterInventoryV2::RuntimeDescriptor other = HostParameterInventoryV2::buildDescriptorAt(j);
            if (std::strcmp(entry.stableId, other.stableId) == 0)
            {
                std::printf("FAIL: duplicate stableId %s\n", entry.stableId);
                return false;
            }
            if (std::strcmp(entry.displayName, other.displayName) == 0)
            {
                std::printf("FAIL: duplicate displayName %s\n", entry.displayName);
                return false;
            }
        }

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
    SequencerState sequencer;
    froggers_v2::FroggersV2ControlCore core;
    initHostAndDelay(*host, delay);

    HostParameterPendingStoreV2 pendingStore;

    constexpr size_t kIndex = 0;
    const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(kIndex);

    pendingStore.queue(kIndex, 0.15f);
    pendingStore.queue(kIndex, 0.85f);
    HostParameterRoutingV2::applyPending(*host, delay, sequencer, core, pendingStore);

    const float applied = HostParameterRoutingV2::readValue(entry, *host, delay, sequencer, core);
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

    return true;
}

bool test_global_crunchy_routing()
{
    auto host = std::make_unique<DesktopHostIO>();
    DelayState delay;
    SequencerState sequencer;
    froggers_v2::FroggersV2ControlCore core;
    initHostAndDelay(*host, delay);

    const HostParameterInventoryV2::RuntimeDescriptor entry =
        HostParameterInventoryV2::buildDescriptorAt(HostParameterInventoryV2::kPageKnobCount
                                                    + HostParameterInventoryV2::kPageModDepthCount);
    HostParameterRoutingV2::applyValue(entry, 0.42f, *host, delay, sequencer, core);

    if (!nearlyEqual(host->GetGlobalCrunchy(), 0.42f) || !nearlyEqual(core.globalCrunchy(), 0.42f))
    {
        std::printf("FAIL: global Crunchy routing mismatch\n");
        return false;
    }

    return true;
}

bool test_vco_morph_defaults()
{
    struct Case
    {
        uint8_t index;
        float expected;
    };
    constexpr Case cases[] = {{0, 0.0f}, {1, 1.0f}, {2, 0.5f}};
    for (const Case& c : cases)
    {
        const float got = HostParameterInventoryV2::vcoMorphDefault(c.index);
        if (!nearlyEqual(got, c.expected))
        {
            std::printf(
                "FAIL: vcoMorphDefault(%u) expected %f got %f\n",
                static_cast<unsigned>(c.index),
                c.expected,
                got);
            return false;
        }
    }
    return true;
}

bool test_no_pair_ar_axis()
{
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        if (std::strstr(entry.stableId, "pair_ar") != nullptr)
        {
            std::printf("FAIL: pair-AR stableId present in v2 inventory: %s\n", entry.stableId);
            return false;
        }
    }
    return true;
}

bool test_no_sus_stable_ids()
{
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        if (std::strstr(entry.stableId, "Sus") != nullptr
            || std::strstr(entry.displayName, "Sus") != nullptr)
        {
            std::printf("FAIL: Sus parameter present in v2 inventory: %s / %s\n",
                        entry.stableId,
                        entry.displayName);
            return false;
        }
    }
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
    if (!test_global_crunchy_routing())
    {
        return 1;
    }
    if (!test_vco_morph_defaults())
    {
        return 1;
    }
    if (!test_no_pair_ar_axis())
    {
        return 1;
    }
    if (!test_no_sus_stable_ids())
    {
        return 1;
    }

    std::printf("PASS: HostParameterProcessorV2 tests (%zu parameters, synth identity verified)\n",
                HostParameterInventoryV2::kCount);
    return 0;
}
