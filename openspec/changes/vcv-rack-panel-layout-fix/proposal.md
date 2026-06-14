## Why

The shipped VCV Rack plugin packs six submodule columns (48 knobs + 48 mod jacks + 48 row labels) onto a single 72 HP expander while the primary module is also 72 HP but only uses ~22 HP of widgets. Column widgets overlap (10 HP labels + knob + jack span ~12 HP but are placed on 12 HP centers), so most controls are illegible or off-panel. Selecting the module in Rack’s browser preview triggers instability (Framebuffer 0×0 warnings, fatal signals during widget teardown). A partial layout refactor in `FieldParityWidget.hpp` left the tree non-compiling (`panelSize`, `addRowLabel`, and `columnCenterX` signature drift). The `vcv-vst-field-parity-panel` design already documented the **3+3 expander fallback** when six columns do not fit at readable density — that fallback was never applied.

## What Changes

- **BREAKING:** Replace single `FroggersTigaExpander` (72 HP × 6 columns) with two expanders: **Expander A** (36 HP, pages 0–2) and **Expander B** (36 HP, pages 3–5), chained left of primary via Rack expander API.
- Shrink **primary** to **24 HP** — mod rack, Random, CC enables, master I/O only; no empty 50 HP dead zone.
- Rewrite `FieldParityWidget.hpp` as the single layout authority: panel HP constants, precomputed column centers (constexpr tables), row Y positions, bounds checks.
- Drop per-row `Label` widgets on expanders; row names live in `configParam`/`ParamDisplayNames` tooltips (page title label per column only).
- Fix `primaryModule()` to walk the full left-expander chain (required when Expander B sits between Expander A and primary).
- Add compile-time layout validation script (`sim/check_vcv_panel_bounds.sh`) and wire it into CI.
- Restore a clean x64 build/install path documented in `vcv/DEVELOPMENT.md`.

## Capabilities

### New Capabilities

- `vcv-rack-panel-layout`: Rack module topology (24 HP primary + 36 HP × 2 expanders), widget placement rules, browser-safe widget set, expander chain resolution, and layout bounds verification.

### Modified Capabilities

- (none — field-parity DSP/host behavior unchanged; this is widget topology and layout only)

## Impact

- `vcv/src/plugin.cpp` — module split, widget rewrite, expander chain
- `vcv/src/widgets/FieldParityWidget.hpp` — layout constants and helpers
- `vcv/plugin.json` — **BREAKING:** two expander model slugs replace one
- `sim/check_vcv_panel_bounds.sh` (new) + `.github/workflows/pages.yml` or VCV CI hook
- `vcv/DEVELOPMENT.md` — install/rebuild notes
- `openspec/changes/vcv-vst-field-parity-panel/` — cross-reference D3 fallback as implemented
