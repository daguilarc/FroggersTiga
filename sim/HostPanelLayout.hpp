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

constexpr int kPanelColumnMinWidth = 168;
constexpr int kPanelBodyMinHeight = 488;
constexpr int kHostedEditorMinWidth = 16 + 6 * kPanelColumnMinWidth;
constexpr int kHostedEditorMinHeight =
    16 + kTransportRowH + kModRackRowH + 6 + 40 + 6 + kPanelBodyMinHeight;

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

constexpr uint8_t kScopeSampleCapacity = 96;

enum class ModCellPresentation : uint8_t
{
    Scope = 0,
    Led = 1,
};

struct ModRackCellSpec
{
    uint8_t modIndex;
    ModCellPresentation presentation;
    bool includeDesktop;
    bool includeWeb;
    bool includeVst;
    bool includeVcv;
};

inline constexpr ModRackCellSpec kModRackCatalog[] = {
    {0, ModCellPresentation::Scope, true, true, false, false},
    {1, ModCellPresentation::Scope, true, false, false, false},
    {4, ModCellPresentation::Scope, true, true, true, true},
    {5, ModCellPresentation::Led, true, true, true, true},
    {6, ModCellPresentation::Led, true, true, true, true},
};
} // namespace HostPanelLayout
