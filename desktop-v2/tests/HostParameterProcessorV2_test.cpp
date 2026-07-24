#include "HostParameterInventoryV2.hpp"
#include "HostParameterRoutingV2.hpp"
#include "HostParameterStateEnvelopeV2.hpp"
#include "manifest/FroggersV2AppManifest.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifndef FROGGERS_VST_IS_SYNTH
#error "HostParameterProcessorV2_test requires FROGGERS_VST_IS_SYNTH=1"
#endif

#if FROGGERS_VST_IS_SYNTH != 1
#error "FROGGERS_VST_IS_SYNTH must be 1 for instrument identity"
#endif

#ifndef FROGGERS_EXPECT_HOST_PARAM_COUNT_V2
// Task 7.4 (D11/D14): Cross-coupler axes removed from the Audio page (1 page
// knob + 1 page mod depth), so 122 -> 120.
// Task 7.5 (D15): per-VCO Sustain added to the ADSR/Envelope page (3 new rows
// -- one per VCO -- each contributing 1 page knob + 1 page mod depth), so
// 120 -> 126.
#define FROGGERS_EXPECT_HOST_PARAM_COUNT_V2 126
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

    // Crunchy is a scene-morphed parameter: setGlobalCrunchy() writes the
    // *active-scene* endpoint (activeSceneOrdinal()), while globalCrunchy()
    // returns the left<->right scene crossfade (blendedSceneCenter). Those two
    // views only coincide when the blend saturates to an endpoint; at the
    // default blend of 0.5 with left=0/right=1 a set-then-get is intentionally
    // NOT idempotent (the crossfade semantics are locked in by
    // test_crunchy_scene_encoder_parity in ControlCoreBridge_test). Pin the
    // blend to an endpoint so this routing check has a well-defined round-trip:
    // it verifies the host param reaches BOTH the host flat float and the
    // control core, not the orthogonal scene-blend morph behaviour.
    core.setSceneBlend(0.0f);

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

// Task 7.8 (D13d, operator 2026-07-23): the three VCO waveform-morph controls
// are labeled "Shape" (desktop-v2-scoped display name only -- the internal
// VcoMorph axis/stableId ("vco_morph%u") are unchanged, and the shared
// v1-desktop table (desktop/Source/HostParameterRegistry.cpp) still says
// "morph", which is correct: that path is out of this task's scope).
bool test_vco_morph_display_name_is_shape()
{
    for (uint8_t i = 0; i < HostParameterInventoryV2::kMorphCount; ++i)
    {
        char buffer[64];
        froggers_v2::manifest::formatInventoryDisplayName(
            buffer, sizeof(buffer), HostParameterInventoryV2::Axis::VcoMorph, 0, 0, i);
        const std::string expected = "Global/VCO" + std::to_string(i + 1) + " Shape";
        if (expected != buffer)
        {
            std::printf(
                "FAIL: VCO morph %u displayName expected \"%s\" got \"%s\"\n",
                static_cast<unsigned>(i),
                expected.c_str(),
                buffer);
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

// Task 7.5 (D15): Sustain is now a real per-VCO control (true ASR) -- this
// used to be test_no_sus_stable_ids, guarding that Sustain was NOT present
// while it was still an open "sustain spike" question. Now that D15 landed
// it, flip the assertion: the Envelope page must expose all three VCOs'
// Sustain rows in the v2 inventory.
bool test_sustain_labels_present()
{
    bool foundVco1 = false;
    bool foundVco2 = false;
    bool foundVco3 = false;
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        foundVco1 = foundVco1 || std::strstr(entry.displayName, "Sustain VCO1") != nullptr;
        foundVco2 = foundVco2 || std::strstr(entry.displayName, "Sustain VCO2") != nullptr;
        foundVco3 = foundVco3 || std::strstr(entry.displayName, "Sustain VCO3") != nullptr;
    }
    if (!foundVco1 || !foundVco2 || !foundVco3)
    {
        std::printf(
            "FAIL: expected Sustain VCO1/2/3 displayNames in v2 inventory (found1=%d found2=%d found3=%d)\n",
            foundVco1,
            foundVco2,
            foundVco3);
        return false;
    }
    return true;
}

// 11.2: plugin state round-trip for the manifest-owned host parameters that the legacy
// SimPresetSnapshot payload never serializes (GlobalCrunchy, VcoMorph, Sequencer, SceneBlend,
// GestureWeight — see HostParameterStateEnvelopeV2.hpp for why PageKnob/PageModDepth are
// intentionally excluded here: those already round-trip correctly through SimPresetSnapshot's
// raw-value copy, and re-checking them via HostParameterRoutingV2::readValue would fail because
// several page rows are read back through the musical "Fuego" curve, whose output is not its own
// inverse). Simulates a real DAW save/reload: values are applied on one
// (host/delay/sequencer/core) instance, serialized via HostParameterStateEnvelopeV2 (the tail
// PluginProcessorV2::getStateInformation/setStateInformation append after the legacy
// SimPresetSnapshot payload — see PluginProcessorV2.cpp), then deserialized into a *fresh*,
// default-constructed instance standing in for a newly created plugin instance. Every covered
// stable ID is looked up by name (via a stableId -> expected-value map) rather than assumed to
// stay at the same index, so a hypothetical future reordering of the inventory axes would still
// be caught if stable-ID identity broke.
bool test_state_envelope_tail_round_trip_keyed_by_stable_id()
{
    if (HostParameterStateEnvelopeV2::kEntryCount != 12)
    {
        std::printf("FAIL: expected HostParameterStateEnvelopeV2 to cover 12 non-page stable IDs, got %zu\n",
                    HostParameterStateEnvelopeV2::kEntryCount);
        return false;
    }

    auto hostA = std::make_unique<DesktopHostIO>();
    DelayState delayA;
    SequencerState sequencerA;
    froggers_v2::FroggersV2ControlCore coreA;
    initHostAndDelay(*hostA, delayA);

    const size_t firstIndex = HostParameterStateEnvelopeV2::kFirstNonPageIndex;
    const size_t lastIndex = HostParameterInventoryV2::kCount;

    // Phase 1: apply a varied, deterministic value to every covered entry (golden-ratio stride
    // keeps it well away from 0/0.5/1 degenerate defaults without ever landing exactly on
    // entry.minNorm/maxNorm).
    for (size_t i = firstIndex; i < lastIndex; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        const float stride = static_cast<float>(i) * 0.6180339887f;
        const float fractional = stride - std::floor(stride);
        const float target = entry.minNorm + fractional * (entry.maxNorm - entry.minNorm);
        HostParameterRoutingV2::applyValue(entry, target, *hostA, delayA, sequencerA, coreA);
    }

    // Phase 2: capture the fully-settled read-back (post-quantization, e.g. sequencer
    // direction/speed choice snapping) for each stable ID. This is exactly what writeTail() below
    // will independently recompute, so the round trip is judged against the real settled state.
    std::map<std::string, float> expectedByStableId;
    for (size_t i = firstIndex; i < lastIndex; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        const float settled = HostParameterRoutingV2::readValue(entry, *hostA, delayA, sequencerA, coreA);
        expectedByStableId[std::string(entry.stableId)] = settled;
    }

    std::vector<uint8_t> tail(HostParameterStateEnvelopeV2::kTailBytes);
    HostParameterStateEnvelopeV2::writeTail(*hostA, delayA, sequencerA, coreA, tail.data());

    // Fresh instance: stands in for a brand-new plugin object (e.g. DAW project reopened).
    auto hostB = std::make_unique<DesktopHostIO>();
    DelayState delayB;
    SequencerState sequencerB;
    froggers_v2::FroggersV2ControlCore coreB;
    initHostAndDelay(*hostB, delayB);

    HostParameterStateEnvelopeV2::readTail(tail.data(), *hostB, delayB, sequencerB, coreB);

    for (size_t i = firstIndex; i < lastIndex; ++i)
    {
        const HostParameterInventoryV2::RuntimeDescriptor entry = HostParameterInventoryV2::buildDescriptorAt(i);
        const auto found = expectedByStableId.find(std::string(entry.stableId));
        if (found == expectedByStableId.end())
        {
            std::printf("FAIL: no recorded expected value for stableId %s\n", entry.stableId);
            return false;
        }

        const float actual = HostParameterRoutingV2::readValue(entry, *hostB, delayB, sequencerB, coreB);
        if (!nearlyEqual(actual, found->second, 1.0e-4f))
        {
            std::printf("FAIL: state round-trip mismatch for stableId %s expected %f got %f\n",
                        entry.stableId,
                        found->second,
                        actual);
            return false;
        }
    }

    return true;
}

struct NamedTest
{
    const char* name;
    bool (*run)();
};

// Accumulate-then-apply: run every case and report all results instead of bailing out on the
// first failure, so one failing case can never hide the pass/fail status of the others.
constexpr NamedTest kTests[] = {
    {"test_parameter_count_assertion", test_parameter_count_assertion},
    {"test_inventory_registry_completeness", test_inventory_registry_completeness},
    {"test_pending_coalescing_latest_wins", test_pending_coalescing_latest_wins},
    {"test_global_crunchy_routing", test_global_crunchy_routing},
    {"test_vco_morph_defaults", test_vco_morph_defaults},
    {"test_vco_morph_display_name_is_shape", test_vco_morph_display_name_is_shape},
    {"test_no_pair_ar_axis", test_no_pair_ar_axis},
    {"test_sustain_labels_present", test_sustain_labels_present},
    {"test_state_envelope_tail_round_trip_keyed_by_stable_id",
     test_state_envelope_tail_round_trip_keyed_by_stable_id},
};
} // namespace

int main()
{
    bool anyFailure = false;
    for (const NamedTest& t : kTests)
    {
        const bool passed = t.run();
        std::printf("%s: %s\n", passed ? "PASS" : "FAIL", t.name);
        if (!passed)
        {
            anyFailure = true;
        }
    }

    if (anyFailure)
    {
        std::printf("FAIL: one or more HostParameterProcessorV2 tests failed\n");
        return 1;
    }

    std::printf("PASS: HostParameterProcessorV2 tests (%zu parameters, synth identity verified)\n",
                HostParameterInventoryV2::kCount);
    return 0;
}
