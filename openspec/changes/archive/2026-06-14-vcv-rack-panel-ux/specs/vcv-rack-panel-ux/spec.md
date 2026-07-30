## ADDED Requirements

### Requirement: Minimal panel SVG with branding

Each FroggersTiga module model SHALL call `setPanel(createPanel(...))` with light and dark SVG assets. Panel background SHALL be neutral gray. The text “FroggersTiga” SHALL appear as a tiny corner watermark (~6–8 pt Comic Sans or SVG-outlined equivalent), unobtrusive and clear of screws/jacks. All other silkscreen text SHALL be black, functional labels only — no decorative graphics.

#### Scenario: Brand present but unobtrusive

- **WHEN** any FroggersTiga module is placed at 100% zoom
- **THEN** “FroggersTiga” is visible in a corner without hover and does not compete with functional labels

#### Scenario: Primary legibility

- **WHEN** primary module is placed at 100% zoom
- **THEN** mod output jacks, audio/CV/MIDI ports, and CC enable switches show printed labels on the panel SVG

#### Scenario: Expander legibility

- **WHEN** Voicing or FX expander is placed at 100% zoom
- **THEN** each column shows a page title and each row shows a parameter name on the panel SVG, sourced from `ParamDisplayNames`

### Requirement: Standard Rack widget palette

Knobs SHALL use `RoundSmallBlackKnob` on expanders. Primary switches SHALL use `CKSS` and `TL1105`. Ports SHALL use `ThemedPJ301MPort`. No custom-painted widgets or oscilloscope displays.

#### Scenario: No oscilloscope on VCV

- **WHEN** primary module widget is built
- **THEN** mod rack shows green LED indicators only — no CV trace or scope widget

### Requirement: Green LED mod indicators

Mod rack outputs (VCO Envelope, Random 1, Random 2) and CC enable switches SHALL use `GreenLight` only — not `GreenRedLight`. Labels SHALL use “Random 1” and “Random 2” — never “marbles”.

#### Scenario: Mod LED threshold

- **WHEN** mod CV exceeds 55% of full scale while processing
- **THEN** the corresponding green LED is fully bright

#### Scenario: Random naming

- **WHEN** user reads mod output silkscreen or tooltip
- **THEN** labels say “Random 1” / “Random 2” or “random 1” / “random 2” — not “marbles”

### Requirement: Voicing and FX expander topology

The plugin SHALL expose **Froggers Tiga Voicing** (48 HP, pages 0,1,3,4) and **Froggers Tiga FX** (36 HP, pages 2,5). Expander A/B models SHALL be removed.

#### Scenario: Voicing pages

- **WHEN** Voicing expander is chained left of Primary
- **THEN** knobs and mod inputs for Audio, Random S&H, Filter, and Drive pages are present

#### Scenario: FX stereo jacks

- **WHEN** FX expander widget is built
- **THEN** stereo audio input (L/R) and stereo audio output (L/R) jacks are visible and labeled on the panel SVG

### Requirement: Primary control de-overlap

CC enable switches SHALL NOT share X/Y coordinates with gate, MIDI, or audio jacks on the primary module.

#### Scenario: CC vs gate separation

- **WHEN** primary panel bounds are checked
- **THEN** CC switch centers are ≥ 2 GRID away from gate jack center

### Requirement: Single label authority

Silkscreen strings SHALL derive from `ParamDisplayNames`. A separate abbrev table SHALL NOT be introduced. `configParam` / `configInput` / `configOutput` names SHALL remain for tooltips.

#### Scenario: Tooltip parity

- **WHEN** user hovers a Voicing row knob
- **THEN** tooltip shows `ParamDisplayNames::forHostPageRow` string matching silkscreen

### Requirement: Bounds and compile gate

`sim/check_vcv_panel_bounds.sh` SHALL validate 48 HP Voicing, 36 HP FX, and primary de-overlap constants. `make` in `vcv/` SHALL exit 0.

#### Scenario: CI bounds

- **WHEN** bounds script runs
- **THEN** exit code 0

### Requirement: VST and desktop unaffected

Public desktop and web sim hosts SHALL NOT require changes. VST local build SHALL continue using labeled `MainComponent` with scopes unchanged.

#### Scenario: Desktop unchanged

- **WHEN** desktop builds with `BUILD_VST=OFF`
- **THEN** standalone app layout and labels are unchanged by VCV panel UX work
