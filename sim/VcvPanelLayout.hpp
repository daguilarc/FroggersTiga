#pragma once

#include "HostPanelLayout.hpp"

// MIT layout authority for VCV Rack panel geometry. CI reads this header;
// vcv/ (GPL, local-only) includes it via -I../sim.

namespace VcvPanelLayout
{
constexpr float kGridWidth = 15.f;
constexpr float kGridHeight = 380.f;
constexpr float kPrimaryHp = 24.f;
constexpr float kVoicingHp = 48.f;
constexpr float kMainHp = kPrimaryHp + kVoicingHp;
constexpr float kFxHp = 36.f;
constexpr float kVcoArHp = 24.f;
constexpr float kColumnHp = 12.f;
constexpr int kVoicingColumns = 4;
constexpr int kFxColumns = 2;
constexpr float kPrimaryRightmostIoGrid = 21.5f;
constexpr float kPrimaryGateGridX = 11.5f;
constexpr float kPrimaryGlobalCrunchyGridX = 20.0f;
constexpr float kPrimaryGlobalCrunchyCvGridX = 22.0f;
constexpr float kPrimaryGlobalCrunchyGridY = 10.5f;
constexpr float kPanelHeightMm = 128.5f;
constexpr float kHeaderStripGridY = 2.25f;
constexpr int kRows = HostPanelLayout::kNumRows;
} // namespace VcvPanelLayout
