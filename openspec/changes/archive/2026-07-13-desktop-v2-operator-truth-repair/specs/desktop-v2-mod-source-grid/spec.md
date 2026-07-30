## MODIFIED Requirements

### Requirement: Permanent parameter modulation rack has fifteen Froggers source lanes
When a user opens a parameter's modulation detail view, desktop v2 SHALL expose a permanent 15-lane Froggers source rack. Each lane SHALL have an independent signed depth; depth zero SHALL mean that lane is off. Multiple lanes with nonzero depth on the same row SHALL sum (eligibility-gated) into the row's effective modulation. Depth editing SHALL NOT require a prior compact-picker or dropdown assignment step.

#### Scenario: Multi-lane depths sum when eligible
- **WHEN** two or more eligible lanes on one row have nonzero depth
- **THEN** the row's effective modulation and attenuated arc reflect the sum of those eligible lane contributions
- **THEN** ineligible lanes with leftover nonzero depth contribute nothing to effective, arc bounds, or modulatorsMask

#### Scenario: Unavailable lanes stay visible but disabled
- **WHEN** external audio input is absent
- **THEN** External Audio (audio rate) and External Audio (envelope follower) remain visible in the 4×4 detail grid
- **THEN** those cells are greyed/disabled and refuse turn and clear-press edits
- **WHEN** the operator opens detail for a VCO-owned row whose pair-bus self-feedback lanes are blocked by the manifest
- **THEN** those blocked pair-bus cells remain visible but greyed/disabled and refuse edits

#### Scenario: Direct ModDrillIn opens detail without prior assignment
- **WHEN** the operator activates ModDrillIn for a row (center MOD LED click, or a mapped encoder press)
- **THEN** the 4×4 parameter-detail grid opens for that row even if every lane depth is still zero
- **THEN** ring drag / encoder rotation continues to dispatch ParamTurn and does not open detail

### Requirement: Parameter detail uses a 4x4 encoder grid
Desktop v2 parameter-detail modulation view SHALL render sixteen encoder cells at the default standalone size: fifteen source-depth encoders plus one dedicated **Target (Back)** encoder. The Target (Back) cell SHALL remain visible while source depths are edited and SHALL exit detail on ParamPress.

#### Scenario: Sixteenth cell is Target (Back)
- **WHEN** the user opens modulation detail for a manifest-eligible parameter at 1280x920
- **THEN** cell 15 is labeled **Target (Back)**
- **THEN** ParamPress on that cell closes detail and restores the module page

#### Scenario: Module-row mod dropdown column is retired
- **WHEN** the operator views a carousel module page
- **THEN** no ModLanePicker / mod-column dropdown is present beside encoder rows
- **THEN** modulation editing happens through ModDrillIn into the 4×4 depth grid

### Requirement: Encoders expose modulation through attenuated-centered CV LED and MOD drill-in
Desktop v2 parameter encoders SHALL show modulation state through a fixed-center CV LED / MOD affordance. The center MOD/CV LED SHALL be the sole mouse hit target that opens parameter-detail modulation (`ModDrillIn`). The ring annulus SHALL drag-edit via `ParamTurn` and SHALL NOT open detail.

#### Scenario: MOD LED opens parameter detail
- **WHEN** the user clicks the center MOD/CV LED on a parameter encoder
- **THEN** the UI opens the 4×4 parameter-detail modulation grid for that parameter
- **WHEN** the user drag-turns the ring annulus
- **THEN** the parameter value (or open detail-lane depth) changes without opening or closing detail
