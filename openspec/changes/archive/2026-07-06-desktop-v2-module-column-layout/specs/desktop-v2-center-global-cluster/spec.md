## MODIFIED Requirements

### Requirement: Center global cluster column placement

Standalone desktop v2 SHALL place global randomize actions, **Crunchy**, and **Shift** in a **center column** bounded exclusively between the encoder column and mod column. The center column SHALL NOT be positioned as a z-ordered overlay on top of the submodule encoder viewport or mod cells.

#### Scenario: Center column disjoint from mod column

- **WHEN** the carousel lays out at default width
- **THEN** `CenterGlobalClusterV2` bounds width equals `kCenterGlobalClusterW`
- **THEN** center cluster right edge plus `kModuleRowModGap` equals mod column left edge

#### Scenario: Center column overflow remains contained

- **WHEN** the center cluster preferred control stack exceeds the center column height
- **THEN** `CenterGlobalClusterV2` uses compact spacing and enables internal vertical scrolling inside its assigned bounds
- **THEN** its bounds still do not intersect encoder or mod column bounds

#### Scenario: Same host mutations as GlobalStripV2

- **WHEN** the operator clicks Rand All, Rand Mods, Rand waveforms, Rand Resample, Crunchy, or Shift in the center column
- **THEN** the same control-core and `DesktopHostIO` mutations fire as the retired `GlobalStripV2`
