## Why

Desktop `SubModulePanel` lays out each parameter as a full-width horizontal strip: label on the far left, knob and patch jack on the far right, with empty space between. Labels feel disconnected from their controls; patch targets are hard to associate visually. The web sim already groups each parameter in a bordered column cell — desktop should use the same spatial logic adapted to its row-first panel (eight stacked rows per module, patch cables on jacks).

## What Changes

- Refactor `SubModulePanel` row layout so each of the eight parameter rows lives in a **bordered grid cell** (one row per cell, single column stack).
- Inside each cell: **label**, **knob**, optional **wave button** (VCO rows), and **input jack** grouped together — no full-width label stretch.
- Draw **thin 1 px borders** around each cell using existing panel chrome colours; no heavy boxes or nested subgroup panels.
- Preserve patch-cable port registration (`collectInputPorts`, `m_inputJackBounds`) and existing knob/jack sizes (38 px knob, 20 px jack).
- Panel header (title + Randomize / Randmod) unchanged; six-panel horizontal split unchanged.

## Capabilities

### New Capabilities

- `desktop-panel-row-cells`: Bordered per-row cells inside each submodule panel; label + knob + jack grouped spatially.

### Modified Capabilities

- `desktop-panel-knobs`: Row layout requirement — knobs and jacks SHALL sit inside the row cell bounds, not at the panel far edge with detached labels.

## Impact

- `desktop/Source/SubModulePanel.cpp` — `layoutPanel()`, `paint()`
- `desktop/Source/SubModulePanel.h` — row cell bounds storage (if not paint-only)
- `PatchCableOverlay` — no API change; jack screen bounds must remain accurate after layout
- Default window 1440×720 (`DesktopChromeLayout`) — verify Audio VCO rows still fit at ~240 px panel width
