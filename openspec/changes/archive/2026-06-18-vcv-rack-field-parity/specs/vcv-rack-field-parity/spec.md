## ADDED Requirements

### Requirement: Field-parity usable without hover

The VCV FroggersTiga main and FX modules SHALL together provide the same **randomize mutations**, **VCO morph controls**, **mod-rack patch topology**, and **readable faceplate identification** as web and desktop, without requiring tooltip hover as the primary UI.

#### Scenario: User identifies controls at 100% zoom

- **WHEN** modules are placed at 100% rack zoom without hover
- **THEN** faceplate silkscreen identifies columns, rows, I/O, mod rack, randomize buttons, and mod cell names
- **THEN** tooltips supplement but do not replace faceplate labels

#### Scenario: Interactive controls visible without hover

- **WHEN** the user inspects the main module at 100% zoom
- **THEN** wave-morph icons on Audio rows 0–2 and mod-rack green LEDs are visible without hover
- **THEN** no mod-rack CV trace or oscilloscope widgets are present on VCV

#### Scenario: Wave morph icon is not a mod-rack scope

- **WHEN** the user inspects a VCO morph widget on Audio row 0–2
- **THEN** it displays a static one-cycle wave shape from the current morph parameter
- **THEN** it does not trace live mod CV or audio over time

### Requirement: Randomize control surface

VCV SHALL expose the same randomize actions as web `index.html` and desktop `GlobalStrip` + `SubModulePanel`.

#### Scenario: Per-column knob randomize

- **WHEN** the user triggers Randomize on the Audio column
- **THEN** only Audio page knobs randomize (`RandomizePage(0)`), matching desktop SubModulePanel

#### Scenario: Per-column mod randomize

- **WHEN** the user triggers Randmod on the Filter column
- **THEN** only Filter page mod routes randomize (`RandomizePageMod(3)`), respecting MIDI CC gating

#### Scenario: Global randomize strip

- **WHEN** the user triggers Rand All, Rand Mods, Random (marbles), or Rand waveforms on the main module
- **THEN** the engine mutation matches desktop `GlobalStrip` for that button

#### Scenario: FX delay randomize

- **WHEN** the user triggers Randomize or Randmod on the Delay column of the FX module
- **THEN** `DelayState` knob/mod randomize runs, matching desktop Delay panel

#### Scenario: Random is marbles, not Rand All

- **WHEN** the user triggers the faceplate **Random** control
- **THEN** marbles bags step (`ButtonCallback(0)`)
- **THEN** all-page knob randomize does **not** run unless **Rand All** is pressed

### Requirement: Mod rack and VCO morph parity

VCV SHALL match desktop mod-rack **topology** and Audio-column VCO morph UI as specified in `vcv-mod-rack-scopes` and `vcv-vco-morph-controls`. Mod-rack **visual feedback** on VCV is LED-only by design.

#### Scenario: End-to-end mod patch parity

- **WHEN** a user patches VCO Envelope mod to Audio VCO1 on desktop
- **THEN** the same patch produces equivalent timbre on VCV using mod rack VCO Envelope output

#### Scenario: VCO morph audible parity

- **WHEN** the user cycles VCO1 morph on VCV and desktop with identical knob settings
- **THEN** output timbre changes match for each morph step

### Requirement: Stereo FX routing when expander linked

VCV SHALL match desktop stereo delay/reverb output behavior as specified in `vcv-stereo-fx-routing` (Option C via `applyStereoBus`).

#### Scenario: FX outputs differ under stereo width

- **WHEN** FX expander is linked and delay width or reverb width is active
- **THEN** FX L and R outputs are not identical copies of main audio out

### Requirement: Verification before close

No field-parity panel task SHALL be marked complete without passing automated SVG/layout checks and documented manual Rack verification at 100% zoom.

#### Scenario: CI SVG gate

- **WHEN** `sim/check_vcv_panel_svg.sh` runs on `vcv/res/FroggersTiga*.svg`
- **THEN** no live `<text>` elements remain

#### Scenario: Manual Rack gate

- **WHEN** a reviewer completes the change
- **THEN** screenshots at 100% zoom show main + FX labels, randomize buttons, wave-morph widgets, and five mod-rack LEDs in expected positions

### Requirement: Single layout authority

VCV panel geometry for new controls SHALL be defined only in `VcvPanelLayout.hpp`. Widget placement, SVG generation, and bounds CI SHALL read the same constants — no parallel coordinate tables.

#### Scenario: New control anchors centralized

- **WHEN** global strip, column action row, mod rack, or wave-morph positions are defined
- **THEN** `kGlobalStripGridY`, `kColumnActionGridY`, `kModRackCellCount`, and `kWaveMorphGridOffsetX` exist in `VcvPanelLayout.hpp`
- **THEN** `generate_panels.py` derives silkscreen positions from that header, not duplicated inline math

#### Scenario: Bounds CI covers new regions

- **WHEN** `sim/check_vcv_panel_bounds.sh` runs after layout changes
- **THEN** it validates global strip, column actions, five mod cells, and wave-morph tails within panel HP

### Requirement: Mod rack wired from one cell table

Mod rack outputs, LEDs, and `process()` CV assignment SHALL iterate a single static cell table covering mod indices 0, 1, 4, 5, 6 — not hardcoded per-index copy-paste.

#### Scenario: No triplicate mod light updates

- **WHEN** mod rack LED brightness is updated each block
- **THEN** one loop over the cell table sets each light from `GetCvOut(cell.modIndex)` at threshold 0.55

#### Scenario: Widget count matches table

- **WHEN** the mod rack widget is constructed
- **THEN** output jack and LED count equals `kModRackCellCount` (5)
- **THEN** faceplate silkscreen label count matches widget count
