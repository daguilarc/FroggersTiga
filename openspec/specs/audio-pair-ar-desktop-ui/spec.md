# audio-pair-ar-desktop-ui Specification

## Purpose
Render the desktop Audio page pair-AR horizontal band with patch jacks, rotaries, and rotated labels aligned to shared layout constants.
## Requirements
### Requirement: Horizontal pair-AR band on Audio panel

On host page 0 (Audio) only, `SubModulePanel` SHALL render a horizontal band below the eight existing vertical rows containing four equal-width columns.

Each column SHALL stack controls top-to-bottom: **mod input jack**, **rotary knob**, **label** — inverted relative to vertical rows (label below knob).

Pair-AR **labels** SHALL render the full `ParamDisplayNames::forAudioPairAr` string rotated **90 degrees clockwise** (not horizontal). Labels SHALL NOT truncate at default module width (300 px).

#### Scenario: Band absent on non-Audio pages

- **WHEN** the user views Filter, Drive, or any non-Audio submodule
- **THEN** the pair-AR band is not visible

#### Scenario: Patch overlay includes pair-AR jacks

- **WHEN** the user engages patch-cable mode on desktop
- **THEN** four additional input ports appear on the Audio panel aligned to the pair-AR jack positions

#### Scenario: Rotated labels fully legible

- **WHEN** the user views the Audio submodule at default panel width
- **THEN** all four pair-AR labels show complete text (e.g. **Release 1+2**), not clipped prefixes like **Rele**

### Requirement: Table-driven layout

Desktop pair-AR widget placement SHALL iterate `kAudioPairArCellCount` from `AudioPairArLayout.hpp` — no hardcoded fourfold duplicate layout blocks in `SubModulePanel.cpp`.

#### Scenario: Column count matches engine

- **WHEN** `kAudioPairArCellCount` is 4
- **THEN** exactly four columns are laid out in the band

