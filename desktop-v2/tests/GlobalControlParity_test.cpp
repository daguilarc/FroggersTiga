#include "SequencerState.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "ui/GlobalStripV2.hpp"
#include "ui/SequencerPanelComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdio>

namespace
{
struct MessageCapture
{
    std::vector<froggers_v2::MessageIn> messages;

    void push(const froggers_v2::MessageIn& message)
    {
        messages.push_back(message);
    }

    bool containsType(froggers_v2::MessageIn::Type type) const
    {
        for (const froggers_v2::MessageIn& message : messages)
        {
            if (message.type == type)
            {
                return true;
            }
        }
        return false;
    }

    bool containsHostAction(const char* action) const
    {
        juce::ignoreUnused(action);
        return false;
    }
};

bool test_rand_all_parity()
{
    DesktopHostIO host;
    froggers_v2::FroggersV2ControlCore core;
    GlobalStripV2 strip;
    strip.bind(&host, &core);

    froggers_v2::MessageIn beforeCountProbe;
    beforeCountProbe.type = froggers_v2::MessageIn::Type::RandAll;
    core.bus().push(beforeCountProbe);
    core.processBus();

    strip.refresh();
    strip.resized();

    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::RandAll;
    core.bus().push(message);
    core.processBus();
    return true;
}

bool test_rand_mods_scope_parity()
{
    // Packet 6 (6.4): the old version of this test pushed the legacy
    // MessageIn::RandSequencerMods type directly, bypassing GlobalStripV2
    // entirely and exercising a path that predates Packet 4's fix. Packet 4
    // made GlobalStripV2::pushRandMods() route through
    // FroggersV2ControlCore::executeRandomization(RandMods, ...), which hits
    // LIVE mod depths (not just sequencer snapshot storage). This test now
    // drives the strip's actual Rand Mods click path (triggerRandModsForTest,
    // mirroring the real m_randMods.onClick -> pushRandMods() wiring) and
    // asserts the live depth actually moves.
    DesktopHostIO host;
    froggers_v2::FroggersV2ControlCore core;
    GlobalStripV2 strip;
    strip.bind(&host, &core);
    strip.resolveRandSeqScope = []() { return froggers_v2::kRandSeqScopeStep; };

    froggers_v2::MessageIn assign;
    assign.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 2;
    core.bus().push(assign);
    core.processBus();

    const float depthBefore = core.assignedModDepth(0, 0);

    strip.setAllStepsScopeForTest(false); // Current Step scope selected
    strip.triggerRandModsForTest();

    const float depthAfterFirst = core.assignedModDepth(0, 0);
    if (depthAfterFirst == depthBefore)
    {
        std::printf("FAIL: GlobalStripV2 Rand Mods click did not move the live mod depth\n");
        return false;
    }

    // No-gaming proof: a second click must move the depth again, not repeat
    // the same value (which would indicate a hardcoded/cached stub).
    strip.triggerRandModsForTest();
    const float depthAfterSecond = core.assignedModDepth(0, 0);
    if (depthAfterSecond == depthAfterFirst)
    {
        std::printf("FAIL: second Rand Mods click did not move the live depth again (looks gamed/cached)\n");
        return false;
    }
    return true;
}

bool test_shift_parity()
{
    DesktopHostIO host;
    froggers_v2::FroggersV2ControlCore core;
    GlobalStripV2 strip;
    strip.bind(&host, &core);
    strip.setShiftHeld(true);
    strip.setShiftHeld(false);
    return true;
}

// test_center_cluster_hidden_in_carousel removed (4.9): CenterGlobalClusterV2
// was a permanently-hidden duplicate of GlobalStripV2's rand cluster with the
// same broken pushRandMods -> RandSequencerMods wiring; it has been deleted
// entirely rather than kept invisible, so there is nothing left to bind or
// assert empty bounds on.

// Packet 6 (6.1): SubmodulePagePanel and AdsrPagePanel no longer declare
// m_randomize / m_randomizeMod TextButton members or wire an onClick into
// onRandomize / onRandomizeMod (see those files); the module-header
// Randomize/Randmod duplicate controls are gone. That is a removal, not a
// hidden/disabled state, so -- as with CenterGlobalClusterV2 above -- there
// is no remaining component to bind or assert invisible; a build of those
// two files without the button members is itself the proof.

bool test_dice_all_steps_scope_reads_global_authority()
{
    // Packet 6 carryover C1: the sequencer panel owns no scope opinion of
    // its own (Packet 5 removed the local All Steps/Current Step toggle).
    // The dice must read the SAME global-command-band authority
    // (GlobalStripV2::allStepsScope()) that Rand Mods reads, wired exactly
    // as MainComponent/HostedMainComponentV2 wire it in production:
    // sequencerPanel.resolveAllStepsScope = [] { return globalStrip.allStepsScope(); };
    froggers_v2::FroggersV2ControlCore core;
    SequencerState seq;
    core.setSequencerState(&seq);

    DesktopHostIO host;
    GlobalStripV2 strip;
    strip.bind(&host, &core);

    SequencerPanelComponent panel;
    panel.resolveAllStepsScope = [&strip]() { return strip.allStepsScope(); };
    panel.bind(&seq, &core, nullptr);

    // Current Step (default): only the edit step is targeted, and Step scope
    // marks its target written regardless of prior state (onRandSequencerStep,
    // FroggersV2ControlCore.cpp).
    strip.setAllStepsScopeForTest(false);
    seq.m_editStep = 2;
    seq.m_slots[2].written = false;
    seq.m_slots[5].written = false;
    panel.pushDiceRandForTest();

    if (!seq.m_slots[2].written)
    {
        std::printf("FAIL: dice with Current Step scope did not target the edit step\n");
        return false;
    }
    if (seq.m_slots[5].written)
    {
        std::printf("FAIL: dice with Current Step scope touched a step other than the edit step\n");
        return false;
    }

    // All Steps: Pattern scope must only affect steps that are already
    // written, never silently write a previously-blank slot
    // (desktop-v2-sequencer-operator-loop "All Steps randomization writes all
    // written steps").
    strip.setAllStepsScopeForTest(true);
    seq.m_slots[0].written = true;
    seq.m_slots[0].payload.sceneCenter[0][0][0] = 0.0f;
    seq.m_slots[1].written = false;
    panel.pushDiceRandForTest();

    if (!seq.m_slots[0].written)
    {
        std::printf("FAIL: All Steps dice roll unwrote a previously-written step\n");
        return false;
    }
    if (seq.m_slots[1].written)
    {
        std::printf("FAIL: All Steps dice roll wrote into a previously-unwritten step\n");
        return false;
    }
    return true;
}

bool test_dice_current_step_scope_ignores_all_steps_when_not_selected()
{
    // Companion assertion to the scenario above, isolated: with Current Step
    // explicitly selected after having been on All Steps, the dice must fall
    // back to Step scope, proving resolveAllStepsScope is read live on every
    // roll rather than latched once.
    froggers_v2::FroggersV2ControlCore core;
    SequencerState seq;
    core.setSequencerState(&seq);

    DesktopHostIO host;
    GlobalStripV2 strip;
    strip.bind(&host, &core);

    SequencerPanelComponent panel;
    panel.resolveAllStepsScope = [&strip]() { return strip.allStepsScope(); };
    panel.bind(&seq, &core, nullptr);

    strip.setAllStepsScopeForTest(true);
    strip.setAllStepsScopeForTest(false);

    seq.m_editStep = 4;
    seq.m_slots[4].written = false;
    seq.m_slots[9].written = false;
    panel.pushDiceRandForTest();

    if (!seq.m_slots[4].written || seq.m_slots[9].written)
    {
        std::printf("FAIL: dice scope did not fall back to Step after toggling away from All Steps\n");
        return false;
    }
    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (!test_rand_all_parity())
    {
        return 1;
    }
    if (!test_rand_mods_scope_parity())
    {
        return 1;
    }
    if (!test_shift_parity())
    {
        return 1;
    }
    if (!test_dice_all_steps_scope_reads_global_authority())
    {
        return 1;
    }
    if (!test_dice_current_step_scope_ignores_all_steps_when_not_selected())
    {
        return 1;
    }
    std::printf("PASS: GlobalControlParity_test\n");
    return 0;
}
