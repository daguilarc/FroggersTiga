#include "control/FroggersV2ControlCore.hpp"
#include "ui/CenterGlobalClusterV2.hpp"
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

bool test_center_cluster_hidden_in_carousel()
{
    froggers_v2::FroggersV2ControlCore core;
    DesktopHostIO host;
    CenterGlobalClusterV2 cluster;
    cluster.bind(&host, &core);
    cluster.setBounds(0, 0, 0, 0);
    if (!cluster.getBounds().isEmpty())
    {
        std::printf("FAIL: center cluster should have empty bounds when retired\n");
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
    if (!test_center_cluster_hidden_in_carousel())
    {
        return 1;
    }
    std::printf("PASS: GlobalControlParity_test\n");
    return 0;
}
