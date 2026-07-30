## ADDED Requirements

### Requirement: Monophonic Sheaf parameter model
All Froggers parameters SHALL be registered through Sheaf's `ParameterManager` into `ParameterGroup`s configured with `numVoices = 1`. Froggers value scaling SHALL be preserved for every ported parameter. No bespoke parameter, inventory, or randomization model SHALL be introduced beside Sheaf's.

#### Scenario: Groups are monophonic
- **WHEN** the app initializes its parameter groups
- **THEN** every group reports `numVoices == 1`
- **THEN** each encoder renders a single value ring rather than stacked per-voice arcs

#### Scenario: Single authority
- **WHEN** the app is inspected for parameter state
- **THEN** exactly one `ParameterManager` and one `BankSlot` exist
- **THEN** no parallel parameter table, page-state, or randomization mutator exists

### Requirement: One sixteen-slot bank per Froggers page
Each existing Froggers page SHALL become exactly one bank of sixteen parameter slots. Pages SHALL NOT be merged. A bank's own parameters SHALL occupy the leading slots; remaining parameter slots MAY be empty.

#### Scenario: Page identity is preserved
- **WHEN** the banks are enumerated
- **THEN** there is one bank per original Froggers page
- **THEN** each bank contains that page's parameters and no other page's

#### Scenario: Sparse banks are valid
- **WHEN** a bank has fewer parameters than available slots
- **THEN** the unused slots render as empty
- **THEN** the occupied slots keep their positions rather than being renumbered

#### Scenario: Every bank holds nine parameters
- **WHEN** the six banks are enumerated
- **THEN** each holds nine parameters at slot indices 0 through 8, without every one of the nine necessarily originating as a page row
- **THEN** the Audio bank is the exception that makes up its nine: three of its nine are the VCO Shape controls rather than page rows
- **THEN** slot indices 9 through 13 are empty in every bank

### Requirement: Fixed global control slots
Bank slots SHALL be indexed from zero (`0..15`). In every bank, the local **Crispy** control SHALL occupy slot index **14** and the global **Crunchy** control SHALL occupy slot index **15**. These positions SHALL be identical across all banks and SHALL NOT change when the active bank changes.

#### Scenario: Global controls never move
- **WHEN** the operator switches from one bank to another
- **THEN** Crispy remains at slot index 14
- **THEN** Crunchy remains at slot index 15

#### Scenario: Crunchy is one global control
- **WHEN** Crunchy is adjusted from any bank
- **THEN** the same single global value changes
- **THEN** no per-bank copy of Crunchy exists

### Requirement: Waveform Shape controls are ordinary bank slots
The three VCO waveform **Shape** controls SHALL be registered as ordinary parameters in the Audio bank, not as a separate global axis outside the grid.

#### Scenario: Shape appears on the grid
- **WHEN** the Audio bank is displayed
- **THEN** the three Shape controls occupy ordinary encoder slots
- **THEN** they are addressable, modulatable, and randomizable like any other parameter

### Requirement: Each bank carries its own color
Every bank SHALL have a distinct bank color. That color SHALL be realized by giving every one of that bank's parameters that color; declaring a color on the bank alone, without propagating it to the bank's parameters, SHALL NOT render anything. Because the global Crunchy control is a single shared parameter appearing in all six banks, it SHALL carry one fixed color rather than taking on each bank's color.

#### Scenario: Banks are visually distinguishable
- **WHEN** the operator switches banks
- **THEN** the encoder grid renders in that bank's distinct color
- **THEN** that color is present because it has been applied to every parameter in the bank, not merely declared on the bank

#### Scenario: Crunchy keeps one fixed color across banks
- **WHEN** the operator switches from one bank to another
- **THEN** the global Crunchy control's color does not change to match the newly active bank
- **THEN** Crunchy renders in its own fixed color in every bank

### Requirement: Global Crunchy is one shared parameter, not six copies
The global Crunchy control SHALL be a single shared parameter occupying slot index 15 in all six banks. It SHALL NOT be six independent per-bank copies.

#### Scenario: Adjusting Crunchy from any bank moves the same value
- **WHEN** Crunchy is adjusted while any bank is active
- **THEN** the same single underlying value changes
- **THEN** switching to a different bank shows that same changed value at slot index 15

#### Scenario: Crunchy is published once
- **WHEN** the app's published parameters are enumerated
- **THEN** Crunchy appears once, not once per bank
- **THEN** no per-bank Crunchy duplicate exists

#### Scenario: Drilling into Crunchy targets the same parameter from any bank
- **WHEN** the operator drills into Crunchy's modulation from any bank
- **THEN** the same single Crunchy parameter is the drill-in target
- **THEN** modulation applied from one bank's Crunchy cell affects the identical parameter reachable from every other bank

### Requirement: Scenes
The app SHALL support Sheaf scene state (scene selection and blend) through the Sheaf parameter model.

#### Scenario: Scene blend applies across banks
- **WHEN** the operator changes scene blend
- **THEN** parameter values interpolate between scene endpoints
- **THEN** the change is reflected on the rendered encoder rings

### Requirement: Defined initial patch
The app SHALL ship a defined initial patch: a small, enumerated set of parameters carries non-neutral starting values, and every other parameter starts at its ordinary default.

#### Scenario: Waveform Shape controls start at fixed points
- **WHEN** the app starts for the first time
- **THEN** the first oscillator's Shape control reads its minimum value
- **THEN** the second oscillator's Shape control reads its midpoint value
- **THEN** the third oscillator's Shape control reads its maximum value

#### Scenario: Cross-oscillator modulation depths are present at minimal depth
- **WHEN** the app starts for the first time
- **THEN** a defined set of cross-oscillator modulation depth assignments is already present
- **THEN** each of those assignments sits at the smallest non-zero depth available

#### Scenario: No other parameter departs from its ordinary default
- **WHEN** the app starts for the first time
- **THEN** every parameter outside the enumerated initial-patch set reads its ordinary default value
