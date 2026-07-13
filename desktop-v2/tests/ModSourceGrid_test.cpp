// Packet 2 (task 2.7): parameter-detail 16-cell grid, 15-lane manifest rack,
// CV LED behavior, bipolar mod depth, and unavailable-external-audio state.
// Every source lane / eligibility rule projects from the manifest
// (FroggersV2AppManifest.hpp) — this suite consumes it, it does not re-derive it.
// Packet 4 (modulator visualizers): CvLaneHistoryStore + underlay bind / Target clear.
#include "control/FroggersV2ControlCore.hpp"
#include "manifest/FroggersV2AppManifest.hpp"
#include "ui/CvLaneHistoryStore.hpp"
#include "ui/EncoderRingComponent.hpp"
#include "ui/ModDetailUnderlayBind.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{
using froggers_v2::FroggersV2ControlCore;
using froggers_v2::MessageIn;
namespace manifest = froggers_v2::manifest;

bool nearlyEqual(float a, float b, float eps = 2.0e-3f)
{
    return std::fabs(a - b) < eps;
}

void pushAndProcess(FroggersV2ControlCore& core, const MessageIn& message)
{
    core.bus().push(message);
    core.processBus();
}

MessageIn selectPage(uint8_t page)
{
    MessageIn m;
    m.type = MessageIn::Type::SelectPage;
    m.page = page;
    return m;
}

MessageIn assignSource(uint8_t page, uint8_t row, uint8_t source)
{
    MessageIn m;
    m.type = MessageIn::Type::ModSourceAssign;
    m.page = page;
    m.slot = row;
    m.index = source;
    return m;
}

MessageIn modDrillIn(uint8_t page, uint8_t slot)
{
    return MessageIn::ModDrillIn(page, slot);
}

// Opens the parameter-detail grid on (page, row) with `source` assigned.
void openDetailGrid(FroggersV2ControlCore& core, uint8_t page, uint8_t row, uint8_t source)
{
    pushAndProcess(core, selectPage(page));
    pushAndProcess(core, assignSource(page, row, source));
    pushAndProcess(core, modDrillIn(page, row));
}

bool test_15_lane_rack()
{
    if (manifest::kPermanentModulationSources.size() != 15
        || froggers_v2::kNumModSources != 15)
    {
        std::printf("FAIL: permanent modulation rack is not 15 lanes\n");
        return false;
    }
    for (const auto& source : manifest::kPermanentModulationSources)
    {
        if (source.displayName == nullptr || source.displayName[0] == '\0')
        {
            std::printf("FAIL: manifest source lane missing display name\n");
            return false;
        }
    }
    // LFO 1-3 must be present as first-class source lanes (8,9,10).
    if (std::string(manifest::kPermanentModulationSources[8].displayName) != "LFO 1"
        || std::string(manifest::kPermanentModulationSources[9].displayName) != "LFO 2"
        || std::string(manifest::kPermanentModulationSources[10].displayName) != "LFO 3")
    {
        std::printf("FAIL: LFO 1-3 not present as manifest source lanes\n");
        return false;
    }
    return true;
}

bool test_16_cell_detail_grid()
{
    if (froggers_v2::kUiSlots != manifest::kModDetailCellCount || froggers_v2::kUiSlots != 16)
    {
        std::printf("FAIL: detail grid cell count is not the manifest 16\n");
        return false;
    }

    FroggersV2ControlCore core;
    openDetailGrid(core, 3, 0, 8);

    if (core.visibleCount() != 16)
    {
        std::printf("FAIL: detail grid visible count %u != 16\n",
                    static_cast<unsigned>(core.visibleCount()));
        return false;
    }
    // Cells 0..14 map 1:1 to manifest lanes; cell 15 is the dedicated target.
    for (uint8_t lane = 0; lane < froggers_v2::kNumModSources; ++lane)
    {
        if (core.visibleSlotIsTarget(lane) || core.visibleModIndexForSlot(lane) != lane)
        {
            std::printf("FAIL: detail cell %u is not lane %u\n",
                        static_cast<unsigned>(lane), static_cast<unsigned>(lane));
            return false;
        }
    }
    if (!core.visibleSlotIsTarget(15) || core.visibleModIndexForSlot(15) != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: cell 15 is not the dedicated target/Crispy cell\n");
        return false;
    }
    return true;
}

bool test_cv_led_behavior()
{
    FroggersV2ControlCore core;
    openDetailGrid(core, 3, 0, 5);
    // Packet 15.2a: a lane's CV LED lights when its depth is non-zero, not on
    // assignment alone (assignment alone leaves depth at 0/off, matching the
    // target contract) -- turn the assigned lane's own cell before checking.
    pushAndProcess(core, MessageIn::ParamTurn(3, 5, 5.0f));

    // Only the assigned lane's cell lights its CV LED (modulator mask bit); the
    // route also lights the row's aggregate modulator mask.
    if (core.uiState().modulatorsMask[5].load(std::memory_order_acquire) == 0)
    {
        std::printf("FAIL: assigned lane cell CV LED not lit\n");
        return false;
    }
    if (core.uiState().modulatorsMask[6].load(std::memory_order_acquire) != 0)
    {
        std::printf("FAIL: unassigned lane cell CV LED lit\n");
        return false;
    }
    if (core.effectiveRow(3, 0).modulatorsMask == 0)
    {
        std::printf("FAIL: row modulator mask not set for assigned route\n");
        return false;
    }

    // Selecting None clears the route and extinguishes the LED.
    pushAndProcess(core, assignSource(3, 0, froggers_v2::kNoSelection));
    if (core.assignedModSource(3, 0) != froggers_v2::kNoSelection
        || core.effectiveRow(3, 0).modulatorsMask != 0)
    {
        std::printf("FAIL: None did not clear the route / CV LED\n");
        return false;
    }
    return true;
}

bool test_bipolar_depth()
{
    FroggersV2ControlCore core;
    openDetailGrid(core, 3, 0, 5);
    const float center = core.effectiveRow(3, 0).sceneLeft;

    // Turning the assigned lane's cell negative gives a negative depth and a
    // symmetric bipolar arc around the base value.
    pushAndProcess(core, MessageIn::ParamTurn(3, 5, -5.0f));
    const float negDepth = core.assignedModDepth(3, 0);
    if (negDepth >= 0.0f)
    {
        std::printf("FAIL: negative turn did not produce negative depth (%f)\n", negDepth);
        return false;
    }
    const auto negRow = core.effectiveRow(3, 0);
    if (!nearlyEqual(center - negRow.arcMin, negRow.arcMax - center))
    {
        std::printf("FAIL: bipolar arc not symmetric around base value\n");
        return false;
    }

    // A large positive turn flips the sign and clamps to the bipolar [-1, 1].
    pushAndProcess(core, MessageIn::ParamTurn(3, 5, 200.0f));
    const float posDepth = core.assignedModDepth(3, 0);
    if (posDepth <= 0.0f || posDepth > 1.0f + 1.0e-4f)
    {
        std::printf("FAIL: positive depth not clamped into (0, 1]: %f\n", posDepth);
        return false;
    }
    return true;
}

bool test_unavailable_external_audio()
{
    constexpr uint8_t kPage = 3;  // non-VCO page: pair-bus special-casing does not apply
    constexpr uint8_t kRow = 0;

    // LFO 1-3 (lanes 8,9,10) are always assignable.
    for (uint8_t lfo = 8; lfo <= 10; ++lfo)
    {
        if (!manifest::isModLaneAssignable(kPage, kRow, lfo, false))
        {
            std::printf("FAIL: LFO lane %u should be assignable\n", static_cast<unsigned>(lfo));
            return false;
        }
    }

    // External-audio lanes are visible-but-unavailable with no input.
    if (manifest::isModLaneAssignable(kPage, kRow, manifest::kExternalAudioRateLane, false)
        || manifest::isModLaneAssignable(kPage, kRow, manifest::kExternalAudioEfLane, false))
    {
        std::printf("FAIL: external-audio lanes assignable with no input\n");
        return false;
    }

    // With input present, both external-audio lanes become assignable.
    if (!manifest::isModLaneAssignable(kPage, kRow, manifest::kExternalAudioRateLane, true)
        || !manifest::isModLaneAssignable(kPage, kRow, manifest::kExternalAudioEfLane, true))
    {
        std::printf("FAIL: external-audio lanes not assignable with input present\n");
        return false;
    }
    return true;
}

bool test_vco_pair_bus_eligibility()
{
    // VCO page (0): each VCO row only accepts the complementary pair-bus lane
    // (VCO1→2+3=lane1, VCO2→1+3=lane2, VCO3→1+2=lane0); no raw single-VCO lanes.
    struct RowExpect { uint8_t row; uint8_t eligibleLane; };
    const RowExpect expected[] = {{0, 1}, {1, 2}, {2, 0}};
    for (const auto& e : expected)
    {
        for (uint8_t lane = 0; lane <= 2; ++lane)
        {
            const bool assignable = manifest::isModLaneAssignable(0, e.row, lane, false);
            if (assignable != (lane == e.eligibleLane))
            {
                std::printf("FAIL: VCO row %u lane %u eligibility wrong (got %d)\n",
                            static_cast<unsigned>(e.row), static_cast<unsigned>(lane), assignable);
                return false;
            }
        }
    }

    // Non-VCO page: pair-bus special-casing does not apply, all three assignable.
    for (uint8_t lane = 0; lane <= 2; ++lane)
    {
        if (!manifest::isModLaneAssignable(3, 0, lane, false))
        {
            std::printf("FAIL: pair-bus lane %u not assignable on non-VCO page\n",
                        static_cast<unsigned>(lane));
            return false;
        }
    }
    return true;
}

bool test_multi_lane_effective_with_eligibility()
{
    // Packet 15.9: two eligible depths sum into modulatorsMask; ineligible
    // leftover depth is ignored by effectiveRow/computeEffective.
    FroggersV2ControlCore core;
    constexpr uint8_t kPage = 0;
    constexpr uint8_t kRow = 0;

    core.setLaneDepth(kPage, kRow, 1, 0.4f);  // eligible pair-bus
    core.setLaneDepth(kPage, kRow, 8, 0.3f);  // eligible LFO 1
    core.setLaneDepth(kPage, kRow, 0, 0.9f);  // ineligible pair-bus leftover

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    const auto row = core.effectiveRow(kPage, kRow);
    const uint16_t expected = static_cast<uint16_t>((1u << 1) | (1u << 8));
    if (row.modulatorsMask != expected)
    {
        std::printf("FAIL: multi-lane eligibility mask expected 0x%x got 0x%x\n",
                    static_cast<unsigned>(expected),
                    static_cast<unsigned>(row.modulatorsMask));
        return false;
    }
    return true;
}

bool test_unavailable_lane_not_editable()
{
    FroggersV2ControlCore core;
    constexpr uint8_t kPage = 3;
    constexpr uint8_t kRow = 0;
    constexpr uint8_t kLane = manifest::kExternalAudioRateLane;

    pushAndProcess(core, selectPage(kPage));
    pushAndProcess(core, modDrillIn(kPage, kRow));
    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLane, 12.0f));
    if (!nearlyEqual(core.laneDepth(kPage, kRow, kLane), 0.0f))
    {
        std::printf("FAIL: unavailable external-audio lane accepted ParamTurn\n");
        return false;
    }
    return true;
}

bool test_cv_lane_history_store()
{
    CvLaneHistoryStore store;
    if (store.bufferSize() != CvLaneHistoryStore::kBufferSize
        || CvLaneHistoryStore::kNumLanes != 15
        || CvLaneHistoryStore::kBufferSize != CvScopeDisplay::kBufferSize)
    {
        std::printf("FAIL: CvLaneHistoryStore sizing mismatch\n");
        return false;
    }

    store.pushLaneSample(0, -0.5f);
    store.pushLaneSample(0, 1.5f);
    store.pushLaneSample(0, 0.25f);
    if (!nearlyEqual(store.latest(0), 0.25f))
    {
        std::printf("FAIL: store latest after clamp/push got %f\n", store.latest(0));
        return false;
    }
    if (store.samples(0) == nullptr || store.writeIndex(0) == 0)
    {
        std::printf("FAIL: store samples/writeIndex not advanced\n");
        return false;
    }
    return true;
}

bool test_underlay_bind_and_target_clear()
{
    FroggersV2ControlCore core;
    openDetailGrid(core, 3, 0, 5);

    CvLaneHistoryStore store;
    for (uint8_t lane = 0; lane < CvLaneHistoryStore::kNumLanes; ++lane)
    {
        store.pushLaneSample(lane, 0.1f * static_cast<float>(lane));
    }

    std::array<EncoderRingComponent, froggers_v2::kUiSlots> rings{};
    desktop_v2::bindDetailUnderlays(rings, &core, &store, true);

    for (uint8_t lane = 0; lane < froggers_v2::kNumModSources; ++lane)
    {
        if (!rings[lane].hasUnderlay())
        {
            std::printf("FAIL: detail lane cell %u missing underlay\n",
                        static_cast<unsigned>(lane));
            return false;
        }
    }
    if (rings[15].hasUnderlay())
    {
        std::printf("FAIL: Target (Back) cell still has underlay\n");
        return false;
    }

    // Unavailable external-audio lane stays bound (display-only; grey chrome elsewhere).
    constexpr uint8_t kExtLane = manifest::kExternalAudioRateLane;
    if (!rings[kExtLane].hasUnderlay())
    {
        std::printf("FAIL: unavailable lane underlay not bound safely\n");
        return false;
    }

    desktop_v2::bindDetailUnderlays(rings, &core, &store, false);
    for (auto& ring : rings)
    {
        if (ring.hasUnderlay())
        {
            std::printf("FAIL: underlay not cleared when detail closed\n");
            return false;
        }
    }

    EncoderRingComponent standalone;
    float samples[4] = {0.0f, 0.5f, 1.0f, 0.25f};
    standalone.setUnderlay(samples, 4, 2, juce::Colours::orange);
    if (!standalone.hasUnderlay())
    {
        std::printf("FAIL: setUnderlay did not arm underlay\n");
        return false;
    }
    standalone.clearUnderlay();
    if (standalone.hasUnderlay())
    {
        std::printf("FAIL: clearUnderlay left underlay armed\n");
        return false;
    }
    return true;
}
} // namespace

int main()
{
    const bool ok = test_15_lane_rack() && test_16_cell_detail_grid() && test_cv_led_behavior()
                 && test_bipolar_depth() && test_unavailable_external_audio()
                 && test_vco_pair_bus_eligibility()
                 && test_multi_lane_effective_with_eligibility()
                 && test_unavailable_lane_not_editable()
                 && test_cv_lane_history_store()
                 && test_underlay_bind_and_target_clear();
    if (!ok)
    {
        return 1;
    }
    std::printf("PASS: ModSourceGrid tests\n");
    return 0;
}
