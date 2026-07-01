#include "DesktopV2HostCallbacks.hpp"

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
    ctx.carousel.onPageChanged = [&ctx](uint8_t page) { pushSelectPage(ctx, page); };
    ctx.carousel.onRandomize = [&ctx](uint8_t page) {
        froggers_v2::MessageIn message;
        message.type = froggers_v2::MessageIn::Type::RandPage;
        message.page = page;
        ctx.core.bus().push(message);
        ctx.core.processBus();
        ctx.bridge.syncToHost();
        ctx.carousel.refresh();
    };
    ctx.carousel.onRandomizeMod = [&ctx](uint8_t page) { pushRandomizeMod(ctx, page); };
}
} // namespace desktop_v2
