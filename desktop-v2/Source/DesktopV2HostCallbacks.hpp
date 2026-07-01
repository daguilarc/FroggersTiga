#pragma once

#include "control/FroggersV2ControlCore.hpp"
#include "control/FroggersV2HostBridge.hpp"
#include "DesktopHostIO.hpp"
#include "ui/PageCarouselComponent.hpp"

#include <cstdint>

namespace desktop_v2
{
struct HostCallbackContext
{
    froggers_v2::FroggersV2ControlCore& core;
    froggers_v2::FroggersV2HostBridge& bridge;
    DesktopHostIO& host;
    PageCarouselComponent& carousel;
    uint32_t& lastModRoutesVersion;

    HostCallbackContext(froggers_v2::FroggersV2ControlCore& coreIn,
                        froggers_v2::FroggersV2HostBridge& bridgeIn,
                        DesktopHostIO& hostIn,
                        PageCarouselComponent& carouselIn,
                        uint32_t& lastModRoutesVersionIn)
        : core(coreIn)
        , bridge(bridgeIn)
        , host(hostIn)
        , carousel(carouselIn)
        , lastModRoutesVersion(lastModRoutesVersionIn)
    {
    }
};

void wireCallbacks(const HostCallbackContext& ctx);
void refreshAndWireHostCallbacks(HostCallbackContext& ctx,
                                 froggers_v2::FroggersV2ControlCore& core,
                                 froggers_v2::FroggersV2HostBridge& bridge,
                                 DesktopHostIO& host,
                                 PageCarouselComponent& carousel,
                                 uint32_t& lastModRoutesVersion);
void pushSelectPage(const HostCallbackContext& ctx, uint8_t page);
void pushRandomizeMod(const HostCallbackContext& ctx, uint8_t page);
} // namespace desktop_v2
