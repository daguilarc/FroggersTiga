## ADDED Requirements

### Requirement: Modulation drill-in depth is at most two layers
Desktop v2 SHALL allow modulation drill-in from a first-layer module parameter into the 16-cell parameter-detail mod page (fifteen source-depth encoders plus Target/Back). The system SHALL reject opening a further modulation page from a depth cell. Recursion depth greater than 1 for UI drill-in is forbidden.

#### Scenario: Depth cell press does not open nested mod page
- **WHEN** parameter-detail mod view is open
- **AND** the operator presses a source-depth cell
- **THEN** the UI does not open a second nested mod page
- **THEN** the press applies the depth-cell interaction defined for layer 1 only (select/edit depth), not ModDrillIn to layer 2

#### Scenario: Target Back returns to layer 0
- **WHEN** the operator presses Target (Back) on the detail grid
- **THEN** mod view closes and the first-layer module surface is shown

### Requirement: Random S&H exists only as mod lanes
Desktop v2 SHALL expose Random S&H 1 and Random S&H 2 only as permanent modulation source lanes with depth editing and Sheaf-style ganged visualizers. Desktop v2 SHALL NOT provide a Random S&H module page or module-section.

#### Scenario: No Random module page in carousel or unified surface
- **WHEN** the operator navigates module sections on the Application surface
- **THEN** no Random S&H module section or page is listed
- **THEN** Random S&H 1/2 remain available as mod-depth lanes on other parameters’ detail grids
