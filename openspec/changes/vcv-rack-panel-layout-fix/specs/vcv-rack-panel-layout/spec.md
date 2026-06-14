## ADDED Requirements

### Requirement: Three-module Rack topology

The VCV plugin SHALL expose three module models: **Froggers Tiga** (primary), **Froggers Tiga Expander A**, and **Froggers Tiga Expander B**. Primary SHALL be 24 HP wide. Each expander SHALL be 36 HP wide and 380 px tall (one Rack row). Expander A SHALL cover host pages 0–2; Expander B SHALL cover host pages 3–5.

#### Scenario: Full field layout in Rack

- **WHEN** user places Primary, Expander A, and Expander B with expanders chained left of Primary
- **THEN** all six submodule pages are accessible with 8 knobs and 8 mod inputs each

#### Scenario: Primary panel width

- **WHEN** Primary module widget is created
- **THEN** `box.size.x` equals `24 × RACK_GRID_WIDTH`

### Requirement: Non-overlapping column layout

Each expander column SHALL occupy exactly 12 HP. Knob and mod input widgets SHALL fit entirely within their column bounds at 100% Rack zoom. The plugin SHALL NOT render per-row text labels on expander panels (row names SHALL remain in param tooltips via `ParamDisplayNames`).

#### Scenario: Three columns on 36 HP expander

- **WHEN** Expander A widget is built
- **THEN** exactly three columns are visible with page titles Audio, Random S&H, and Reverb

#### Scenario: Column bounds

- **WHEN** `sim/check_vcv_panel_bounds.sh` runs in CI
- **THEN** it exits 0 confirming all widget positions fit within panel HP constants

### Requirement: Expander chain primary resolution

Expander modules SHALL locate the primary `FroggersTigaModule` by walking the `leftExpander.module` chain until a `FroggersTigaModule` is found or the chain ends. Expander B SHALL successfully sync when chained as `[Expander B] ← [Expander A] ← [Primary]`.

#### Scenario: Two-hop chain

- **WHEN** Expander B’s left expander is Expander A and Expander A’s left expander is Primary
- **THEN** Expander B `process()` updates host pages 3–5 on the shared `PagedHostIO`

### Requirement: Module browser stability

Selecting any FroggersTiga model in Rack’s module browser SHALL NOT crash Rack. Widget trees SHALL NOT place interactive widgets outside panel bounds or with zero-size framebuffer children.

#### Scenario: Browser preview primary

- **WHEN** user highlights Froggers Tiga in the module browser
- **THEN** Rack shows a preview without fatal signal

#### Scenario: Browser preview expander

- **WHEN** user highlights Froggers Tiga Expander A or B in the module browser
- **THEN** Rack shows a preview without fatal signal

### Requirement: Cached primary pointer on expanders

Expander modules SHALL cache the resolved primary module pointer when the expander link changes and SHALL invalidate the cache when the link breaks. `process()` SHALL NOT call `dynamic_cast` on every sample when the cached pointer is valid.

#### Scenario: Link established

- **WHEN** expander is connected left of Primary
- **THEN** first `process()` after link resolves primary once and reuses the pointer on subsequent calls until unlink

### Requirement: Layout compile gate

The repository SHALL provide `sim/check_vcv_panel_bounds.sh` validating widget placement against `FieldParityWidget.hpp` constants. The VCV plugin SHALL compile cleanly against Rack SDK 2.4.1 with no references to removed layout APIs (`panelSize`, `addRowLabel`, two-argument `columnCenterX`).

#### Scenario: CI bounds check

- **WHEN** CI runs `sim/check_vcv_panel_bounds.sh`
- **THEN** the script passes

#### Scenario: Clean compile

- **WHEN** `make` runs in `vcv/` with `RACK_DIR` set to Rack SDK 2.4.1
- **THEN** build completes with exit code 0
