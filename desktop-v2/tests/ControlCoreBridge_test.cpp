#include "DelayState.hpp"
#include "HostParameterInventoryV2.hpp"
#include "PermanentModTapRack.hpp"
#include "V2ParamDisplayNames.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

using froggers_v2::FroggersV2ControlCore;
using froggers_v2::FroggersV2HostBridge;
using froggers_v2::MessageIn;

namespace
{
constexpr float kEps = 1.0e-5f;

float peakAbs(const float* samples, size_t count)
{
    float peak = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        peak = std::max(peak, std::fabs(samples[i]));
    }
    return peak;
}

bool nearlyEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

void pushAndProcess(FroggersV2ControlCore& core, const MessageIn& message)
{
    core.bus().push(message);
    core.processBus();
}

bool test_message_bus_and_ui_state()
{
    FroggersV2ControlCore core;
    const uint32_t startVersion = core.uiState().version.load(std::memory_order_acquire);
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 1.0f));
    const uint32_t endVersion = core.uiState().version.load(std::memory_order_acquire);
    if (endVersion <= startVersion)
    {
        std::printf("FAIL: ui state version did not advance\n");
        return false;
    }
    if (core.uiState().visibleCount.load(std::memory_order_acquire) == 0)
    {
        std::printf("FAIL: ui visible count is zero\n");
        return false;
    }
    return true;
}

bool test_scene_centers_seeded_from_defaults()
{
    FroggersV2ControlCore core;
    for (uint8_t row = 0; row <= 2; ++row)
    {
        const float expectedAudio = HostParameterInventoryV2::pageKnobDefault(0, row);
        const auto audioRow = core.effectiveRow(0, row);
        if (!nearlyEqual(audioRow.effective, expectedAudio, 2.0e-3f))
        {
            std::printf(
                "FAIL: audio row %u effective expected %f got %f\n",
                static_cast<unsigned>(row),
                expectedAudio,
                audioRow.effective);
            return false;
        }
    }
    for (uint8_t row = 0; row < 6; ++row)
    {
        const float expectedPairAr = HostParameterInventoryV2::pageKnobDefault(6, row);
        const auto pairArRow = core.effectiveRow(6, row);
        if (!nearlyEqual(pairArRow.effective, expectedPairAr, 2.0e-3f))
        {
            std::printf(
                "FAIL: Pair-AR row %u effective expected %f got %f\n",
                static_cast<unsigned>(row),
                expectedPairAr,
                pairArRow.effective);
            return false;
        }
    }

    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);
    bridge.syncToHost();
    const float expectedAudioRow0 = HostParameterInventoryV2::pageKnobDefault(0, 0);
    if (!nearlyEqual(host.GetPageParam(0, 0), expectedAudioRow0, 2.0e-3f))
    {
        std::printf("FAIL: bridge sync did not push seeded audio row to host\n");
        return false;
    }
    for (uint8_t i = 0; i < HostParameterInventoryV2::kMorphCount; ++i)
    {
        const float expectedMorph = HostParameterInventoryV2::vcoMorphDefault(i);
        if (!nearlyEqual(host.GetVcoMorph(i), expectedMorph, 2.0e-3f))
        {
            std::printf(
                "FAIL: VCO morph %u expected %f got %f after sync\n",
                static_cast<unsigned>(i),
                expectedMorph,
                host.GetVcoMorph(i));
            return false;
        }
    }
    return true;
}

bool test_scene_blend_gesture_shift_semantics()
{
    FroggersV2ControlCore core;

    MessageIn setRight;
    setRight.type = MessageIn::Type::SceneSelect;
    setRight.index = 2;
    pushAndProcess(core, setRight);

    MessageIn blendZero;
    blendZero.type = MessageIn::Type::SceneBlend;
    blendZero.value = 0.0f;
    pushAndProcess(core, blendZero);
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 1.0f));

    MessageIn blendOne;
    blendOne.type = MessageIn::Type::SceneBlend;
    blendOne.value = 1.0f;
    pushAndProcess(core, blendOne);
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 5.0f));

    const auto withBlend = core.effectiveRow(0, 0);
    if (!(withBlend.sceneRight > withBlend.sceneLeft))
    {
        std::printf("FAIL: scene right did not diverge from scene left\n");
        return false;
    }

    MessageIn selectGesture;
    selectGesture.type = MessageIn::Type::GestureSelect;
    selectGesture.index = 0;
    pushAndProcess(core, selectGesture);

    MessageIn gestureWeight;
    gestureWeight.type = MessageIn::Type::GestureWeight;
    gestureWeight.index = 0;
    gestureWeight.value = 1.0f;
    pushAndProcess(core, gestureWeight);
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 1.0f));
    const auto withGesture = core.effectiveRow(0, 0);
    if ((withGesture.gesturesMask & 0x1u) == 0)
    {
        std::printf("FAIL: gesture mask not set\n");
        return false;
    }
    return true;
}

bool test_interaction_matrix_revert_and_mod_view()
{
    FroggersV2ControlCore core;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 12.5f));

    MessageIn assign;
    assign.type = MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 0;
    pushAndProcess(core, assign);

    MessageIn press;
    press.type = MessageIn::Type::ParamPress;
    press.page = 0;
    press.slot = 0;
    pushAndProcess(core, press);
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != 0)
    {
        std::printf("FAIL: mod view did not open on assigned row\n");
        return false;
    }

    const float minBefore = core.effectiveRow(0, 0).arcMin;
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 1.0f));
    const float minAfter = core.effectiveRow(0, 0).arcMin;
    if (nearlyEqual(minBefore, minAfter))
    {
        std::printf("FAIL: mod depth lane did not change arc bounds\n");
        return false;
    }

    MessageIn closePress;
    closePress.type = MessageIn::Type::ParamPress;
    closePress.page = 0;
    closePress.slot = static_cast<uint8_t>(core.uiState().visibleCount.load(std::memory_order_acquire) - 1);
    pushAndProcess(core, closePress);
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: mod view did not close from target press\n");
        return false;
    }
    return true;
}

bool test_visible_rows_match_page()
{
    FroggersV2ControlCore core;

    constexpr uint8_t kAudioRows = 8;
    if (core.visibleCount() != kAudioRows)
    {
        std::printf(
            "FAIL: audio page visibleCount expected %u got %u\n",
            static_cast<unsigned>(kAudioRows),
            static_cast<unsigned>(core.visibleCount()));
        return false;
    }
    for (uint8_t slot = 0; slot < kAudioRows; ++slot)
    {
        if (core.visibleRowForSlot(slot) != slot)
        {
            std::printf(
                "FAIL: audio slot %u should map row %u\n",
                static_cast<unsigned>(slot),
                static_cast<unsigned>(slot));
            return false;
        }
    }

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = 1;
    pushAndProcess(core, selectPage);

    constexpr uint8_t kFilterRows = 10;
    if (core.visibleCount() != kFilterRows)
    {
        std::printf(
            "FAIL: filter page visibleCount expected %u got %u\n",
            static_cast<unsigned>(kFilterRows),
            static_cast<unsigned>(core.visibleCount()));
        return false;
    }
    for (uint8_t slot = 0; slot < kFilterRows; ++slot)
    {
        if (core.visibleRowForSlot(slot) != slot)
        {
            std::printf(
                "FAIL: filter slot %u should map row %u\n",
                static_cast<unsigned>(slot),
                static_cast<unsigned>(slot));
            return false;
        }
    }
    return true;
}

bool test_filter_ten_rows_no_bank_paging()
{
    FroggersV2ControlCore core;

    MessageIn selectFilter;
    selectFilter.type = MessageIn::Type::SelectPage;
    selectFilter.page = 1;
    pushAndProcess(core, selectFilter);

    constexpr uint8_t kFilterRows = 10;
    if (core.visibleCount() != kFilterRows)
    {
        std::printf(
            "FAIL: filter visibleCount expected %u got %u\n",
            static_cast<unsigned>(kFilterRows),
            static_cast<unsigned>(core.visibleCount()));
        return false;
    }
    for (uint8_t slot = 0; slot < kFilterRows; ++slot)
    {
        if (core.visibleRowForSlot(slot) != slot)
        {
            std::printf(
                "FAIL: filter slot %u should map row %u\n",
                static_cast<unsigned>(slot),
                static_cast<unsigned>(slot));
            return false;
        }
    }

    const int docH = DesktopV2ChromeLayout::encoderDocumentHeight(kFilterRows);
    if (docH != DesktopV2ChromeLayout::gridPx(50))
    {
        std::printf("FAIL: filter document height expected %d got %d\n", DesktopV2ChromeLayout::gridPx(50), docH);
        return false;
    }
    return true;
}

bool test_viewport_scroll_reaches_row_nine()
{
    constexpr uint8_t kFilterRows = 10;
    constexpr int kShortViewportRows = 5;
    const int docH = DesktopV2ChromeLayout::encoderDocumentHeight(kFilterRows);
    const int viewH = DesktopV2ChromeLayout::encoderDocumentHeight(kShortViewportRows);
    const int maxScrollY = docH - viewH;
    const int row9Top = 9 * DesktopV2ChromeLayout::kEncoderRowH;
    if (maxScrollY < row9Top - viewH + DesktopV2ChromeLayout::kEncoderRowH)
    {
        std::printf("FAIL: short viewport cannot scroll to row 9\n");
        return false;
    }
    if (maxScrollY != row9Top - viewH + DesktopV2ChromeLayout::kEncoderRowH)
    {
        std::printf("FAIL: max scroll Y mismatch for row 9 visibility\n");
        return false;
    }
    return true;
}

bool test_bridge_sync_and_rand_all_scope()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);

    FroggersV2HostBridge bridge(core, host);

    MessageIn setBlend;
    setBlend.type = MessageIn::Type::SceneBlend;
    setBlend.value = 0.5f;
    pushAndProcess(core, setBlend);
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 1.0f));

    MessageIn crunchyTurn = MessageIn::ParamTurn(froggers_v2::kNumHostPages, 0, 1.0f);
    pushAndProcess(core, crunchyTurn);
    bridge.syncToHost();

    const float expected = core.effectiveRow(0, 0).effective;
    const float bridged = host.GetPageParam(0, 0);
    if (!nearlyEqual(expected, bridged, 2.0e-3f))
    {
        std::printf("FAIL: bridge value mismatch expected=%f got=%f\n", expected, bridged);
        return false;
    }
    if (!nearlyEqual(host.GetGlobalCrunchy(), core.globalCrunchy(), 2.0e-3f))
    {
        std::printf("FAIL: global crunchy mismatch\n");
        return false;
    }

    MessageIn selectGesture;
    selectGesture.type = MessageIn::Type::GestureSelect;
    selectGesture.index = 1;
    pushAndProcess(core, selectGesture);

    const uint8_t leftBefore = core.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightBefore = core.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendBefore = core.uiState().sceneBlend.load(std::memory_order_acquire);
    MessageIn randAll;
    randAll.type = MessageIn::Type::RandAll;
    pushAndProcess(core, randAll);
    if (core.uiState().activeGesture.load(std::memory_order_acquire) != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: rand all did not clear gesture selection\n");
        return false;
    }
    const uint8_t leftAfter = core.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightAfter = core.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendAfter = core.uiState().sceneBlend.load(std::memory_order_acquire);
    const bool metadataChanged = leftAfter != leftBefore || rightAfter != rightBefore
                                 || !nearlyEqual(blendAfter, blendBefore);
    if (!metadataChanged)
    {
        std::printf("FAIL: rand all did not randomize scene endpoints or blend\n");
        return false;
    }
    if (nearlyEqual(core.globalCrunchy(), 0.0f))
    {
        std::printf("FAIL: rand all did not randomize crunchy scene slots\n");
        return false;
    }
    return true;
}

bool test_sync_to_host_reaches_non_active_page()
{
    // Packet 3 regression coverage: syncToHost must push every page's rows to
    // the host every frame, not only the carousel's currently active page.
    // Before the fix, a Drive/Filter (or any non-visible page) change would
    // sit in FroggersV2ControlCore until the operator happened to revisit
    // that page, silently failing to reach audio in the meantime.
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);

    FroggersV2HostBridge bridge(core, host);
    bridge.syncToHost();

    constexpr uint8_t kNonActivePage = 4; // "Drive" page (manifest page name table).
    constexpr uint8_t kRow = 0;
    if (core.activePage() != 0)
    {
        std::printf("FAIL: test precondition violated, expected default active page 0\n");
        return false;
    }

    const float before = host.GetPageParam(kNonActivePage, kRow);

    // Edit a row on a page that is never selected active (no SelectPage message
    // is ever pushed), mirroring turning a Drive/Filter knob while looking at
    // a different carousel page.
    pushAndProcess(core, MessageIn::ParamTurn(kNonActivePage, kRow, 10.0f));

    if (core.activePage() != 0)
    {
        std::printf("FAIL: editing a non-active page must not switch the active carousel page\n");
        return false;
    }

    bridge.syncToHost();

    const float expected = core.effectiveRow(kNonActivePage, kRow).effective;
    const float actual = host.GetPageParam(kNonActivePage, kRow);
    if (!nearlyEqual(actual, expected, 2.0e-3f))
    {
        std::printf(
            "FAIL: non-active page row did not reach host: expected=%f got=%f\n",
            expected,
            actual);
        return false;
    }
    if (nearlyEqual(actual, before))
    {
        std::printf(
            "FAIL: non-active page row unchanged by sync (before=%f after=%f)\n",
            before,
            actual);
        return false;
    }
    return true;
}

bool test_rand_page_updates_scenes()
{
    FroggersV2ControlCore core;

    const uint8_t leftBefore = core.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightBefore = core.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendBefore = core.uiState().sceneBlend.load(std::memory_order_acquire);
    const float audioBefore = core.effectiveRow(0, 0).effective;

    MessageIn randPage;
    randPage.type = MessageIn::Type::RandPage;
    randPage.page = 1;
    pushAndProcess(core, randPage);

    if (core.uiState().leftSceneOrdinal.load(std::memory_order_acquire) != leftBefore
        || core.uiState().rightSceneOrdinal.load(std::memory_order_acquire) != rightBefore
        || !nearlyEqual(core.uiState().sceneBlend.load(std::memory_order_acquire), blendBefore))
    {
        std::printf("FAIL: rand page changed scene endpoints or blend\n");
        return false;
    }
    if (nearlyEqual(core.effectiveRow(1, 0).effective, 0.5f, 2.0e-3f))
    {
        std::printf("FAIL: rand page did not update filter row 0 scenes\n");
        return false;
    }
    if (!nearlyEqual(core.effectiveRow(0, 0).effective, audioBefore, 2.0e-3f))
    {
        std::printf("FAIL: rand page changed a different page\n");
        return false;
    }
    return true;
}

bool test_crunchy_scene_encoder_parity()
{
    FroggersV2ControlCore core;
    if (!nearlyEqual(core.globalCrunchy(), 0.0f))
    {
        std::printf("FAIL: crunchy factory default expected 0\n");
        return false;
    }

    MessageIn blendZero;
    blendZero.type = MessageIn::Type::SceneBlend;
    blendZero.value = 0.0f;
    pushAndProcess(core, blendZero);
    pushAndProcess(core, MessageIn::ParamTurn(froggers_v2::kNumHostPages, 0, 10.0f));
    const float atBlendZero = core.globalCrunchy();
    if (!(atBlendZero > 0.05f))
    {
        std::printf("FAIL: crunchy ring turn did not edit active scene slot\n");
        return false;
    }

    MessageIn blendOne;
    blendOne.type = MessageIn::Type::SceneBlend;
    blendOne.value = 1.0f;
    pushAndProcess(core, blendOne);
    const float atBlendOne = core.globalCrunchy();
    if (!nearlyEqual(atBlendOne, 0.0f, 2.0e-3f))
    {
        std::printf("FAIL: crunchy blend morph did not follow right scene slot\n");
        return false;
    }

    MessageIn randAll;
    randAll.type = MessageIn::Type::RandAll;
    pushAndProcess(core, randAll);
    if (nearlyEqual(core.globalCrunchy(), 0.0f))
    {
        std::printf("FAIL: rand all did not randomize crunchy slots\n");
        return false;
    }
    return true;
}

bool test_sequencer_clock_recall_and_bridge_sync()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);

    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);

    SequencerSlotPayload stepOne{};
    stepOne.sceneCenter[0][0][0] = 0.8f;
    stepOne.sceneCenter[0][0][1] = 0.2f;
    stepOne.sceneCenter[0][0][2] = 0.5f;
    stepOne.gestureWeight[0] = 1.0f;
    
    host.m_sequencer.captureStep(1, stepOne);
    host.m_sequencer.m_playhead = 1;

    bridge.onSequencerStepAdvance();

    const auto recalled = core.effectiveRow(0, 0);
    if (!nearlyEqual(recalled.sceneLeft, 0.8f, 2.0e-3f))
    {
        std::printf("FAIL: sequencer recall scene left expected 0.8 got %f\n", recalled.sceneLeft);
        return false;
    }
    if (!nearlyEqual(host.GetPageParam(0, 0), recalled.effective, 1.0e-2f))
    {
        std::printf("FAIL: sequencer recall did not sync to host\n");
        return false;
    }
    return true;
}

bool test_rand_mod_syncs_host_mod_routes()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);

    host.m_pageManager.SetPageModSource(0, 0, 11);
    host.m_pageManager.SetPageModDepth(0, 0, 0.75f);
    bridge.syncFromHostModRoutes();

    if (core.assignedModSource(0, 0) != 11)
    {
        std::printf(
            "FAIL: syncFromHostModRoutes expected internal mod 11 got %u\n",
            static_cast<unsigned>(core.assignedModSource(0, 0)));
        return false;
    }
    if (!nearlyEqual(core.assignedModDepth(0, 0), 0.75f, 2.0e-3f))
    {
        std::printf(
            "FAIL: syncFromHostModRoutes expected depth 0.75 got %f\n",
            core.assignedModDepth(0, 0));
        return false;
    }

    host.EnqueueRandomizePanelMod(0);
    host.DrainPendingMutations();
    bridge.syncFromHostModRoutes();

    for (uint8_t row = 0; row < HostParameterInventoryV2::rowsForUiPage(0); ++row)
    {
        const uint8_t hostMod = host.GetPageModSource(0, row);
        uint8_t expectedInternal = froggers_v2::kNoSelection;
        if (hostMod < froggers_v2::kNumModSources)
        {
            expectedInternal = hostMod;
        }
        if (core.assignedModSource(0, row) != expectedInternal)
        {
            std::printf(
                "FAIL: Rand Mod row %u core mod %u expected %u from host %u\n",
                static_cast<unsigned>(row),
                static_cast<unsigned>(core.assignedModSource(0, row)),
                static_cast<unsigned>(expectedInternal),
                static_cast<unsigned>(hostMod));
            return false;
        }
    }
    return true;
}

bool test_sequencer_clock_via_host_callback()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.setBpm(120.0f);
    host.m_sequencer.m_playing = true;

    SequencerSlotPayload stepOne{};
    stepOne.sceneCenter[0][0][0] = 0.75f;
    stepOne.sceneCenter[0][0][1] = 0.75f;
    stepOne.sceneCenter[0][0][2] = 0.75f;
    
    host.m_sequencer.captureStep(1, stepOne);

    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);

    if (!host.m_sequencer.advanceOnSamples(22050, 44100.0f))
    {
        std::printf("FAIL: sequencer did not advance on samples\n");
        return false;
    }
    if (!host.m_onSequencerStepAdvance)
    {
        std::printf("FAIL: host sequencer callback not registered\n");
        return false;
    }
    host.m_onSequencerStepAdvance();
    if (host.m_sequencer.m_playhead != 1)
    {
        std::printf("FAIL: playhead expected 1 after advance\n");
        return false;
    }
    if (!nearlyEqual(core.effectiveRow(0, 0).sceneLeft, 0.75f, 2.0e-3f))
    {
        std::printf("FAIL: host callback did not recall snapshot\n");
        return false;
    }
    return true;
}

bool test_sequencer_snapshot_round_trip()
{
    FroggersV2ControlCore core;
    core.setGestureWeight(0, 0.6f);
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 5.0f));

    SequencerSlotPayload captured;
    core.captureSequencerSlotPayload(captured);
    if (!captured.gate)
    {
        std::printf("FAIL: capture did not set hasData\n");
        return false;
    }

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, -5.0f));
    core.applySequencerSlotPayload(captured);

    const auto row = core.effectiveRow(0, 0);
    if (!nearlyEqual(row.sceneLeft, captured.sceneCenter[0][0][0], 2.0e-3f))
    {
        std::printf("FAIL: snapshot round-trip scene mismatch\n");
        return false;
    }
    if (!nearlyEqual(core.gestureWeight(0), 0.6f, 2.0e-3f))
    {
        std::printf("FAIL: snapshot round-trip gesture mismatch\n");
        return false;
    }
    return true;
}

bool test_sequencer_factory_reset()
{
    FroggersV2ControlCore core;
    SequencerState seq;
    core.setSequencerState(&seq);

    seq.m_slots[3].payload.sceneCenter[0][0][0] = 0.99f;
    seq.m_slots[3].payload.gate = true;
    seq.m_slots[3].written = true;

    MessageIn reset;
    reset.type = MessageIn::Type::ResetSequencerStep;
    reset.slot = 3;
    pushAndProcess(core, reset);

    const float expected = HostParameterInventoryV2::pageKnobDefault(0, 0);
    if (!nearlyEqual(seq.m_slots[3].payload.sceneCenter[0][0][0], expected, 2.0e-3f))
    {
        std::printf("FAIL: factory reset scene center\n");
        return false;
    }
    if (seq.m_slots[3].payload.gate)
    {
        std::printf("FAIL: factory reset gate expected false\n");
        return false;
    }
    if (!seq.m_slots[3].written)
    {
        std::printf("FAIL: factory reset hasData expected true\n");
        return false;
    }
    return true;
}

bool test_sequencer_full_step_randomize()
{
    FroggersV2ControlCore core;
    SequencerState seq;
    core.setSequencerState(&seq);

    const float blendBefore = core.sceneBlend();
    seq.m_slots[2].payload.sceneCenter[0][0][0] = 0.11f;
    seq.m_slots[2].written = true;

    MessageIn rand;
    rand.type = MessageIn::Type::RandSequencerStep;
    rand.slot = 2;
    rand.page = froggers_v2::kRandSeqScopeFullStep;
    pushAndProcess(core, rand);

    if (!seq.m_slots[2].written)
    {
        std::printf("FAIL: full-step randomize hasData\n");
        return false;
    }
    if (nearlyEqual(seq.m_slots[2].payload.sceneCenter[0][0][0], 0.11f, 1.0e-4f))
    {
        std::printf("FAIL: full-step randomize did not change scene slot\n");
        return false;
    }
    if (!nearlyEqual(core.sceneBlend(), blendBefore, 1.0e-4f))
    {
        std::printf("FAIL: full-step randomize changed live scene blend\n");
        return false;
    }
    return true;
}

bool test_sequencer_dice_step_and_pattern()
{
    FroggersV2ControlCore core;
    SequencerState seq;
    core.setSequencerState(&seq);

    seq.m_editStep = 1;
    seq.m_slots[1].written = false;
    seq.m_slots[2].written = false;
    seq.m_slots[3].written = true;
    seq.m_slots[3].payload.sceneCenter[0][0][0] = 0.42f;

    MessageIn stepDice;
    stepDice.type = MessageIn::Type::RandSequencerStep;
    stepDice.slot = 1;
    stepDice.page = froggers_v2::kRandSeqScopeStep;
    pushAndProcess(core, stepDice);

    if (!seq.m_slots[1].written)
    {
        std::printf("FAIL: step dice did not write edit step\n");
        return false;
    }
    if (seq.m_slots[2].written)
    {
        std::printf("FAIL: step dice wrote non-edit step\n");
        return false;
    }

    MessageIn patternDice;
    patternDice.type = MessageIn::Type::RandSequencerStep;
    patternDice.page = froggers_v2::kRandSeqScopePattern;
    pushAndProcess(core, patternDice);

    // Packet 5 / "All Steps randomization writes all written steps": Pattern
    // scope must only touch already-written steps. Step 2 is still blank at
    // this point (the earlier step-dice wrote step 1, not step 2), so it must
    // stay unwritten rather than being silently filled in.
    if (seq.m_slots[2].written)
    {
        std::printf("FAIL: pattern dice wrote into previously-unwritten step 2\n");
        return false;
    }
    if (nearlyEqual(seq.m_slots[3].payload.sceneCenter[0][0][0], 0.42f, 1.0e-4f))
    {
        std::printf("FAIL: pattern dice did not overwrite already-written step 3\n");
        return false;
    }
    return true;
}

bool test_rand_seq_all_steps_scope_targets_only_written_steps()
{
    // Dedicated regression coverage for the onRandSequencerStep Pattern-scope
    // fix (desktop-v2-sequencer-operator-loop "All Steps randomization writes
    // all written steps"): unwritten slots must be skipped entirely -- not
    // randomized and not marked written -- while already-written slots are
    // re-randomized in place.
    FroggersV2ControlCore core;
    SequencerState seq;
    core.setSequencerState(&seq);

    seq.m_slots[0].written = true;
    seq.m_slots[0].payload.sceneCenter[0][0][0] = 0.33f;
    seq.m_slots[1].written = false;
    seq.m_slots[1].payload.sceneCenter[0][0][0] = 0.77f;

    MessageIn patternDice;
    patternDice.type = MessageIn::Type::RandSequencerStep;
    patternDice.page = froggers_v2::kRandSeqScopePattern;
    pushAndProcess(core, patternDice);

    if (nearlyEqual(seq.m_slots[0].payload.sceneCenter[0][0][0], 0.33f, 1.0e-4f))
    {
        std::printf("FAIL: All Steps rand-seq did not re-randomize written step 0\n");
        return false;
    }
    if (seq.m_slots[1].written)
    {
        std::printf("FAIL: All Steps rand-seq marked an unwritten step written\n");
        return false;
    }
    if (!nearlyEqual(seq.m_slots[1].payload.sceneCenter[0][0][0], 0.77f, 1.0e-4f))
    {
        std::printf("FAIL: All Steps rand-seq mutated payload of an unwritten step\n");
        return false;
    }
    return true;
}

bool test_pair_ar_gate_policy()
{
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);

    host.SetPageKnob(6, 0, 0.5f);
    host.SetPageKnob(6, 1, 0.5f);

    constexpr size_t kBlock = 512;
    float in[kBlock] = {};
    float out[kBlock] = {};

    host.m_sequencer.m_playing = false;
    host.SetGate(true);
    for (int i = 0; i < 120; ++i)
    {
        host.ProcessBlock(in, out, kBlock);
    }
    const float stoppedPeak = peakAbs(out, kBlock);
    if (stoppedPeak < 0.01f)
    {
        std::printf("FAIL: Pair-AR gate expected open when sequencer stopped\n");
        return false;
    }

    host.m_sequencer.m_playing = true;
    host.m_sequencer.m_playhead = 0;
    host.m_sequencer.m_slots[0].written = true;
    host.m_sequencer.m_slots[0].payload.gate = false;
    host.SetGate(false);
    for (int i = 0; i < 240; ++i)
    {
        host.ProcessBlock(in, out, kBlock);
    }
    const float gatedPeak = peakAbs(out, kBlock);
    if (gatedPeak > stoppedPeak * 0.25f)
    {
        std::printf("FAIL: Pair-AR gate expected closed while playing without step/MIDI gate\n");
        return false;
    }

    host.m_sequencer.m_slots[0].payload.gate = true;
    for (int i = 0; i < 480; ++i)
    {
        host.ProcessBlock(in, out, kBlock);
    }
    if (peakAbs(out, kBlock) < gatedPeak * 2.0f)
    {
        std::printf("FAIL: Pair-AR gate expected open from active step gate\n");
        return false;
    }
    return true;
}

bool test_pair_ar_page_seven_rows()
{
    FroggersV2ControlCore core;

    MessageIn selectPairAr;
    selectPairAr.type = MessageIn::Type::SelectPage;
    selectPairAr.page = 6;
    pushAndProcess(core, selectPairAr);

    const uint8_t pairArRowCount = HostParameterInventoryV2::rowsForUiPage(6);
    if (core.visibleCount() != pairArRowCount)
    {
        std::printf(
            "FAIL: Pair-AR visibleCount expected %u got %u\n",
            static_cast<unsigned>(pairArRowCount),
            static_cast<unsigned>(core.visibleCount()));
        return false;
    }
    if (std::strcmp(V2ParamDisplayNames::forHostPage(6), "Pair-AR") != 0)
    {
        std::printf("FAIL: page 6 carousel label expected Pair-AR\n");
        return false;
    }
    return true;
}

bool test_sequencer_record_capture()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_writeSeqArm = true;
    host.m_sequencer.m_playhead = 1;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 8.0f));
    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);
    const float expectedScene = core.effectiveRow(0, 0).sceneLeft;

    bridge.onSequencerStepAdvance();

    if (!host.m_sequencer.m_slots[0].written)
    {
        std::printf("FAIL: record capture did not set hasData on step left\n");
        return false;
    }
    if (!nearlyEqual(host.m_sequencer.m_slots[0].payload.sceneCenter[0][0][0], expectedScene, 2.0e-3f))
    {
        std::printf("FAIL: record capture scene mismatch on step left\n");
        return false;
    }
    return true;
}

bool test_write_seq_step_zero_on_first_beat()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_writeSeqArm = true;
    host.m_sequencer.m_playhead = 1;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 6.0f));
    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);
    const float expectedScene = core.effectiveRow(0, 0).sceneLeft;

    bridge.onSequencerStepAdvance();

    if (!host.m_sequencer.m_slots[0].written)
    {
        std::printf("FAIL: first beat did not capture step 0\n");
        return false;
    }
    const float captured = host.m_sequencer.m_slots[0].payload.sceneCenter[0][0][0];
    if (!nearlyEqual(captured, expectedScene, 2.0e-3f))
    {
        std::printf("FAIL: first beat step 0 capture expected %f got %f\n", expectedScene, captured);
        return false;
    }
    return true;
}

bool test_write_seq_step_zero_on_start_sequence()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_writeSeqArm = true;
    host.m_sequencer.m_playhead = 0;
    host.m_sequencer.m_playing = false;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 7.0f));
    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);
    const float expectedScene = core.effectiveRow(0, 0).sceneLeft;

    bridge.captureLiveToSequencerStep(host.m_sequencer.m_playhead);

    if (!host.m_sequencer.m_slots[0].written)
    {
        std::printf("FAIL: Start Sequence capture did not set hasData on step 0\n");
        return false;
    }
    if (!nearlyEqual(host.m_sequencer.m_slots[0].payload.sceneCenter[0][0][0], expectedScene, 2.0e-3f))
    {
        std::printf("FAIL: Start Sequence capture scene mismatch on step 0\n");
        return false;
    }
    return true;
}

bool test_write_seq_stopped_navigate_save()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_writeSeqArm = true;
    host.m_sequencer.m_editStep = 2;
    host.m_sequencer.m_playing = false;

    host.m_sequencer.m_slots[3].payload.sceneCenter[0][0][0] = 0.25f;
    host.m_sequencer.m_slots[3].written = true;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 8.0f));
    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);
    const float expectedScene = core.effectiveRow(0, 0).sceneLeft;

    bridge.captureLiveToSequencerStep(2);
    host.m_sequencer.m_editStep = 3;
    bridge.recallSequencerStep(3);

    if (!host.m_sequencer.m_slots[2].written)
    {
        std::printf("FAIL: stopped navigate did not save previous edit step\n");
        return false;
    }
    if (!nearlyEqual(host.m_sequencer.m_slots[2].payload.sceneCenter[0][0][0], expectedScene, 2.0e-3f))
    {
        std::printf(
            "FAIL: stopped navigate saved scene expected %f got %f\n",
            expectedScene,
            host.m_sequencer.m_slots[2].payload.sceneCenter[0][0][0]);
        return false;
    }
    if (!nearlyEqual(core.effectiveRow(0, 0).sceneLeft, 0.25f, 2.0e-3f))
    {
        std::printf("FAIL: stopped navigate did not recall new edit step\n");
        return false;
    }
    return true;
}

bool test_write_seq_three_beats_steps_without_duplicate_step0()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_writeSeqArm = true;
    host.m_sequencer.m_playing = true;
    host.m_sequencer.m_playhead = 0;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 5.0f));
    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);
    const float expectedStep0 = core.effectiveRow(0, 0).sceneLeft;

    bridge.captureLiveToSequencerStep(0);
    host.m_sequencer.m_writeSeqJustStarted = true;

    for (int beat = 0; beat < 3; ++beat)
    {
        host.m_sequencer.m_playhead = static_cast<uint8_t>((host.m_sequencer.m_playhead + 1u) % 16u);
        bridge.onSequencerStepAdvance();
    }

    for (uint8_t step = 0; step < 3; ++step)
    {
        if (!host.m_sequencer.m_slots[step].written)
        {
            std::printf("FAIL: three-beat write seq step %u missing hasData\n", static_cast<unsigned>(step));
            return false;
        }
    }
    if (!nearlyEqual(host.m_sequencer.m_slots[0].payload.sceneCenter[0][0][0], expectedStep0, 2.0e-3f))
    {
        std::printf("FAIL: step 0 capture overwritten on first beat\n");
        return false;
    }
    return true;
}

bool test_unwritten_step_noop_on_advance()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_playhead = 2;
    host.m_sequencer.m_slots[2].written = false;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 8.0f));
    const float liveBefore = core.effectiveRow(0, 0).sceneLeft;

    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);
    bridge.onSequencerStepAdvance();

    if (host.m_sequencer.m_slots[2].written)
    {
        std::printf("FAIL: unwritten step should stay unwritten on advance\n");
        return false;
    }
    if (!nearlyEqual(core.effectiveRow(0, 0).sceneLeft, liveBefore, 2.0e-3f))
    {
        std::printf("FAIL: unwritten step advance changed live audio state\n");
        return false;
    }
    return true;
}

bool test_playback_skips_unwritten_steps()
{
    // desktop-v2-sequencing "Cleared steps are skipped by playback" /
    // "Written-step mask creates odd effective lengths": with Write Seq. not
    // armed, the playhead ring must skip past unwritten slots and land on
    // the next written one, applying that step's snapshot.
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_playing = true;
    host.m_sequencer.m_playhead = 0;

    SequencerSlotPayload stepFive{};
    stepFive.sceneCenter[0][0][0] = 0.6f;
    stepFive.sceneCenter[0][0][1] = 0.6f;
    stepFive.sceneCenter[0][0][2] = 0.6f;
    host.m_sequencer.captureStep(5, stepFive);

    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);

    bridge.onSequencerStepAdvance();

    if (host.m_sequencer.m_playhead != 5)
    {
        std::printf(
            "FAIL: playback did not skip unwritten steps, playhead expected 5 got %u\n",
            static_cast<unsigned>(host.m_sequencer.m_playhead));
        return false;
    }
    if (!nearlyEqual(core.effectiveRow(0, 0).sceneLeft, 0.6f, 2.0e-3f))
    {
        std::printf("FAIL: playback did not apply the written step landed on after skipping\n");
        return false;
    }
    return true;
}

bool test_empty_pattern_playback_is_transport_noop()
{
    // desktop-v2-sequencing "Empty written-step mask is transport no-op":
    // with every one of the 16 slots unwritten, playback SHALL emit no
    // snapshot recalls/gates/resets and MUST NOT hang trying to find a
    // written step that does not exist.
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    host.m_sequencer.m_playing = true;
    host.m_sequencer.m_playhead = 0;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 8.0f));
    const float liveBefore = core.effectiveRow(0, 0).sceneLeft;

    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);

    for (int beat = 0; beat < 3; ++beat)
    {
        bridge.onSequencerStepAdvance();
    }

    for (uint8_t step = 0; step < SequencerState::kSlotCount; ++step)
    {
        if (host.m_sequencer.m_slots[step].written)
        {
            std::printf("FAIL: empty pattern playback wrote step %u\n", static_cast<unsigned>(step));
            return false;
        }
    }
    if (!nearlyEqual(core.effectiveRow(0, 0).sceneLeft, liveBefore, 2.0e-3f))
    {
        std::printf("FAIL: empty pattern playback changed live audio state\n");
        return false;
    }
    return true;
}

bool test_sequencer_16_slot_snapshot_and_lock_round_trip()
{
    // Task 5.4: exercises snapshot/lock round-trip across all 16 fixed
    // slots -- a mix of written and unwritten steps, plus a locked written
    // step -- rather than spot-checking one or two indices. desktop-v2-
    // sequencing "Fixed sixteen-step ring" / "Cleared steps are skipped by
    // playback" / lock-gated recall (FroggersV2HostBridge::onSequencerStepAdvance).
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);

    // Even slots written with distinct, deterministic values; odd slots left
    // unwritten. Slot 6 is additionally locked.
    for (uint8_t step = 0; step < SequencerState::kSlotCount; step += 2)
    {
        SequencerSlotPayload payload{};
        const float value = 0.05f * static_cast<float>(step + 1);
        payload.sceneCenter[0][0][0] = value;
        payload.sceneCenter[0][0][1] = value;
        payload.sceneCenter[0][0][2] = value;
        host.m_sequencer.captureStep(step, payload);
    }
    host.m_sequencer.m_slots[6].locked = true;

    for (uint8_t step = 0; step < SequencerState::kSlotCount; ++step)
    {
        const bool expectedWritten = (step % 2 == 0);
        if (host.m_sequencer.slotWritten(step) != expectedWritten)
        {
            std::printf(
                "FAIL: step %u written state expected %d got %d\n",
                static_cast<unsigned>(step),
                expectedWritten,
                host.m_sequencer.slotWritten(step));
            return false;
        }
    }

    host.m_sequencer.m_playing = true;
    host.m_sequencer.m_playhead = 15;

    std::array<bool, SequencerState::kSlotCount> visited{};
    for (int i = 0; i < SequencerState::kSlotCount; ++i)
    {
        // Manually perform the one-step base advance that advanceOnSamples()
        // would normally do before invoking the callback (matches the
        // existing test_write_seq_three_beats_steps_without_duplicate_step0
        // convention for driving onSequencerStepAdvance directly).
        host.m_sequencer.advancePlayhead();
        bridge.onSequencerStepAdvance();

        const uint8_t ph = host.m_sequencer.m_playhead;
        if (ph % 2 != 0)
        {
            std::printf("FAIL: playback landed on unwritten (odd) step %u\n", static_cast<unsigned>(ph));
            return false;
        }
        visited[ph] = true;

        if (ph == 6)
        {
            if (nearlyEqual(core.effectiveRow(0, 0).sceneLeft, 0.05f * 7.0f, 1.0e-4f))
            {
                std::printf("FAIL: locked step 6 was recalled despite being locked\n");
                return false;
            }
        }
        else
        {
            const float expected = 0.05f * static_cast<float>(ph + 1);
            if (!nearlyEqual(core.effectiveRow(0, 0).sceneLeft, expected, 2.0e-3f))
            {
                std::printf(
                    "FAIL: step %u recall mismatch expected %f got %f\n",
                    static_cast<unsigned>(ph),
                    expected,
                    core.effectiveRow(0, 0).sceneLeft);
                return false;
            }
        }
    }

    for (uint8_t step = 0; step < SequencerState::kSlotCount; step += 2)
    {
        if (!visited[step])
        {
            std::printf(
                "FAIL: written step %u never visited across a full ring loop\n",
                static_cast<unsigned>(step));
            return false;
        }
    }
    if (!host.m_sequencer.m_slots[6].written || !host.m_sequencer.m_slots[6].locked)
    {
        std::printf("FAIL: locked step 6 lost its written/locked state\n");
        return false;
    }
    if (!nearlyEqual(host.m_sequencer.m_slots[6].payload.sceneCenter[0][0][0], 0.05f * 7.0f, 1.0e-4f))
    {
        std::printf("FAIL: locked step 6 snapshot payload mutated by playback\n");
        return false;
    }
    return true;
}

bool test_rand_seq_playhead_target_while_playing()
{
    FroggersV2ControlCore core;
    SequencerState seq;
    seq.m_playing = true;
    seq.m_playhead = 3;
    seq.m_editStep = 0;
    core.setSequencerState(&seq);

    MessageIn stepDice;
    stepDice.type = MessageIn::Type::RandSequencerStep;
    stepDice.slot = 0;
    stepDice.page = froggers_v2::kRandSeqScopeStep;
    pushAndProcess(core, stepDice);

    if (!seq.m_slots[3].written)
    {
        std::printf("FAIL: playing rand-seq did not target playhead step\n");
        return false;
    }
    if (seq.m_slots[0].written)
    {
        std::printf("FAIL: playing rand-seq wrote edit step instead of playhead\n");
        return false;
    }
    return true;
}

bool test_rand_mods_per_step_snapshots()
{
    FroggersV2ControlCore core;
    SequencerState seq;
    seq.m_editStep = 2;
    core.setSequencerState(&seq);

    seq.m_slots[2].payload.modSource[0][0] = froggers_v2::kNoSelection;
    seq.m_slots[2].payload.modDepth[0][0] = 0.0f;

    MessageIn randMods;
    randMods.type = MessageIn::Type::RandSequencerMods;
    randMods.page = froggers_v2::kRandSeqScopeStep;
    pushAndProcess(core, randMods);

    if (!seq.m_slots[2].written)
    {
        std::printf("FAIL: rand mods did not mark step hasData\n");
        return false;
    }
    if (seq.m_slots[2].payload.modSource[0][0] == froggers_v2::kNoSelection
        && nearlyEqual(seq.m_slots[2].payload.modDepth[0][0], 0.0f))
    {
        std::printf("FAIL: rand mods did not write mod route into snapshot\n");
        return false;
    }
    return true;
}

// 4.8: single-authority randomization tests covering the executeRandomization
// adapter that GlobalStripV2 now routes through (see spec scenarios "Global
// Rand Mods randomizes live mod depths", "Global Rand Mods is not
// sequencer-only", "Rand All respects scene scope", "Rand Mods respects step
// scope").

bool test_rand_mods_changes_live_depths_without_sequencer()
{
    FroggersV2ControlCore core;

    // Assign live mod routes on two different pages -- including the Delay
    // page (kDelayUiPage == 5, HostParameterInventoryV2.hpp) -- matching an
    // operator who turned module rows onto a modulation source before
    // hitting Rand Mods. No sequencer is bound at all, which exercises
    // "Global Rand Mods is not sequencer-only": the live depths must still
    // randomize.
    MessageIn assignAudio;
    assignAudio.type = MessageIn::Type::ModSourceAssign;
    assignAudio.page = 0;
    assignAudio.slot = 0;
    assignAudio.index = 2;
    pushAndProcess(core, assignAudio);

    MessageIn assignDelay;
    assignDelay.type = MessageIn::Type::ModSourceAssign;
    assignDelay.page = 5;
    assignDelay.slot = 0;
    assignDelay.index = 3;
    pushAndProcess(core, assignDelay);

    const float audioDepthBefore = core.assignedModDepth(0, 0);
    const float delayDepthBefore = core.assignedModDepth(5, 0);

    core.executeRandomization(
        FroggersV2ControlCore::RandomizationCommand::RandMods, 0, froggers_v2::kRandSeqScopeStep);

    const float audioDepthAfterFirst = core.assignedModDepth(0, 0);
    const float delayDepthAfterFirst = core.assignedModDepth(5, 0);
    if (nearlyEqual(audioDepthAfterFirst, audioDepthBefore))
    {
        std::printf("FAIL: Rand Mods did not change the live mod depth on page 0 row 0\n");
        return false;
    }
    if (nearlyEqual(delayDepthAfterFirst, delayDepthBefore))
    {
        std::printf("FAIL: Rand Mods did not change the live mod depth on the Delay page (5)\n");
        return false;
    }

    // Revert round trip / no-gaming proof: a second Rand Mods call must move
    // the live depth again. A hardcoded return value or a "randomize the
    // snapshot only, leave live state as a copy" stub would produce the same
    // depth on both calls.
    core.executeRandomization(
        FroggersV2ControlCore::RandomizationCommand::RandMods, 0, froggers_v2::kRandSeqScopeStep);
    const float audioDepthAfterSecond = core.assignedModDepth(0, 0);
    if (nearlyEqual(audioDepthAfterSecond, audioDepthAfterFirst))
    {
        std::printf("FAIL: second Rand Mods call did not move the live depth again (looks gamed/cached)\n");
        return false;
    }
    return true;
}

bool test_rand_mods_step_scope_writes_only_written_steps()
{
    FroggersV2ControlCore core;
    SequencerState seq;
    seq.m_editStep = 2;
    core.setSequencerState(&seq);

    seq.m_slots[2].written = true;
    seq.m_slots[3].written = false;
    seq.m_slots[3].payload.modSource[0][0] = froggers_v2::kNoSelection;

    MessageIn assign;
    assign.type = MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 4;
    pushAndProcess(core, assign);

    core.executeRandomization(
        FroggersV2ControlCore::RandomizationCommand::RandMods, 0, froggers_v2::kRandSeqScopeStep);

    if (seq.m_slots[2].payload.modSource[0][0] != 4)
    {
        std::printf(
            "FAIL: Rand Mods (Current Step) did not capture the live mod route into the written edit step\n");
        return false;
    }
    if (seq.m_slots[3].written || seq.m_slots[3].payload.modSource[0][0] != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: Rand Mods (Current Step) touched an unwritten step\n");
        return false;
    }
    return true;
}

bool test_rand_mods_all_steps_scope_targets_only_written_steps()
{
    FroggersV2ControlCore core;
    SequencerState seq;
    core.setSequencerState(&seq);

    seq.m_slots[0].written = true;
    seq.m_slots[1].written = false;
    seq.m_slots[1].payload.modSource[0][0] = froggers_v2::kNoSelection;

    MessageIn assign;
    assign.type = MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 5;
    pushAndProcess(core, assign);

    core.executeRandomization(
        FroggersV2ControlCore::RandomizationCommand::RandMods, 0, froggers_v2::kRandSeqScopePattern);

    if (seq.m_slots[0].payload.modSource[0][0] != 5)
    {
        std::printf("FAIL: All Steps Rand Mods did not write the live mod route into written step 0\n");
        return false;
    }
    if (seq.m_slots[1].written || seq.m_slots[1].payload.modSource[0][0] != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: All Steps Rand Mods wrote into (or marked written) an unwritten step\n");
        return false;
    }
    return true;
}

bool test_rand_all_respects_scene_scope()
{
    FroggersV2ControlCore core;

    // Default left/right scene ordinals (0/1) and blend (0.5) resolve
    // activeSceneOrdinal() to scene index 1; scene index 0 is the
    // non-active slot that Current Scene scope must leave untouched.
    const uint8_t leftBefore = core.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightBefore = core.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendBefore = core.uiState().sceneBlend.load(std::memory_order_acquire);
    const float nonActiveSceneBefore = core.effectiveRow(0, 0).sceneLeft;
    const float activeSceneBefore = core.effectiveRow(0, 0).sceneRight;

    core.executeRandomization(
        FroggersV2ControlCore::RandomizationCommand::RandAll, froggers_v2::kRandSceneScopeCurrent, 0);

    const uint8_t leftAfter = core.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightAfter = core.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendAfter = core.uiState().sceneBlend.load(std::memory_order_acquire);
    if (leftAfter != leftBefore || rightAfter != rightBefore || !nearlyEqual(blendAfter, blendBefore))
    {
        std::printf("FAIL: Current Scene Rand All changed the scene endpoints or blend\n");
        return false;
    }
    const float nonActiveSceneAfter = core.effectiveRow(0, 0).sceneLeft;
    if (!nearlyEqual(nonActiveSceneAfter, nonActiveSceneBefore))
    {
        std::printf("FAIL: Current Scene Rand All mutated the non-active scene slot\n");
        return false;
    }
    const float activeSceneAfter = core.effectiveRow(0, 0).sceneRight;
    if (nearlyEqual(activeSceneAfter, activeSceneBefore))
    {
        std::printf("FAIL: Current Scene Rand All did not randomize the active scene edit target\n");
        return false;
    }

    // All Scenes scope (the other radio state) must still move the scene
    // endpoints/blend, proving the scope radio genuinely changes behavior
    // rather than being decorative.
    core.executeRandomization(
        FroggersV2ControlCore::RandomizationCommand::RandAll, froggers_v2::kRandSceneScopeAll, 0);
    const uint8_t leftAfterAll = core.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightAfterAll = core.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendAfterAll = core.uiState().sceneBlend.load(std::memory_order_acquire);
    if (leftAfterAll == leftAfter && rightAfterAll == rightAfter && nearlyEqual(blendAfterAll, blendAfter))
    {
        std::printf("FAIL: All Scenes Rand All did not randomize the scene endpoints or blend\n");
        return false;
    }
    return true;
}
} // namespace

int main()
{
    if (!test_message_bus_and_ui_state())
    {
        return 1;
    }
    if (!test_scene_centers_seeded_from_defaults())
    {
        return 1;
    }
    if (!test_scene_blend_gesture_shift_semantics())
    {
        return 1;
    }
    if (!test_interaction_matrix_revert_and_mod_view())
    {
        return 1;
    }
    if (!test_visible_rows_match_page())
    {
        return 1;
    }
    if (!test_filter_ten_rows_no_bank_paging())
    {
        return 1;
    }
    if (!test_viewport_scroll_reaches_row_nine())
    {
        return 1;
    }
    if (!test_bridge_sync_and_rand_all_scope())
    {
        return 1;
    }
    if (!test_sync_to_host_reaches_non_active_page())
    {
        return 1;
    }
    if (!test_rand_page_updates_scenes())
    {
        return 1;
    }
    if (!test_crunchy_scene_encoder_parity())
    {
        return 1;
    }
    if (!test_sequencer_clock_recall_and_bridge_sync())
    {
        return 1;
    }
    if (!test_sequencer_clock_via_host_callback())
    {
        return 1;
    }
    if (!test_sequencer_snapshot_round_trip())
    {
        return 1;
    }
    if (!test_sequencer_factory_reset())
    {
        return 1;
    }
    if (!test_sequencer_full_step_randomize())
    {
        return 1;
    }
    if (!test_sequencer_dice_step_and_pattern())
    {
        return 1;
    }
    if (!test_rand_seq_all_steps_scope_targets_only_written_steps())
    {
        return 1;
    }
    if (!test_sequencer_record_capture())
    {
        return 1;
    }
    if (!test_write_seq_step_zero_on_first_beat())
    {
        return 1;
    }
    if (!test_write_seq_step_zero_on_start_sequence())
    {
        return 1;
    }
    if (!test_write_seq_stopped_navigate_save())
    {
        return 1;
    }
    if (!test_write_seq_three_beats_steps_without_duplicate_step0())
    {
        return 1;
    }
    if (!test_unwritten_step_noop_on_advance())
    {
        return 1;
    }
    if (!test_playback_skips_unwritten_steps())
    {
        return 1;
    }
    if (!test_empty_pattern_playback_is_transport_noop())
    {
        return 1;
    }
    if (!test_sequencer_16_slot_snapshot_and_lock_round_trip())
    {
        return 1;
    }
    if (!test_rand_seq_playhead_target_while_playing())
    {
        return 1;
    }
    if (!test_rand_mods_per_step_snapshots())
    {
        return 1;
    }
    if (!test_rand_mods_changes_live_depths_without_sequencer())
    {
        return 1;
    }
    if (!test_rand_mods_step_scope_writes_only_written_steps())
    {
        return 1;
    }
    if (!test_rand_mods_all_steps_scope_targets_only_written_steps())
    {
        return 1;
    }
    if (!test_rand_all_respects_scene_scope())
    {
        return 1;
    }
    if (!test_rand_mod_syncs_host_mod_routes())
    {
        return 1;
    }
    if (!test_pair_ar_gate_policy())
    {
        return 1;
    }
    if (!test_pair_ar_page_seven_rows())
    {
        return 1;
    }

    std::printf("PASS: ControlCoreBridge tests\n");
    return 0;
}
