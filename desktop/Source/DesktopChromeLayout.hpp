#pragma once

#include <array>
#include <cstdint>

#include "HostPanelLayout.hpp"

namespace DesktopChromeLayout
{
using namespace HostPanelLayout;

// Desktop column layout: Drive→Filter→Reverb (pages 4,3,2) matches FrogBlock then ApplyOutputFx output FX order.
inline constexpr std::array<uint8_t, 5> kDesktopCoreColumnPageOrder{0, 1, 4, 3, 2};
} // namespace DesktopChromeLayout
