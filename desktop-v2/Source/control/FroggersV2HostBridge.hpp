#pragma once

#include "DesktopHostIO.hpp"
#include "FroggersV2ControlCore.hpp"

#include <functional>

namespace froggers_v2
{
class FroggersV2HostBridge
{
public:
    FroggersV2HostBridge(FroggersV2ControlCore& core, DesktopHostIO& host);
    ~FroggersV2HostBridge();

    void syncToHost();
    void syncFromHostModRoutes();
    void syncModRoutesToHost();
    void setOnStepCaptured(std::function<void(uint8_t step)> callback);

private:
    enum class ModRouteDirection : uint8_t
    {
        FromHost,
        ToHost,
    };

    void syncModRoutes(uint8_t page, uint8_t row, ModRouteDirection direction);

    FroggersV2ControlCore& m_core;
    DesktopHostIO& m_host;
    std::function<void(uint8_t step)> m_onStepCaptured;
    bool m_vcoMorphDefaultsApplied = false;
};
} // namespace froggers_v2
