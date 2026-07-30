## Why

The shipped VCV Rack plugin packs six submodule columns (48 knobs + 48 mod jacks + 48 row labels) onto a single 72 HP expander while the primary module is also 72 HP but only uses ~22 HP of widgets. Column widgets overlap (10 HP labels + knob + jack span ~12 HP but are placed on 12 HP centers), so most controls are illegible or off-panel. Selecting the module in Rack's browser preview triggers instability (Framebuffer 0×0 warnings, fatal signals during widget teardown). A partial layout refactor in `FieldParityWidget.hpp` left the tree non-compiling (`panelSize`, `addRowLabel`, and `columnCenterX` signature drift). The `vcv-vst-field-parity-panel` design already documented the **3+3 expander fallback** when six columns do not fit at readable density — that fallback was never applied.

**Omni audit (2026-06-13):** Planning artifacts are complete; implementation is ~5% started (stub header only). All 22 original tasks remain unchecked. Repo HEAD does not compile. `sim/check_vcv_panel_bounds.sh` does not exist. This proposal update captures audit gaps, merge gates, and VST parallel analysis.

## What Changes

- **BREAKING:** Replace single `FroggersTigaExpander` (72 HP × 6 columns) with two expanders: **Expander A** (36 HP, pages 0–2) and **Expander B** (36 HP, pages 3–5), chained left of primary via Rack expander API.
- Shrink **primary** to **24 HP** — mod rack, Random, CC enables, master I/O only; no empty 50 HP dead zone.
- Rewrite `FieldParityWidget.hpp` as the single layout authority: panel HP constants, precomputed column centers (constexpr tables), row Y positions, bounds checks.
- Drop per-row `Label` widgets on expanders; row names live in `configParam`/`ParamDisplayNames` tooltips (page title label per column only).
- Fix `primaryModule()` to walk the full left-expander chain (required when Expander B sits between Expander A and primary).
- Add compile-time layout validation script (`sim/check_vcv_panel_bounds.sh`) and wire it into CI.
- Restore a clean x64 build/install path documented in `vcv/DEVELOPMENT.md`.
- **Audit additions:** Widget-level DRY (primary I/O table, corner screws, unified column offset); `HostPanelLayout.hpp` include fix; H5 browser-crash decision gate; merge gate requiring `make` exit 0 before header API renames land.
- **VST scope:** VCV panel topology changes do **not** apply to the JUCE VST — it reuses `MainComponent` with resizable pixel layout. VST and `vcv/` remain **local-only** (not published on `main`); default `BUILD_VST=OFF` on public CI.

## Capabilities

### New Capabilities

- `vcv-rack-panel-layout`: Rack module topology (24 HP primary + 36 HP × 2 expanders), widget placement rules, browser-safe widget set, expander chain resolution, layout bounds verification, and local-only build policy for VCV/VST targets.

### Modified Capabilities

- (none — field-parity DSP/host behavior unchanged; this is widget topology and layout only)

## Impact

- `vcv/src/plugin.cpp` — module split, widget rewrite, expander chain
- `vcv/src/widgets/FieldParityWidget.hpp` — layout constants and helpers
- `vcv/plugin.json` — **BREAKING:** two expander model slugs replace one
- `sim/check_vcv_panel_bounds.sh` (new) + `.github/workflows/pages.yml` or VCV CI hook
- `vcv/DEVELOPMENT.md` — install/rebuild notes
- `openspec/changes/vcv-vst-field-parity-panel/` — cross-reference D3 fallback as implemented
- `desktop/CMakeLists.txt` — default `BUILD_VST=OFF` for public/CI builds (VST stays local-only like `vcv/`)
- `.git/info/exclude` — document VST build artifacts local-only policy (no VST promotion to `main` until explicit release)

## VST parallel analysis (does not block VCV apply)

| VCV issue | Applies to VST? | Reason |
|-----------|-----------------|--------|
| HP panel overlap / illegible widgets | **No** | JUCE `MainComponent::resized()` divides width by 6 panels dynamically |
| Rack browser preview crash | **No** | DAW plugin scanner does not instantiate Rack nanovg widget trees |
| Expander chain / `primaryModule()` | **No** | Single `AudioProcessor` instance; no multi-module linking |
| 72 HP dead zone on primary | **No** | Resizable editor (default 1440×720 px); no fixed HP panels |
| Partial refactor compile break | **No** | VST does not include `FieldParityWidget.hpp` or Rack SDK |
| `ParamDisplayNames` label authority | **Shared** | VST uses same `MainComponent` labels; PM3/VCO Envelope parity is `sim-pm3-knob-parity`, not this change |
| Local-only repo policy | **Yes** | `vcv/` already in `.git/info/exclude`; VST sources are on `main` today — policy change required to keep VST off public branch |

**VST architecture (reference):** `FroggersTigaPlugin` = JUCE `AudioProcessor` + `AudioProcessorEditor` embedding `MainComponent`. Audio flows through `DesktopHostIO` + `FroggersEngine` at DAW sample rate. Layout is pixel-based via `HostPanelLayout` constants (`kDefaultWidth=1440`, `kDefaultHeight=720`), not Eurorack HP. No Rack expander API, no `box.size` constraints.

**Minor VST discrepancy (out of scope here):** `PluginEditor.cpp` sets resize minimum 1024×600; `juce-vst-plugin` spec says 1440×720. Track in `vcv-vst-field-parity-panel`, not this change.
