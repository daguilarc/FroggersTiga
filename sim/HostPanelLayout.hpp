#pragma once

#include "ParamDisplayNames.hpp"

namespace HostPanelLayout
{
constexpr uint8_t kNumHostPages = ParamDisplayNames::kNumHostPages;
constexpr uint8_t kNumRows = ParamDisplayNames::kNumRows;
constexpr uint8_t kCrispyRow = ParamDisplayNames::kCrispyRow;
constexpr uint8_t kDelayHostPage = ParamDisplayNames::kDelayHostPage;

constexpr int kModBoxWidth = 96;
constexpr int kModBoxGap = 16;
constexpr int kModBoxMinWidth = 80;
constexpr int kRecordClusterW = 120;
constexpr int kFormatRowH = 20;
constexpr int kTransportRowH = 32;
constexpr int kModRackRowH = 72;
constexpr int kRecordRowH = 28;
constexpr int kDefaultWidth = 1440;
constexpr int kDefaultHeight = 720;

constexpr int kModRackGroupWidth = 5 * kModBoxWidth + 4 * kModBoxGap;

enum class ModIndicatorMode : uint8_t
{
    LedOnly = 0,
    ScopeAndLed = 1,
};

inline ModIndicatorMode modIndicatorModeForVcv()
{
    return ModIndicatorMode::LedOnly;
}

inline ModIndicatorMode modIndicatorModeForDesktopOrVst()
{
    return ModIndicatorMode::ScopeAndLed;
}
} // namespace HostPanelLayout
