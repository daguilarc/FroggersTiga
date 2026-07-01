#include "DesktopV2HostCallbacks.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "ui/PageCarouselComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdio>

using froggers_v2::FroggersV2ControlCore;
using froggers_v2::FroggersV2HostBridge;

namespace
{
bool test_callback_survives_after_wire_returns()
{
    FroggersV2ControlCore core;
    DesktopHostIO host;
    FroggersV2HostBridge bridge(core, host);
    PageCarouselComponent carousel;
    carousel.bindCore(&core);
    uint32_t lastModRoutesVersion = 0;

    desktop_v2::HostCallbackContext ctx(core, bridge, host, carousel, lastModRoutesVersion);
    desktop_v2::refreshAndWireHostCallbacks(
        ctx, core, bridge, host, carousel, lastModRoutesVersion);

    if (!carousel.onPageChanged)
    {
        std::printf("FAIL: onPageChanged not wired\n");
        return false;
    }

    carousel.onPageChanged(1);

    if (core.activePage() != 1)
    {
        std::printf("FAIL: activePage expected 1 got %u\n", static_cast<unsigned>(core.activePage()));
        return false;
    }

    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (!test_callback_survives_after_wire_returns())
    {
        return 1;
    }
    std::printf("PASS: CallbackLifetime_test\n");
    return 0;
}
