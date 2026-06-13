## 1. Row cell layout

- [x] 1.1 Add `m_rowCellBounds` (`std::array<juce::Rectangle<int>, 8>`) to `SubModulePanel.h`
- [x] 1.2 Add `layoutRowCell(cell, row, hasWave, ...)` helper — label + wave/knob/jack cluster inside `cell`; set `m_inputJackBounds[row]`
- [x] 1.3 Refactor `layoutPanel()` — loop rows 0–6 through `layoutRowCell`; row 7 (FUEG) through same helper with `hasWave = false`

## 2. Visual borders

- [x] 2.1 In `paint()`, loop rows 0–7 and draw 1 px border on each `m_rowCellBounds[row]` before mod rings and jack fills
- [x] 2.2 Use chrome contrast colour (`0xff3d444d` or existing panel border token)

## 3. Verify layout and cables

- [x] 3.1 Build desktop target; launch at 1440×720 — eight bordered cells visible per panel, labels grouped with knobs
- [x] 3.2 Audio panel: VCO1–VCO3 labels + wave + knob + jack fit without clipping
- [ ] 3.3 Drag patch cable from mod rack to row jacks on Audio and Delay — drop targets align with visible jacks
- [x] 3.4 Resize window to 1680×720 — layout remains valid, no overlapping cells

## 4. Archive

- [ ] 4.1 Archive change; merge specs into `openspec/specs/desktop-panel-row-cells/` and `desktop-panel-knobs/`
