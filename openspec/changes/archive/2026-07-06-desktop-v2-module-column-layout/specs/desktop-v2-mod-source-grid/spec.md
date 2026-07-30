## MODIFIED Requirements

### Requirement: Mod source cell fixed footprint

Mod source cells SHALL use width at least `kModCellW` (18u) within a dedicated mod column whose horizontal bounds do not overlap the center global cluster. Cells SHALL NOT be placed at absolute X inside a full-width encoder content surface that shares pixels with the center column overlay.

#### Scenario: Mod column exclusive band

- **WHEN** submodule panel lays out mod cells at default window width
- **THEN** each mod cell X origin equals 0 inside the mod column content
- **THEN** each mod cell page-space left edge equals the mod column container page-space left edge derived from `moduleRowColumns(rowWidth).modX`
- **THEN** mod cell page-space bounds do not intersect center cluster page-space bounds

#### Scenario: Longest label without ellipsis

- **WHEN** a mod cell displays the longest catalog label at default 1280×920
- **THEN** the assigned-source label strip does not truncate to ellipsis
