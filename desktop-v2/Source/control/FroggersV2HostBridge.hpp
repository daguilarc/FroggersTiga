#pragma once

#include "DesktopHostIO.hpp"
#include "FroggersV2ControlCore.hpp"

namespace froggers_v2
{
class FroggersV2HostBridge
{
public:
    FroggersV2HostBridge(FroggersV2ControlCore& core, DesktopHostIO& host);
    ~FroggersV2HostBridge();

    void syncToHost();
    void syncFromHostModRoutes();
    void onSequencerStepAdvance();

private:
    enum class ModRouteDirection : uint8_t
    {
        FromHost,
        ToHost,
    };

    void syncModRoutes(uint8_t page, uint8_t row, ModRouteDirection direction);

    FroggersV2ControlCore& m_core;
    DesktopHostIO& m_host;
    bool m_vcoMorphDefaultsApplied = false;
};
} // namespace froggers_v2
