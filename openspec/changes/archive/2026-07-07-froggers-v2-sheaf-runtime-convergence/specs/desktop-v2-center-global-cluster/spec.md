## ADDED Requirements

### Requirement: Top global-command band replaces center global cluster
Standalone desktop v2 SHALL place global randomize actions, Crunchy, Shift, waveform-randomize, and Marbles/Rand Resample in the manifest-declared global-command band of the top chrome stack on normal module pages and parameter-detail pages. Global controls SHALL include the shared scene/step scope pairs for Randomize All and Randomize Mod and SHALL NOT be positioned as a z-ordered overlay on top of the submodule encoder viewport, compact module grid, parameter-detail source rack, sequencer controls, or mod cells.

#### Scenario: Top strip disjoint from module body
- **WHEN** the carousel lays out at default width
- **THEN** the global-command band bounds do not intersect compact module parameter cells, parameter-detail cells, sequencer controls, or mod-column cells
- **THEN** any legacy `CenterGlobalClusterV2` transition component is hidden or has empty bounds

#### Scenario: No hidden center cluster overlay
- **WHEN** global controls are rendered from the global-command band
- **THEN** no hidden center global cluster receives pointer events
- **THEN** no center global cluster bounds intersect encoder, module-grid, parameter-detail, sequencer, or mod-column bounds

#### Scenario: Global-command band frees parameter-detail body
- **WHEN** the parameter-detail view is shown
- **THEN** global Randomize All, Randomize Mod, waveform-randomize, Marbles, Crunchy, and Shift controls are reachable from the global-command band
- **THEN** the `All Scenes` / `Current Scene` and `All Steps` / `Current Step` scope pairs are visible below the global Randomize All and Randomize Mod buttons
- **THEN** the parameter-detail body can allocate its center area to the 4x4 parameter-detail encoder grid
- **THEN** no hidden center global cluster overlaps the source rack

#### Scenario: Global controls preserve host mutations
- **WHEN** the operator invokes Randomize All, Randomize Mod, waveform-randomize, Marbles/Rand Resample, Crunchy, Shift, or the scene/step randomization scope controls from the global-command band projection
- **THEN** the same control-core and `DesktopHostIO` mutations fire as the equivalent v2 global control before the projection change
- **THEN** migrating the controls does not introduce a second UI-owned state path for randomization, Crunchy, Shift, sequencer scope, scene scope, or Marbles/Rand Resample behavior
