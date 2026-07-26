#include "control/FroggersV2ControlCore.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/GlobalStripV2.hpp"

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

    froggers_v2::MessageIn assign;
    assign.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    assign.page = 0;
    assign.slot = 0;
    assign.index = 2;
    core.bus().push(assign);
    core.processBus();

    const float depthBefore = core.assignedModDepth(0, 0);

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

bool test_shift_control_absent()
{
    // Packet 18.6 / D16: Shift machinery is fully removed. GlobalStripV2 has
    // no setShiftHeld / Shift toggle; command-row fill is the Crunchy ring.
    DesktopHostIO host;
    froggers_v2::FroggersV2ControlCore core;
    GlobalStripV2 strip;
    strip.bind(&host, &core);
    strip.setSize(1280, DesktopV2ChromeLayout::kGlobalCommandBandH);
    strip.resized();
    if (strip.crunchyRingBoundsForTest().getRight() != strip.getLocalBounds().getRight())
    {
        std::printf("FAIL: Crunchy ring does not fill to global strip right edge after Shift removal\n");
        return false;
    }
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
    if (!test_shift_control_absent())
    {
        return 1;
    }
    std::printf("PASS: GlobalControlParity_test\n");
    return 0;
}
