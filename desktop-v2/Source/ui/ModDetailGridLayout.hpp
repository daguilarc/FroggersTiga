#pragma once

#include "control/FroggersV2ControlCore.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdint>

// Shared geometry for the parameter-detail grid. The grid is a fixed 4x4
// (16-cell) layout: manifest lanes 0..kNumModSources-1 followed by the single
// dedicated target/Crispy cell. This is the sole authority for detail-grid cell
// placement so the module and Pair-AR panels do not each re-derive it.
namespace froggers_v2::ui
{
constexpr int kModDetailGridCols = 4;
constexpr int kModDetailGridRows = 4;
constexpr int kModDetailCellPad = 3;
constexpr int kModDetailLabelStripH = 14;

static_assert(kModDetailGridCols * kModDetailGridRows == froggers_v2::kUiSlots,
              "detail grid must render exactly kUiSlots (16) cells");

inline juce::Rectangle<int> modDetailCellBounds(const juce::Rectangle<int>& area, int index)
{
    const int col = index % kModDetailGridCols;
    const int row = index / kModDetailGridCols;
    const int cellW = area.getWidth() / kModDetailGridCols;
    const int cellH = area.getHeight() / kModDetailGridRows;
    return juce::Rectangle<int>(
        area.getX() + col * cellW, area.getY() + row * cellH, cellW, cellH);
}
} // namespace froggers_v2::ui
