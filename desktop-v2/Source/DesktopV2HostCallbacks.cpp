#include "DesktopV2HostCallbacks.hpp"

#include <cassert>

namespace desktop_v2
{
void pushSelectPage(const HostCallbackContext& ctx, uint8_t page)
{
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::SelectPage;
    message.page = page;
    ctx.core.bus().push(message);
    ctx.core.processBus();
    ctx.bridge.syncToHost();
    ctx.carousel.refresh();
}

void pushRandomizeMod(const HostCallbackContext& ctx, uint8_t page)
{
    ctx.host.EnqueueRandomizePanelMod(page);
    ctx.host.DrainPendingMutations();
    ctx.lastModRoutesVersion = ctx.host.modRoutesVersion();
    ctx.bridge.syncFromHostModRoutes();
    ctx.carousel.refresh();
}

void wireCallbacks(const HostCallbackContext& ctx)
{
    ctx.carousel.onPageChanged = [ctxPtr = &ctx](uint8_t page) { pushSelectPage(*ctxPtr, page); };
    ctx.carousel.onRandomize = [ctxPtr = &ctx](uint8_t page) {
        froggers_v2::MessageIn message;
        message.type = froggers_v2::MessageIn::Type::RandPage;
        message.page = page;
        ctxPtr->core.bus().push(message);
        ctxPtr->core.processBus();
        ctxPtr->bridge.syncToHost();
        ctxPtr->carousel.refresh();
    };
    ctx.carousel.onRandomizeMod = [ctxPtr = &ctx](uint8_t page) { pushRandomizeMod(*ctxPtr, page); };
}

void refreshAndWireHostCallbacks(HostCallbackContext& ctx,
                                 froggers_v2::FroggersV2ControlCore& core,
                                 froggers_v2::FroggersV2HostBridge& bridge,
                                 DesktopHostIO& host,
                                 PageCarouselComponent& carousel,
                                 uint32_t& lastModRoutesVersion)
{
    assert(&ctx.core == &core);
    assert(&ctx.bridge == &bridge);
    assert(&ctx.host == &host);
    assert(&ctx.carousel == &carousel);
    assert(&ctx.lastModRoutesVersion == &lastModRoutesVersion);
    wireCallbacks(ctx);
}
} // namespace desktop_v2
