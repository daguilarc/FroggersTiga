## Why

Operator QA on the Release build (1280×920) shows the **layout-density** change failed its primary goal: `CenterGlobalClusterV2` renders **on top of** mod source dropdowns, Shift overlaps encoder rows, performance-band labels truncate to `...`, and the module viewport scrolls despite ~790px of unused horizontal space and dead vertical space in the sequencer panel. Root cause is architectural: the center cluster is a **z-ordered overlay sibling** in `PageCarouselComponent`, while mod cells use **absolute X offsets inside a full-width viewport** — two independent layout systems with no collision guarantee. The grid constants in `DesktopV2ChromeLayout.hpp` describe a four-column row, but nothing enforces exclusive column bounds at runtime.

## What Changes

- **Replace overlay with exclusive column layout:** `PageCarouselComponent` (or a new `ModulePanelLayout`) SHALL partition carousel body width into **label | encoder | center-global | mod** columns. `CenterGlobalClusterV2` occupies the center column bounds only; mod cells occupy the mod column only — no `setBounds` overlap at default window size.
- **Single layout authority:** Add `ModuleRowColumnLayout` (constants + `columnBounds(rowWidth)`) consumed by `SubmodulePagePanel`, `AdsrPagePanel`, and `PageCarouselComponent`. Derive `kModuleRowModX` from column sums — never hardcode independent X constants that can drift.
- **Vertical budget fix:** Size encoder viewport to `min(documentHeight, availableHeight)`; enable scroll **only** when document exceeds available height. Reclaim sequencer panel dead space (toolbar + centered grid) by top-aligning step grid and using flex remainder for carousel when rows fit.
- **Truncation fixes:** Performance-band marbles labels (`kPerfMarblesLabelH` = 1u is too small); scene/gesture controls — measure label widths or increase constants. Mod combo labels at 18u width without ellipsis at default width.
- **Layout regression gate:** Add `LayoutBounds_test.cpp` — after layout at 1280×920, assert zero intersection between center-cluster bounds and mod-cell bounds on Audio page; assert 8-row Audio page needs no vertical scroll.
- **AdsrPagePanel parity:** Apply same column layout (currently still diverges from submodule panel).

## Capabilities

### New Capabilities

- `desktop-v2-module-column-layout`: Exclusive four-column module geometry, single layout authority, vertical scroll policy, layout bounds regression test.

### Modified Capabilities

- `desktop-v2-grid-layout`: Column X offsets derived from sum of prior column widths; document `kModuleRowModX` as computed, not magic 31u.
- `desktop-v2-center-global-cluster`: Center cluster is a layout column participant, not a carousel overlay sibling.
- `desktop-v2-page-carousel`: Carousel body uses horizontal column split; remove overlay `setBounds` on top of submodule viewport.
- `desktop-v2-mod-source-grid`: Mod cells laid out in dedicated mod column with width `max(kModCellW, remainder)`.
- `desktop-v2-performance-band-chrome`: Marbles label height and minimum widths fix truncation at 1280px.

## Impact

- `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp` — column layout helper; fix `kPerfMarblesLabelH`; derive mod X.
- `desktop-v2/Source/ui/PageCarouselComponent.*` — column split; remove overlay positioning.
- `desktop-v2/Source/ui/SubmodulePagePanel.*`, `AdsrPagePanel.*` — consume column layout; viewport width = encoder column only.
- `desktop-v2/Source/ui/CenterGlobalClusterV2.*` — parent bounds = center column; cluster uses compact spacing and internal scroll when its preferred stack height exceeds column height.
- `desktop-v2/Source/ui/PerformanceBandV2.*` — label sizing.
- `desktop-v2/Source/ui/SequencerPanelComponent.*` — top-align grid; reduce dead space.
- `desktop-v2/tests/LayoutBounds_test.cpp` — new regression gate.
- `HostedMainComponentV2` — same column layout via shared carousel.

## Evidence (verified 2026-07-04, screenshot + source)

| Claim | Verification |
|-------|----------------|
| Center cluster is overlay sibling | `PageCarouselComponent.cpp` L139–145: `m_submodulePanel.setBounds(area)` then `m_centerCluster.setBounds(area.getX() + kModuleRowCenterClusterX, …)` — cluster added after panel, paints on top |
| Mod at fixed X inside full-width viewport | `SubmodulePagePanel.cpp` L209–213: mod at `kModuleRowModX` within `m_encoderContent` sized to full viewport width |
| 10px gap cluster→mod | Cluster ends 300px, mod starts 310px — z-order makes cluster occlude mod hit targets; Shift stack spans encoder row Y at overlapping visual band |
| Marbles label height 1u | `DesktopV2ChromeLayout.hpp` L71: `kPerfMarblesLabelH = gridPx(1)` — 10px label strip |
| Sequencer dead space | `SequencerPanelComponent.cpp` L421–422: grid centered in remainder with `gridY = area.getY() + (area.getHeight() - gridH) / 2` |
| Scroll despite fit potential | Viewport always `setScrollBarsShown(true, false)`; content height = rows × 50px without comparing to viewport |
| ~790px horizontal dead space | Mod column ends ~490px on 1280px window — screenshot confirms vast right void |

## OMNI rule audit (2026-07-04)

| Rule | Violation | Fix |
|------|-----------|-----|
| Data flow | Constants → two layout paths → overlapping bounds | Single `ModuleRowColumnLayout` pipeline |
| Repetition | Submodule + Adsr duplicate row X math | Shared column helper (≥2 callers) |
| Verification | Layout change shipped without bounds gate | `LayoutBounds_test.cpp` |
| Defensive code | Overlay “works on paper” with 10px gap | Exclusive columns — overlap impossible by construction |
| Plan language | — | Deterministic SHALL/MUST in specs |

## OMNI rule audit refresh (2026-07-05)

| Rule | Finding | Artifact update |
|------|---------|-----------------|
| Contract honesty | Mod-column spec mixed container-local and page-local X coordinates | Specs now require mod cells at x=0 inside the mod column container and compare absolute bounds only in the bounds test |
| Single authority / data flow | Tasks lacked explicit gates proving all layout consumers call the helper | Tasks now include helper-use grep/audit gates for carousel, Submodule, and ADSR |
| Repetition / helper extraction | Design left mod placement as “sibling viewport or panel region” | Design now selects one sibling `modColumnViewport` implementation with synced scroll |
| Plan language | Proposal used conditional hedge language for center-cluster overflow | Proposal now states compact spacing plus internal scroll as the required overflow behavior |
| Local reasoning / nesting | Tasks only mentioned nesting in one late item | Tasks now require the nesting audit as an OMNI gate before merge |

## Relationship to prior changes

- **`desktop-v2-layout-density-write-seq` (archived):** Implemented overlay center cluster — **this change supersedes D1 overlay decision** with exclusive columns. Keeps center-cluster widgets and constants intent.
- **`desktop-v2-chrome-sequencer-ux` (in progress):** Submodule left-anchor done; performance-band truncation open — absorbed here.
