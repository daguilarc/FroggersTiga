#include "DelayState.hpp"
#include "HostParameterInventoryV2.hpp"
#include "HostParameterRoutingV2.hpp"
#include "PermanentModTapRack.hpp"
#include "V2DesktopPageDisplayNames.hpp"
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
        const float expectedPairAr = HostParameterInventoryV2::pageKnobDefault(5, row);
        const auto pairArRow = core.effectiveRow(5, row);
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

bool test_scene_blend_semantics()
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
    return true;
}

bool test_interaction_matrix_revert_and_mod_view()
{
    FroggersV2ControlCore core;

    pushAndProcess(core, MessageIn::ParamTurn(0, 0, 12.5f));

    // Page 0 row 0: only pair-bus lane 1 is assignable among lanes 0–2.
    MessageIn assign;
    assign.type = MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 1;
    pushAndProcess(core, assign);

    pushAndProcess(core, MessageIn::ModDrillIn(0, 0));
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != 0)
    {
        std::printf("FAIL: mod view did not open on assigned row\n");
        return false;
    }

    const float minBefore = core.effectiveRow(0, 0).arcMin;
    pushAndProcess(core, MessageIn::ParamTurn(0, 1, 1.0f));
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

    // Task 7.4 (D11/D14): Cross-coupler row removed (3 pitch + 3 PM + Crispy).
    constexpr uint8_t kAudioRows = 7;
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

    const uint8_t leftBefore = core.uiState().leftSceneOrdinal.load(std::memory_order_acquire);
    const uint8_t rightBefore = core.uiState().rightSceneOrdinal.load(std::memory_order_acquire);
    const float blendBefore = core.uiState().sceneBlend.load(std::memory_order_acquire);
    MessageIn randAll;
    randAll.type = MessageIn::Type::RandAll;
    pushAndProcess(core, randAll);
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

    // "Drive" UI page after the Random deletion (0 Audio, 1 Reverb, 2 Filter,
    // 3 Drive, 4 Delay, 5 Pair-AR). Deliberately a non-Delay page so syncToHost
    // routes it through DesktopHostIO::SetPageKnob rather than DelayState. The
    // engine PageManager still carries the Marbles page at PM index 1, so the
    // host must be read at the mapped PM page, not the UI page.
    constexpr uint8_t kNonActiveUiPage = 3;
    const uint8_t kNonActivePmPage = HostParameterRoutingV2::pmPageForUiPage(kNonActiveUiPage);
    constexpr uint8_t kRow = 0;
    if (core.activePage() != 0)
    {
        std::printf("FAIL: test precondition violated, expected default active page 0\n");
        return false;
    }

    const float before = host.GetPageParam(kNonActivePmPage, kRow);

    // Edit a row on a page that is never selected active (no SelectPage message
    // is ever pushed), mirroring turning a Drive/Filter knob while looking at
    // a different carousel page.
    pushAndProcess(core, MessageIn::ParamTurn(kNonActiveUiPage, kRow, 10.0f));

    if (core.activePage() != 0)
    {
        std::printf("FAIL: editing a non-active page must not switch the active carousel page\n");
        return false;
    }

    bridge.syncToHost();

    const float expected = core.effectiveRow(kNonActiveUiPage, kRow).effective;
    const float actual = host.GetPageParam(kNonActivePmPage, kRow);
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

bool test_rand_mod_syncs_host_mod_routes()
{
    extern uint32_t RGen_s_state_probe;
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
        // Audio UI row >= 3 skips over the hidden-but-still-allocated engine
        // XCPL slot (task 7.4 / D11 / D14) -- translate before touching the
        // raw engine param, same as HostParameterRoutingV2's accessors do.
        const uint8_t hostMod =
            host.GetPageModSource(0, HostParameterRoutingV2::engineRowForUiRow(0, row));
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

bool test_envelope_page_ten_rows()
{
    // Task 7.5 renamed this page's carousel label "Pair-AR" -> "Envelope".
    // Row count widened 7 -> 10 same-day (D15 follow-up) when per-VCO Sustain
    // was added -- see V2DesktopPageDisplayNames.hpp's file-header note.
    FroggersV2ControlCore core;

    MessageIn selectEnvelope;
    selectEnvelope.type = MessageIn::Type::SelectPage;
    selectEnvelope.page = 5;
    pushAndProcess(core, selectEnvelope);

    const uint8_t envelopeRowCount = HostParameterInventoryV2::rowsForUiPage(5);
    if (core.visibleCount() != envelopeRowCount)
    {
        std::printf(
            "FAIL: Envelope visibleCount expected %u got %u\n",
            static_cast<unsigned>(envelopeRowCount),
            static_cast<unsigned>(core.visibleCount()));
        return false;
    }
    if (std::strcmp(V2DesktopPageDisplayNames::forHostPage(5), "Envelope") != 0)
    {
        std::printf("FAIL: page 5 carousel label expected Envelope\n");
        return false;
    }
    return true;
}

bool test_rand_mods_changes_live_depths_without_sequencer()
{
    FroggersV2ControlCore core;

    // Assign live mod routes on two different pages -- including the Delay
    // page (kDelayUiPage == 4, HostParameterInventoryV2.hpp) -- matching an
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
    assignDelay.page = 4;
    assignDelay.slot = 0;
    assignDelay.index = 3;
    pushAndProcess(core, assignDelay);

    const float audioDepthBefore = core.assignedModDepth(0, 0);
    const float delayDepthBefore = core.assignedModDepth(4, 0);

    core.executeRandomization(FroggersV2ControlCore::RandomizationCommand::RandMods, 0);

    const float audioDepthAfterFirst = core.assignedModDepth(0, 0);
    const float delayDepthAfterFirst = core.assignedModDepth(4, 0);
    if (nearlyEqual(audioDepthAfterFirst, audioDepthBefore))
    {
        std::printf("FAIL: Rand Mods did not change the live mod depth on page 0 row 0\n");
        return false;
    }
    if (nearlyEqual(delayDepthAfterFirst, delayDepthBefore))
    {
        std::printf("FAIL: Rand Mods did not change the live mod depth on the Delay page (4)\n");
        return false;
    }

    // Revert round trip / no-gaming proof: a second Rand Mods call must move
    // the live depth again. A hardcoded return value or a "randomize the
    // snapshot only, leave live state as a copy" stub would produce the same
    // depth on both calls.
    core.executeRandomization(FroggersV2ControlCore::RandomizationCommand::RandMods, 0);
    const float audioDepthAfterSecond = core.assignedModDepth(0, 0);
    if (nearlyEqual(audioDepthAfterSecond, audioDepthAfterFirst))
    {
        std::printf("FAIL: second Rand Mods call did not move the live depth again (looks gamed/cached)\n");
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
        FroggersV2ControlCore::RandomizationCommand::RandAll, froggers_v2::kRandSceneScopeCurrent);

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
        FroggersV2ControlCore::RandomizationCommand::RandAll, froggers_v2::kRandSceneScopeAll);
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

// Packet 15-B: plumb per-lane depths (FroggersV2ControlCore::laneDepth /
// DesktopHostIO::SetPageLaneDepth) into the V2-only additive multi-lane
// modulation store (sim/V2LaneDepthStore.hpp) so 15-A's engine-side apply
// (Page::GetPreFuegoValue) stops reading all zeros.
//
// Packet 15.2a collapsed ParamState to the lane identity model: modDepth[i]
// IS lane i's signed depth (the parallel modSource[15] identity array is
// retired), so every one of the kNumModSources lanes is independently
// addressable and any number may carry a non-zero depth on the same row at
// once. The two-different-rows setup below still holds as a places-each-lane-
// at-its-own-index proof; the genuinely simultaneous multi-lane-on-one-row
// case 15.2a unlocks is covered separately below
// (test_multiple_lanes_active_simultaneously_on_one_row and friends).

bool assignLiveModRoute(FroggersV2ControlCore& core, uint8_t page, uint8_t row, uint8_t lane, float turnSteps)
{
    MessageIn assign;
    assign.type = MessageIn::Type::ModSourceAssign;
    assign.page = page;
    assign.slot = row;
    assign.index = lane;
    pushAndProcess(core, assign);

    pushAndProcess(core, MessageIn::ModDrillIn(page, row));

    // While the detail (mod-view) grid is open, slot == lane addresses that
    // lane's cell directly (FroggersV2ControlCore::rebuildVisibleSlots).
    pushAndProcess(core, MessageIn::ParamTurn(page, lane, turnSteps));

    MessageIn closePress;
    closePress.type = MessageIn::Type::ParamPress;
    closePress.page = page;
    closePress.slot = static_cast<uint8_t>(core.uiState().visibleCount.load(std::memory_order_acquire) - 1);
    pushAndProcess(core, closePress);
    return true;
}

// Packet 15-B per-lane push: syncModRoutesToHost writes V2LaneDepthStore.
void triggerToHostModRouteSync(FroggersV2HostBridge& bridge, uint8_t /*sequencerSlot*/)
{
    bridge.syncModRoutesToHost();
}

bool test_v2_lane_depth_store_places_each_row_at_its_own_lane()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);

    constexpr uint8_t kPage = 1; // Filter: no VCO pair-bus eligibility special-case (that's page 0 only)
    constexpr uint8_t kRowA = 2;
    constexpr uint8_t kLaneA = 8; // lfo_1
    constexpr uint8_t kRowB = 3;
    constexpr uint8_t kLaneB = 10; // lfo_3

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    assignLiveModRoute(core, kPage, kRowA, kLaneA, 10.0f);  // depth ~ +0.4
    assignLiveModRoute(core, kPage, kRowB, kLaneB, -6.0f);  // depth ~ -0.24

    const float expectedDepthA = core.laneDepth(kPage, kRowA, kLaneA);
    const float expectedDepthB = core.laneDepth(kPage, kRowB, kLaneB);
    if (nearlyEqual(expectedDepthA, 0.0f) || nearlyEqual(expectedDepthB, 0.0f))
    {
        std::printf(
            "FAIL: test precondition, expected non-zero live lane depths (A=%f B=%f)\n",
            expectedDepthA,
            expectedDepthB);
        return false;
    }

    triggerToHostModRouteSync(bridge, 0);

    const uint8_t pmPage = HostParameterRoutingV2::pmPageForUiPage(kPage);
    const float storedA = host.m_v2LaneDepths.Get(pmPage, kRowA, kLaneA);
    const float storedB = host.m_v2LaneDepths.Get(pmPage, kRowB, kLaneB);
    if (!nearlyEqual(storedA, expectedDepthA, 2.0e-3f))
    {
        std::printf("FAIL: row A lane store expected %f got %f\n", expectedDepthA, storedA);
        return false;
    }
    if (!nearlyEqual(storedB, expectedDepthB, 2.0e-3f))
    {
        std::printf("FAIL: row B lane store expected %f got %f\n", expectedDepthB, storedB);
        return false;
    }
    // Each row's non-assigned lane slot must stay zero -- proves the per-lane
    // loop writes into the correct lane index per row, not a fixed lane 0 or
    // smeared across every lane.
    if (!nearlyEqual(host.m_v2LaneDepths.Get(pmPage, kRowA, kLaneB), 0.0f))
    {
        std::printf("FAIL: row A lane B slot expected 0\n");
        return false;
    }
    if (!nearlyEqual(host.m_v2LaneDepths.Get(pmPage, kRowB, kLaneA), 0.0f))
    {
        std::printf("FAIL: row B lane A slot expected 0\n");
        return false;
    }
    return true;
}

bool test_v2_lane_depth_store_zeros_ineligible_lane()
{
    // isModLaneAssignable rejects external-audio lanes (13/14) whenever
    // externalAudioAvailable() is false -- FroggersV2ControlCore's default.
    // ModSourceAssign itself doesn't enforce eligibility (that's a host-
    // store/UI concern per the manifest), so a live route can still point at
    // an unassignable lane; the bridge's per-lane push must zero it in the host
    // store rather than leaking a nonzero depth into the engine's additive
    // sum.
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);

    constexpr uint8_t kPage = 1;
    constexpr uint8_t kRow = 4;
    constexpr uint8_t kLane = froggers_v2::manifest::kExternalAudioRateLane;

    if (core.externalAudioAvailable())
    {
        std::printf("FAIL: test precondition, expected externalAudioAvailable() false by default\n");
        return false;
    }

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    // Inject leftover live depth via applyHostModRoute (bypasses ParamTurn's
    // assignability gate) so the bridge can still prove it zeros ineligible
    // lanes in the host store rather than leaking them into the engine sum.
    core.applyHostModRoute(kPage, kRow, kLane, 0.4f);

    const float liveDepth = core.laneDepth(kPage, kRow, kLane);
    if (nearlyEqual(liveDepth, 0.0f))
    {
        std::printf("FAIL: test precondition, expected a non-zero live depth on the ineligible lane\n");
        return false;
    }

    triggerToHostModRouteSync(bridge, 0);

    const uint8_t pmPage = HostParameterRoutingV2::pmPageForUiPage(kPage);
    const float stored = host.m_v2LaneDepths.Get(pmPage, kRow, kLane);
    if (!nearlyEqual(stored, 0.0f))
    {
        std::printf(
            "FAIL: ineligible lane depth expected 0 in host store, got %f (live depth was %f)\n",
            stored,
            liveDepth);
        return false;
    }
    return true;
}

bool test_v2_lane_depth_additive_sum_reaches_engine()
{
    // Integration: the populated V2LaneDepthStore must actually change
    // Page::GetPreFuegoValue's output through ApplyV2LaneMod's additive sum,
    // not just sit in the store unread. GetPreFuegoValue itself is private,
    // so this drives the public GetParam/GetPageParam path with neutral
    // crunchy (globalCrunchy defaults to 0, and the page's crispy row is
    // left untouched at its own zero default) so the V2 fuego stack's
    // Fuegoize calls degenerate to identity and GetPageParam reduces exactly
    // to clamp(knob + tap*depth, 0, 1).
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);

    constexpr uint8_t kPage = 1;
    constexpr uint8_t kRow = 5;
    constexpr uint8_t kLane = 9; // lfo_2

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    bridge.syncToHost();
    const uint8_t pmPage = HostParameterRoutingV2::pmPageForUiPage(kPage);
    const float knobBaseline = host.GetPageParam(pmPage, kRow);

    assignLiveModRoute(core, kPage, kRow, kLane, 10.0f); // depth ~ +0.4
    const float depth = core.laneDepth(kPage, kRow, kLane);
    if (nearlyEqual(depth, 0.0f))
    {
        std::printf("FAIL: test precondition, expected a non-zero live lane depth\n");
        return false;
    }

    triggerToHostModRouteSync(bridge, 0);

    constexpr float kTapValue = 0.5f;
    host.m_v2ModTaps.SetTap(kLane, kTapValue);

    const float expected = std::min(std::max(knobBaseline + kTapValue * depth, 0.0f), 1.0f);
    const float actual = host.GetPageParam(pmPage, kRow);
    if (!nearlyEqual(actual, expected, 2.0e-3f))
    {
        std::printf(
            "FAIL: additive sum mismatch expected=%f got=%f (knob=%f depth=%f tap=%f)\n",
            expected,
            actual,
            knobBaseline,
            depth,
            kTapValue);
        return false;
    }
    return true;
}

// Packet 15.2a: the load-bearing capability this packet unlocks. Pre-15.2a
// ParamState carried a parallel modSource[15] identity array alongside
// modDepth[15], and every mutation path (setSingleModSource) cleared every
// lane before writing at most one identity into slot 0 -- so a row could
// never carry more than one non-zero lane depth at once. Packet 15.2a
// deletes that identity array; lane i's source IS i, "lane on" = modDepth[i]
// != 0, and every lane is independently turn-editable, so two (or all 15)
// lanes can be simultaneously non-zero on the same row. This test proves
// that directly -- impossible to express before this packet.
bool test_multiple_lanes_active_simultaneously_on_one_row()
{
    FroggersV2ControlCore core;

    constexpr uint8_t kPage = 1; // Filter: no VCO pair-bus eligibility special-case (that's page 0 only)
    constexpr uint8_t kRow = 6;
    constexpr uint8_t kLaneA = 3; // vco_1_ef
    constexpr uint8_t kLaneB = 7; // vco_23_ef

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kRow));

    // Turn both lane cells while the same detail-grid session stays open --
    // no close/reopen, no re-selection between them.
    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLaneA, 10.0f));
    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLaneB, -6.0f));

    const float depthA = core.laneDepth(kPage, kRow, kLaneA);
    const float depthB = core.laneDepth(kPage, kRow, kLaneB);
    if (nearlyEqual(depthA, 0.0f) || nearlyEqual(depthB, 0.0f))
    {
        std::printf(
            "FAIL: expected both lanes non-zero simultaneously on one row (A=%f B=%f)\n",
            depthA,
            depthB);
        return false;
    }
    return true;
}

bool test_press_clears_one_lane_leaves_sibling_lane_active()
{
    FroggersV2ControlCore core;

    constexpr uint8_t kPage = 1;
    constexpr uint8_t kRow = 6;
    constexpr uint8_t kLaneA = 3;
    constexpr uint8_t kLaneB = 7;

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kRow));

    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLaneA, 10.0f));
    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLaneB, -6.0f));

    if (nearlyEqual(core.laneDepth(kPage, kRow, kLaneA), 0.0f)
        || nearlyEqual(core.laneDepth(kPage, kRow, kLaneB), 0.0f))
    {
        std::printf("FAIL: test precondition, expected both lanes non-zero before the clearing press\n");
        return false;
    }

    // Press = clear a lane (turn = set its depth); pressing lane A's cell
    // must not touch lane B's independently-live depth.
    MessageIn clearPress;
    clearPress.type = MessageIn::Type::ParamPress;
    clearPress.page = kPage;
    clearPress.slot = kLaneA;
    pushAndProcess(core, clearPress);

    if (!nearlyEqual(core.laneDepth(kPage, kRow, kLaneA), 0.0f))
    {
        std::printf("FAIL: press did not clear lane A's depth\n");
        return false;
    }
    if (nearlyEqual(core.laneDepth(kPage, kRow, kLaneB), 0.0f))
    {
        std::printf("FAIL: clearing lane A also cleared sibling lane B (should be unaffected)\n");
        return false;
    }
    return true;
}

bool test_v2_lane_depth_store_and_engine_sum_two_simultaneous_lanes()
{
    // End-to-end: two lanes live on the same row, pushed through the ToHost
    // sync (15-B path) into the owning V2LaneDepthStore, then read back
    // through Page::GetPreFuegoValue's additive sum via the public
    // GetPageParam path (see test_v2_lane_depth_additive_sum_reaches_engine
    // for why neutral crunchy makes that path reduce to plain arithmetic).
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);

    constexpr uint8_t kPage = 1;
    constexpr uint8_t kRow = 6;
    constexpr uint8_t kLaneA = 3; // vco_1_ef
    constexpr uint8_t kLaneB = 7; // vco_23_ef

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    bridge.syncToHost();
    const uint8_t pmPage = HostParameterRoutingV2::pmPageForUiPage(kPage);
    const float knobBaseline = host.GetPageParam(pmPage, kRow);

    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kRow));

    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLaneA, 10.0f));  // depth ~ +0.4
    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLaneB, -6.0f)); // depth ~ -0.24

    const float depthA = core.laneDepth(kPage, kRow, kLaneA);
    const float depthB = core.laneDepth(kPage, kRow, kLaneB);
    if (nearlyEqual(depthA, 0.0f) || nearlyEqual(depthB, 0.0f))
    {
        std::printf("FAIL: test precondition, expected two non-zero simultaneous lane depths\n");
        return false;
    }

    triggerToHostModRouteSync(bridge, 0);

    const float storedA = host.m_v2LaneDepths.Get(pmPage, kRow, kLaneA);
    const float storedB = host.m_v2LaneDepths.Get(pmPage, kRow, kLaneB);
    if (!nearlyEqual(storedA, depthA, 2.0e-3f))
    {
        std::printf("FAIL: lane A store expected %f got %f\n", depthA, storedA);
        return false;
    }
    if (!nearlyEqual(storedB, depthB, 2.0e-3f))
    {
        std::printf("FAIL: lane B store expected %f got %f\n", depthB, storedB);
        return false;
    }

    // PermanentModTapRack::SetTap clamps to [0, 1] -- taps are the unipolar
    // modulator signal value; the bipolar part of the formula is the depth
    // (already set above via the two ParamTurn calls), not the tap.
    constexpr float kTapA = 0.5f;
    constexpr float kTapB = 0.7f;
    host.m_v2ModTaps.SetTap(kLaneA, kTapA);
    host.m_v2ModTaps.SetTap(kLaneB, kTapB);

    const float expected =
        std::min(std::max(knobBaseline + kTapA * depthA + kTapB * depthB, 0.0f), 1.0f);
    const float actual = host.GetPageParam(pmPage, kRow);
    if (!nearlyEqual(actual, expected, 2.0e-3f))
    {
        std::printf(
            "FAIL: two-lane additive sum mismatch expected=%f got=%f (knob=%f dA=%f dB=%f tapA=%f tapB=%f)\n",
            expected,
            actual,
            knobBaseline,
            depthA,
            depthB,
            kTapA,
            kTapB);
        return false;
    }
    return true;
}

bool test_rand_mods_gates_ineligible_lanes()
{
    // Packet 15-D: randomizeLiveModDepths must gate ineligible lanes to 0.0f.
    // Page 0, row 0: only VCO pair-bus lane 1 is eligible; lanes 0 and 2 are
    // ineligible. Verify that lanes 0 and 2 are zeroed while lane 1 (and
    // other eligible lanes) get randomized to non-zero.
    FroggersV2ControlCore core;

    // Verify precondition: m_externalAudioAvailable is false (default).
    // This makes external audio lanes ineligible.
    constexpr uint8_t kPage = 0;
    constexpr uint8_t kRow = 0;

    core.executeRandomization(FroggersV2ControlCore::RandomizationCommand::RandMods, 0);

    // After randomization:
    // Lane 0 (VCO pair-bus, ineligible for page 0 row 0) must be 0.0f
    const float depth0 = core.laneDepth(kPage, kRow, 0);
    if (!nearlyEqual(depth0, 0.0f))
    {
        std::printf(
            "FAIL: Rand Mods should gate ineligible lane 0 on page 0 row 0 to 0.0f, got %f\n",
            depth0);
        return false;
    }

    // Lane 1 (VCO pair-bus, eligible for page 0 row 0) should be non-zero
    const float depth1 = core.laneDepth(kPage, kRow, 1);
    if (nearlyEqual(depth1, 0.0f))
    {
        std::printf(
            "FAIL: Rand Mods should randomize eligible lane 1 on page 0 row 0 to non-zero, got 0.0f\n");
        return false;
    }

    // Lane 2 (VCO pair-bus, ineligible for page 0 row 0) must be 0.0f
    const float depth2 = core.laneDepth(kPage, kRow, 2);
    if (!nearlyEqual(depth2, 0.0f))
    {
        std::printf(
            "FAIL: Rand Mods should gate ineligible lane 2 on page 0 row 0 to 0.0f, got %f\n",
            depth2);
        return false;
    }

    // Lane 3+ (not VCO pair-bus or external audio) should be non-zero
    const float depth3 = core.laneDepth(kPage, kRow, 3);
    if (nearlyEqual(depth3, 0.0f))
    {
        std::printf(
            "FAIL: Rand Mods should randomize eligible lane 3 on page 0 row 0 to non-zero, got 0.0f\n");
        return false;
    }

    return true;
}

bool test_compute_effective_sums_eligible_skips_ineligible()
{
    // Packet 15.4 / 15.9: multi-lane effective sum is assignability-gated.
    // Page 0 row 0: pair-bus lane 1 assignable; lanes 0 and 2 not.
    FroggersV2ControlCore core;
    constexpr uint8_t kPage = 0;
    constexpr uint8_t kRow = 0;
    constexpr uint8_t kEligibleLane = 1;
    constexpr uint8_t kIneligibleLane = 0;
    constexpr uint8_t kSecondEligible = 8; // LFO 1

    core.setLaneDepth(kPage, kRow, kEligibleLane, 0.5f);
    core.setLaneDepth(kPage, kRow, kIneligibleLane, 0.5f);
    core.setLaneDepth(kPage, kRow, kSecondEligible, 0.25f);

    // Drive modulator taps away from center so depth contributes.
    MessageIn clockEligible;
    clockEligible.type = MessageIn::Type::Clock;
    clockEligible.index = static_cast<uint8_t>(6 + kEligibleLane);
    clockEligible.value = 1.0f;
    pushAndProcess(core, clockEligible);

    MessageIn clockIneligible;
    clockIneligible.type = MessageIn::Type::Clock;
    clockIneligible.index = static_cast<uint8_t>(6 + kIneligibleLane);
    clockIneligible.value = 1.0f;
    pushAndProcess(core, clockIneligible);

    MessageIn clockSecond;
    clockSecond.type = MessageIn::Type::Clock;
    clockSecond.index = static_cast<uint8_t>(6 + kSecondEligible);
    clockSecond.value = 1.0f;
    pushAndProcess(core, clockSecond);

    const auto row = core.effectiveRow(kPage, kRow);
    const uint16_t expectedMask = static_cast<uint16_t>((1u << kEligibleLane) | (1u << kSecondEligible));
    if (row.modulatorsMask != expectedMask)
    {
        std::printf(
            "FAIL: computeEffective modulatorsMask expected 0x%x got 0x%x\n",
            static_cast<unsigned>(expectedMask),
            static_cast<unsigned>(row.modulatorsMask));
        return false;
    }
    if ((row.modulatorsMask & static_cast<uint16_t>(1u << kIneligibleLane)) != 0)
    {
        std::printf("FAIL: ineligible leftover lane contributed to modulatorsMask\n");
        return false;
    }
    return true;
}

bool test_available_external_audio_contributes_to_effective_and_host()
{
    // Packet 15.4 regression: assignability (not Rand eligibility) gates
    // effective sum and host lane push. External-audio lanes are non-
    // randomizable but must contribute when setExternalAudioAvailable(true).
    FroggersV2ControlCore core;
    DesktopHostIO host;
    DelayState delay;
    host.setDelayState(&delay);
    host.m_hostKind = SimHostKind::DesktopV2;
    host.Init();
    host.SetSampleRate(44100.0f);
    FroggersV2HostBridge bridge(core, host);

    constexpr uint8_t kPage = 1;
    constexpr uint8_t kRow = 4;
    constexpr uint8_t kLane = froggers_v2::manifest::kExternalAudioRateLane;
    constexpr float kDepth = 0.4f;

    core.setExternalAudioAvailable(true);
    if (!core.externalAudioAvailable())
    {
        std::printf("FAIL: precondition, setExternalAudioAvailable(true) did not stick\n");
        return false;
    }
    if (!froggers_v2::manifest::isModLaneAssignable(kPage, kRow, kLane, true))
    {
        std::printf("FAIL: precondition, external-audio lane should be assignable when available\n");
        return false;
    }
    if (froggers_v2::manifest::isModSourceEligibleForRow(kPage, kRow, kLane, true))
    {
        std::printf(
            "FAIL: precondition, external-audio lane must remain Rand-ineligible "
            "(isModSourceEligibleForRow false) even when assignable\n");
        return false;
    }

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);

    core.setLaneDepth(kPage, kRow, kLane, kDepth);
    if (!nearlyEqual(core.laneDepth(kPage, kRow, kLane), kDepth))
    {
        std::printf("FAIL: precondition, expected live external-audio depth %f\n", kDepth);
        return false;
    }

    const auto row = core.effectiveRow(kPage, kRow);
    const uint16_t laneBit = static_cast<uint16_t>(1u << kLane);
    if ((row.modulatorsMask & laneBit) == 0)
    {
        std::printf(
            "FAIL: available external-audio depth missing from modulatorsMask (got 0x%x)\n",
            static_cast<unsigned>(row.modulatorsMask));
        return false;
    }

    triggerToHostModRouteSync(bridge, 0);
    const uint8_t pmPage = HostParameterRoutingV2::pmPageForUiPage(kPage);
    const float stored = host.m_v2LaneDepths.Get(pmPage, kRow, kLane);
    if (!nearlyEqual(stored, kDepth, 2.0e-3f))
    {
        std::printf(
            "FAIL: available external-audio depth expected %f in host store, got %f\n",
            kDepth,
            stored);
        return false;
    }

    // Unavailable: leftover depth must not contribute / must be zeroed on push.
    core.setExternalAudioAvailable(false);
    const auto rowOff = core.effectiveRow(kPage, kRow);
    if ((rowOff.modulatorsMask & laneBit) != 0)
    {
        std::printf("FAIL: unavailable external-audio depth still in modulatorsMask\n");
        return false;
    }
    triggerToHostModRouteSync(bridge, 0);
    const float storedOff = host.m_v2LaneDepths.Get(pmPage, kRow, kLane);
    if (!nearlyEqual(storedOff, 0.0f))
    {
        std::printf(
            "FAIL: unavailable external-audio depth expected 0 in host store, got %f\n",
            storedOff);
        return false;
    }
    return true;
}

bool test_unavailable_lane_refuses_param_turn_and_press()
{
    // Packet 15.3 / 15.9: external-audio lanes refuse edits when unavailable.
    FroggersV2ControlCore core;
    constexpr uint8_t kPage = 1;
    constexpr uint8_t kRow = 0;
    constexpr uint8_t kLane = froggers_v2::manifest::kExternalAudioRateLane;

    if (core.externalAudioAvailable())
    {
        std::printf("FAIL: precondition, externalAudioAvailable should be false\n");
        return false;
    }

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);
    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kRow));

    pushAndProcess(core, MessageIn::ParamTurn(kPage, kLane, 10.0f));
    if (!nearlyEqual(core.laneDepth(kPage, kRow, kLane), 0.0f))
    {
        std::printf("FAIL: ParamTurn edited unavailable external-audio lane\n");
        return false;
    }

    // Inject leftover depth, then press — clear must also be refused.
    core.applyHostModRoute(kPage, kRow, kLane, 0.4f);
    MessageIn press;
    press.type = MessageIn::Type::ParamPress;
    press.page = kPage;
    press.slot = kLane;
    pushAndProcess(core, press);
    if (!nearlyEqual(core.laneDepth(kPage, kRow, kLane), 0.4f))
    {
        std::printf("FAIL: ParamPress cleared unavailable external-audio lane\n");
        return false;
    }
    return true;
}

bool test_vco_pair_bus_lane_refuses_edit()
{
    // Packet 15.9: VCO pair-bus self-feedback lanes refuse detail edits.
    FroggersV2ControlCore core;
    constexpr uint8_t kPage = 0;
    constexpr uint8_t kRow = 0;
    constexpr uint8_t kBlockedLane = 0; // ineligible for VCO1 row

    MessageIn selectPage;
    selectPage.type = MessageIn::Type::SelectPage;
    selectPage.page = kPage;
    pushAndProcess(core, selectPage);
    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kRow));
    pushAndProcess(core, MessageIn::ParamTurn(kPage, kBlockedLane, 10.0f));
    if (!nearlyEqual(core.laneDepth(kPage, kRow, kBlockedLane), 0.0f))
    {
        std::printf("FAIL: ParamTurn edited blocked VCO pair-bus lane\n");
        return false;
    }
    return true;
}

// Packet 15-C1: ModDrillIn (not ParamPress) is the sole action that opens the
// detail grid; ParamPress inside an already-open view keeps its Packet 15.2a
// meaning (clear a lane / exit via the Target-Back cell).
bool test_mod_drill_in_opens_detail_grid()
{
    FroggersV2ControlCore core;
    pushAndProcess(core, MessageIn::ModDrillIn(0, 0));
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != 0)
    {
        std::printf("FAIL: ModDrillIn did not open the detail grid\n");
        return false;
    }
    return true;
}

bool test_param_press_no_longer_opens_detail_grid()
{
    FroggersV2ControlCore core;
    MessageIn press;
    press.type = MessageIn::Type::ParamPress;
    press.page = 0;
    press.slot = 0;
    pushAndProcess(core, press);
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: ParamPress opened the detail grid (should be ModDrillIn-only now)\n");
        return false;
    }
    return true;
}

bool test_param_press_still_clears_lane_while_detail_grid_open()
{
    FroggersV2ControlCore core;
    pushAndProcess(core, MessageIn::ModDrillIn(0, 0));
    pushAndProcess(core, MessageIn::ParamTurn(0, 5, 10.0f));
    if (nearlyEqual(core.laneDepth(0, 0, 5), 0.0f))
    {
        std::printf("FAIL: test precondition, lane 5 should be non-zero after turning\n");
        return false;
    }
    MessageIn clearPress;
    clearPress.type = MessageIn::Type::ParamPress;
    clearPress.page = 0;
    clearPress.slot = 5;
    pushAndProcess(core, clearPress);
    if (!nearlyEqual(core.laneDepth(0, 0, 5), 0.0f))
    {
        std::printf("FAIL: ParamPress on an open lane cell did not clear it\n");
        return false;
    }
    return true;
}

bool test_param_press_still_exits_detail_grid_via_target_cell()
{
    FroggersV2ControlCore core;
    pushAndProcess(core, MessageIn::ModDrillIn(0, 0));
    MessageIn closePress;
    closePress.type = MessageIn::Type::ParamPress;
    closePress.page = 0;
    closePress.slot = static_cast<uint8_t>(core.uiState().visibleCount.load(std::memory_order_acquire) - 1);
    pushAndProcess(core, closePress);
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: ParamPress on the Target(Back) cell did not exit the detail grid\n");
        return false;
    }
    return true;
}

// Packet 6 (D4): drill-in is capped at 2 layers. The detail grid reuses the
// same EncoderRingComponent rings as layer 0, and every ring's center-dot
// hit-zone fires ModDrillIn unconditionally (EncoderRingComponent::mouseDown)
// -- so a click on a depth cell's own center dot while the grid is already
// open emits a second ModDrillIn message. That must be rejected as a no-op:
// the already-open view's targetRow must not change, and it must not close.
bool test_mod_drill_in_from_depth_cell_is_rejected()
{
    FroggersV2ControlCore core;
    constexpr uint8_t kPage = 0;
    constexpr uint8_t kFirstRow = 5;
    constexpr uint8_t kOtherRow = 2;

    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kFirstRow));
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != kFirstRow)
    {
        std::printf("FAIL: test precondition, first ModDrillIn did not open row %u\n", kFirstRow);
        return false;
    }

    // Simulate a nested drill-in fired from within the already-open detail
    // grid (a depth cell's center dot), targeting a different row.
    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kOtherRow));

    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != kFirstRow)
    {
        std::printf(
            "FAIL: nested ModDrillIn while detail grid was open changed/closed the view "
            "(expected targetRow to remain %u)\n",
            kFirstRow);
        return false;
    }
    return true;
}

// Packet 6 (D4): Target(Back) must exit an open detail grid straight to
// layer 0 -- not to some intermediate state -- covering the case where the
// grid was entered from a non-zero row.
bool test_target_back_exits_to_layer_zero_from_nonzero_row()
{
    FroggersV2ControlCore core;
    constexpr uint8_t kPage = 0;
    constexpr uint8_t kRow = 4;
    constexpr uint8_t kFirstRowAfterClose = 1;

    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kRow));
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != kRow)
    {
        std::printf("FAIL: test precondition, ModDrillIn did not open row %u\n", kRow);
        return false;
    }

    MessageIn closePress;
    closePress.type = MessageIn::Type::ParamPress;
    closePress.page = kPage;
    closePress.slot = static_cast<uint8_t>(core.uiState().visibleCount.load(std::memory_order_acquire) - 1);
    pushAndProcess(core, closePress);

    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != froggers_v2::kNoSelection)
    {
        std::printf("FAIL: Target(Back) did not exit the detail grid back to layer 0\n");
        return false;
    }

    // Layer 0 must be reachable again: a fresh ModDrillIn on a different row
    // opens normally (proves the reject gate above does not stick after close).
    pushAndProcess(core, MessageIn::ModDrillIn(kPage, kFirstRowAfterClose));
    if (core.uiState().modViewTargetRow.load(std::memory_order_acquire) != kFirstRowAfterClose)
    {
        std::printf("FAIL: a fresh ModDrillIn after Target(Back) close did not reopen the grid\n");
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
    if (!test_scene_blend_semantics())
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
    if (!test_rand_mod_syncs_host_mod_routes())
    {
        return 1;
    }
    if (!test_pair_ar_gate_policy())
    {
        return 1;
    }
    if (!test_envelope_page_ten_rows())
    {
        return 1;
    }
    if (!test_rand_mods_changes_live_depths_without_sequencer())
    {
        return 1;
    }
    if (!test_rand_all_respects_scene_scope())
    {
        return 1;
    }
    if (!test_v2_lane_depth_store_places_each_row_at_its_own_lane())
    {
        return 1;
    }
    if (!test_v2_lane_depth_store_zeros_ineligible_lane())
    {
        return 1;
    }
    if (!test_v2_lane_depth_additive_sum_reaches_engine())
    {
        return 1;
    }
    if (!test_multiple_lanes_active_simultaneously_on_one_row())
    {
        return 1;
    }
    if (!test_press_clears_one_lane_leaves_sibling_lane_active())
    {
        return 1;
    }
    if (!test_v2_lane_depth_store_and_engine_sum_two_simultaneous_lanes())
    {
        return 1;
    }
    if (!test_rand_mods_gates_ineligible_lanes())
    {
        return 1;
    }
    if (!test_compute_effective_sums_eligible_skips_ineligible())
    {
        return 1;
    }
    if (!test_available_external_audio_contributes_to_effective_and_host())
    {
        return 1;
    }
    if (!test_unavailable_lane_refuses_param_turn_and_press())
    {
        return 1;
    }
    if (!test_vco_pair_bus_lane_refuses_edit())
    {
        return 1;
    }
    if (!test_mod_drill_in_opens_detail_grid())
    {
        return 1;
    }
    if (!test_param_press_no_longer_opens_detail_grid())
    {
        return 1;
    }
    if (!test_param_press_still_clears_lane_while_detail_grid_open())
    {
        return 1;
    }
    if (!test_param_press_still_exits_detail_grid_via_target_cell())
    {
        return 1;
    }
    if (!test_mod_drill_in_from_depth_cell_is_rejected())
    {
        return 1;
    }
    if (!test_target_back_exits_to_layer_zero_from_nonzero_row())
    {
        return 1;
    }

    std::printf("PASS: ControlCoreBridge tests\n");
    return 0;
}
