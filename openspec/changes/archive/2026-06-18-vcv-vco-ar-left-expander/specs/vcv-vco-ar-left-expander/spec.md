## ADDED Requirements

### Requirement: VCO AR left expander module exists

The VCV plugin SHALL register a third module **`Froggers Tiga VCO AR`** designed as a **left expander** linked to the main `Froggers Tiga` module via Rack `leftExpander`.

The module SHALL expose six knobs (Attack and Release for VCO1, VCO2, VCO3), one **Crispy** knob, and **Randomize** + **Randmod** momentary buttons.

#### Scenario: Module appears in library

- **WHEN** a user browses the FroggersTiga plugin in Rack
- **THEN** `Froggers Tiga VCO AR` is listed alongside main and voicing expander modules

#### Scenario: Left expander link

- **WHEN** the user places VCO AR immediately to the left of the main module
- **THEN** the expander link connects and main `process()` receives VCO AR knob values

### Requirement: Panel layout uses shared layout authority

Panel geometry SHALL be defined in `VcvPanelLayout.hpp` (`kVcoArHp`, column pitch, row anchors). Silkscreen labels SHALL come from `ParamDisplayNames::forVcvVcoAr` via path-based SVG generation — no hardcoded label strings in `plugin.cpp`.

#### Scenario: Label parity

- **WHEN** the VCO AR faceplate is rendered at 100% zoom
- **THEN** knob labels read Att. VCO1, Rel. VCO1, Att. VCO2, Rel. VCO2, Att. VCO3, Rel. VCO3

#### Scenario: CI bounds

- **WHEN** `sim/check_vcv_panel_bounds.sh` runs on the VCO AR SVG
- **THEN** all widgets fit within the declared HP without clipping the screw band

### Requirement: Patch layout documented

`vcv/DEVELOPMENT.md` SHALL document the patch order: **VCO AR (left) → Main → Voicing expander (right) → FX (optional)**.

#### Scenario: Developer setup

- **WHEN** a developer reads DEVELOPMENT.md
- **THEN** the left-expander placement requirement is explicit
