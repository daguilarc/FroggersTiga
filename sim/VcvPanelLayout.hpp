#pragma once

#include "HostPanelLayout.hpp"

// MIT layout authority for VCV Rack panel geometry. CI reads this header;
// vcv/ (GPL, local-only) includes it via -I../sim.

namespace VcvPanelLayout
{
constexpr float kGridWidth = 15.f;
constexpr float kGridHeight = 380.f;
constexpr float kPrimaryHp = 24.f;
constexpr float kExpanderHp = 36.f;
constexpr float kColumnHp = 12.f;
constexpr int kExpanderColumns = 3;
constexpr float kPrimaryRightmostIoGrid = 21.5f;
constexpr int kRows = HostPanelLayout::kNumRows;
} // namespace VcvPanelLayout
