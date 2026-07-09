#include "control/FroggersV2ControlCore.hpp"
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
    DesktopHostIO host;
    froggers_v2::FroggersV2ControlCore core;
    GlobalStripV2 strip;
    strip.bind(&host, &core);
    strip.resolveRandSeqScope = []() { return froggers_v2::kRandSeqScopeStep; };

    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::RandSequencerMods;
    message.page = froggers_v2::kRandSeqScopeStep;
    core.bus().push(message);
    core.processBus();
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
    std::printf("PASS: GlobalControlParity_test\n");
    return 0;
}
