## ADDED Requirements

### Requirement: Horizontal pair-AR band on Audio panel

On host page 0 (Audio) only, `SubModulePanel` SHALL render a horizontal band below the eight existing vertical rows containing four equal-width columns.

Each column SHALL stack controls top-to-bottom: **mod input jack**, **rotary knob**, **label** — inverted relative to vertical rows (label below knob).

#### Scenario: Band absent on non-Audio pages

- **WHEN** the user views Filter, Drive, or any non-Audio submodule
- **THEN** the pair-AR band is not visible

#### Scenario: Patch overlay includes pair-AR jacks

- **WHEN** the user engages patch-cable mode on desktop
- **THEN** four additional input ports appear on the Audio panel aligned to the pair-AR jack positions

### Requirement: Table-driven layout

Desktop pair-AR widget placement SHALL iterate `kAudioPairArCellCount` from `AudioPairArLayout.hpp` — no hardcoded fourfold duplicate layout blocks in `SubModulePanel.cpp`.

#### Scenario: Column count matches engine

- **WHEN** `kAudioPairArCellCount` is 4
- **THEN** exactly four columns are laid out in the band
