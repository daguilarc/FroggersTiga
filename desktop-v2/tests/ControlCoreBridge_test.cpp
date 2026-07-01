#include "DelayState.hpp"
#include "HostParameterInventoryV2.hpp"
#include "V2ModTapBank.hpp"
#include "V2ParamDisplayNames.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

using froggers_v2::FroggersV2ControlCore;
using froggers_v2::FroggersV2HostBridge;
using froggers_v2::MessageIn;

namespace
{
constexpr float kEps = 1.0e-5f;

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

    MessageIn shiftOn;
    shiftOn.type = MessageIn::Type::ShiftHeld;
    shiftOn.value = 1.0f;
    pushAndProcess(core, shiftOn);
    const float beforeShiftTurn = core.effectiveRow(0, 0).effective;
    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 1.0f));
    const float afterShiftTurn = core.effectiveRow(0, 0).effective;
    if (!nearlyEqual(beforeShiftTurn, afterShiftTurn))
    {
        std::printf("FAIL: shift-held turn changed parameter\n");
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

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 1.0f));
    MessageIn shiftOn;
    shiftOn.type = MessageIn::Type::ShiftHeld;
    shiftOn.value = 1.0f;
    pushAndProcess(core, shiftOn);

    MessageIn revertPress;
    revertPress.type = MessageIn::Type::ParamPress;
    revertPress.page = 0;
    revertPress.slot = 0;
    pushAndProcess(core, revertPress);
    const auto reverted = core.effectiveRow(0, 0);
    const float expectedDefault = HostParameterInventoryV2::pageKnobDefault(0, 0);
    if (!nearlyEqual(reverted.effective, expectedDefault, 2.0e-3f))
    {
        std::printf("FAIL: shift+press did not revert target parameter to default\n");
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

    MessageIn shiftOn;
    shiftOn.type = MessageIn::Type::ShiftHeld;
    shiftOn.value = 1.0f;
    pushAndProcess(core, shiftOn);
    MessageIn crunchyPress;
    crunchyPress.type = MessageIn::Type::ParamPress;
    crunchyPress.page = froggers_v2::kNumHostPages;
    crunchyPress.slot = 0;
    pushAndProcess(core, crunchyPress);
    if (!nearlyEqual(core.globalCrunchy(), 0.0f, 2.0e-3f))
    {
        std::printf("FAIL: shift+press did not reset crunchy to 0\n");
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

    SequencerStepSnapshot stepOne{};
    stepOne.sceneCenter[0][0][0] = 0.8f;
    stepOne.sceneCenter[0][0][1] = 0.2f;
    stepOne.sceneCenter[0][0][2] = 0.5f;
    stepOne.gestureWeight[0] = 1.0f;
    stepOne.hasData = true;
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

    host.m_pageManager.SetPageModSource(0, 0, 13);
    host.m_pageManager.SetPageModDepth(0, 0, 0.75f);
    bridge.syncFromHostModRoutes();

    if (core.assignedModSource(0, 0) != 6)
    {
        std::printf(
            "FAIL: syncFromHostModRoutes expected internal mod 6 got %u\n",
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
        if (hostMod >= V2ModTapBank::kFirstIndex && hostMod <= V2ModTapBank::kLastIndex)
        {
            expectedInternal = static_cast<uint8_t>(hostMod - V2ModTapBank::kFirstIndex);
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
    host.m_sequencer.setPatternLength(4);
    host.m_sequencer.m_playing = true;

    SequencerStepSnapshot stepOne{};
    stepOne.sceneCenter[0][0][0] = 0.75f;
    stepOne.sceneCenter[0][0][1] = 0.75f;
    stepOne.sceneCenter[0][0][2] = 0.75f;
    stepOne.hasData = true;
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

    SequencerStepSnapshot captured;
    core.captureSequencerStepSnapshot(captured);
    if (!captured.hasData)
    {
        std::printf("FAIL: capture did not set hasData\n");
        return false;
    }

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, -5.0f));
    core.applySequencerStepSnapshot(captured);

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

    seq.m_steps[3].sceneCenter[0][0][0] = 0.99f;
    seq.m_steps[3].gate = true;
    seq.m_steps[3].hasData = true;

    MessageIn reset;
    reset.type = MessageIn::Type::ResetSequencerStep;
    reset.slot = 3;
    pushAndProcess(core, reset);

    const float expected = HostParameterInventoryV2::pageKnobDefault(0, 0);
    if (!nearlyEqual(seq.m_steps[3].sceneCenter[0][0][0], expected, 2.0e-3f))
    {
        std::printf("FAIL: factory reset scene center\n");
        return false;
    }
    if (seq.m_steps[3].gate)
    {
        std::printf("FAIL: factory reset gate expected false\n");
        return false;
    }
    if (!seq.m_steps[3].hasData)
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
    seq.m_steps[2].sceneCenter[0][0][0] = 0.11f;
    seq.m_steps[2].hasData = true;

    MessageIn rand;
    rand.type = MessageIn::Type::RandSequencerStep;
    rand.slot = 2;
    rand.page = froggers_v2::kRandSeqScopeFullStep;
    pushAndProcess(core, rand);

    if (!seq.m_steps[2].hasData)
    {
        std::printf("FAIL: full-step randomize hasData\n");
        return false;
    }
    if (nearlyEqual(seq.m_steps[2].sceneCenter[0][0][0], 0.11f, 1.0e-4f))
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
    seq.setPatternLength(4);
    core.setSequencerState(&seq);

    seq.m_editStep = 1;
    seq.m_steps[1].hasData = false;
    seq.m_steps[2].hasData = false;
    seq.m_steps[3].hasData = true;
    seq.m_steps[3].sceneCenter[0][0][0] = 0.42f;

    MessageIn stepDice;
    stepDice.type = MessageIn::Type::RandSequencerStep;
    stepDice.slot = 1;
    stepDice.page = froggers_v2::kRandSeqScopeStep;
    pushAndProcess(core, stepDice);

    if (!seq.m_steps[1].hasData)
    {
        std::printf("FAIL: step dice did not write edit step\n");
        return false;
    }
    if (seq.m_steps[2].hasData)
    {
        std::printf("FAIL: step dice wrote non-edit step\n");
        return false;
    }

    MessageIn patternDice;
    patternDice.type = MessageIn::Type::RandSequencerStep;
    patternDice.page = froggers_v2::kRandSeqScopePattern;
    pushAndProcess(core, patternDice);

    if (!seq.m_steps[2].hasData)
    {
        std::printf("FAIL: pattern dice did not fill blank step 2\n");
        return false;
    }
    if (!nearlyEqual(seq.m_steps[3].sceneCenter[0][0][0], 0.42f, 1.0e-4f))
    {
        std::printf("FAIL: pattern dice overwrote non-blank step 3\n");
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

    host.SetPageParam(6, 0, 0.0f);
    host.SetPageParam(6, 1, 0.0f);

    constexpr size_t kBlock = 512;
    float in[kBlock] = {};
    float out[kBlock] = {};

    host.m_sequencer.m_playing = false;
    host.SetGate(false);
    for (int i = 0; i < 120; ++i)
    {
        host.ProcessBlock(in, out, kBlock);
    }
    if (host.m_engine.GetEnvelopeLevel() < 0.5f)
    {
        std::printf("FAIL: Pair-AR gate expected open when sequencer stopped\n");
        return false;
    }

    host.m_sequencer.m_playing = true;
    host.m_sequencer.m_playhead = 0;
    host.m_sequencer.m_steps[0].gate = false;
    host.SetGate(false);
    for (int i = 0; i < 240; ++i)
    {
        host.ProcessBlock(in, out, kBlock);
    }
    if (host.m_engine.GetEnvelopeLevel() > 0.1f)
    {
        std::printf("FAIL: Pair-AR gate expected closed while playing without step/MIDI gate\n");
        return false;
    }

    host.m_sequencer.m_steps[0].gate = true;
    for (int i = 0; i < 120; ++i)
    {
        host.ProcessBlock(in, out, kBlock);
    }
    if (host.m_engine.GetEnvelopeLevel() < 0.5f)
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

    constexpr uint8_t kPairArRows = 7;
    if (core.visibleCount() != kPairArRows)
    {
        std::printf(
            "FAIL: Pair-AR visibleCount expected %u got %u\n",
            static_cast<unsigned>(kPairArRows),
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
    host.m_sequencer.m_recordArm = true;
    host.m_sequencer.m_playhead = 2;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 8.0f));
    FroggersV2HostBridge bridge(core, host);
    core.setSequencerState(&host.m_sequencer);

    bridge.onSequencerStepAdvance();

    if (!host.m_sequencer.m_steps[2].hasData)
    {
        std::printf("FAIL: record capture did not set hasData\n");
        return false;
    }
    if (!nearlyEqual(host.m_sequencer.m_steps[2].sceneCenter[0][0][0], core.effectiveRow(0, 0).sceneLeft, 2.0e-3f))
    {
        std::printf("FAIL: record capture scene mismatch\n");
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
    if (!test_sequencer_record_capture())
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
