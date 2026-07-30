## ADDED Requirements

> **D16 (2026-07-24):** the parameter/randomization authority is Sheaf-native (`ParameterManager`/`ParameterGroup`/`Bank`/`SceneState`), NOT the bespoke `FroggersV2ControlCore`, which is retired. Read every "control core" / "existing randomization authority" reference below as the Sheaf `Bank`/`ParameterManager` model. Behavioral requirements are unchanged; only the implementing authority changes. Modulation view/gate maps to `Bank::OpenModulationView`/`CanOpenModulationView`/`Deselect`.

### Requirement: Modulation drill-in enforces two layers
The parameter/modulation authority (Sheaf `Bank`, via `CanOpenModulationView`) SHALL refuse opening a nested modulation view when already in parameter-detail mod view on a depth cell. Enter-mod from layer 0 opens the 16-cell detail grid. Target/Back (`Bank::Deselect`) closes mod view.

#### Scenario: Second drill-in rejected
- **WHEN** mod view is open and a ModDrillIn targeting a depth lane arrives
- **THEN** the control core does not open a nested mod view
- **THEN** layer-1 depth editing state remains the active mod context

### Requirement: Rand arm rides the Sheaf randomization authority
The control layer SHALL implement rand toggle global mode and held next-click-local arm per `desktop-v2-rand-arm-gesture`, using the Sheaf `Bank`/`ParameterManager` randomization (`ApplyModifierToTopLevel(Random)` / `Modifier::RandomMod`) as the single mutator — no parallel mutator. Scope is per-page + global (values and mod-depths); sequencer/gesture rand is retired (D16).

#### Scenario: Local arm consumes one ParamPress
- **WHEN** local rand arm is active and ParamPress arrives for a module parameter
- **THEN** that parameter is randomized once
- **THEN** the arm clears

### Requirement: Sixteen-slot encoder bank map
The control core / UI bank map SHALL expose up to sixteen physical encoder slots for first-layer and detail grids. Module pages MAY use fewer than sixteen connected cells. Global Crunchy and per-page Crispy SHALL occupy stable slot identities across module section changes.

#### Scenario: Sparse page leaves disconnected cells
- **WHEN** the active module section has fewer than sixteen parameters including Crispy
- **THEN** unused encoder slots are disconnected or empty
- **THEN** Crunchy and Crispy slot identities do not change solely because the active section changed
