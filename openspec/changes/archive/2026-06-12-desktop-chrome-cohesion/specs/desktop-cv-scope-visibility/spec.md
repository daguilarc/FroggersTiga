## ADDED Requirements

### Requirement: Marbles scopes show held CV level

When Marbles CV is holding a constant value during Play, the scope SHALL render a visible **level fill** and/or trace at that voltage, not only a blank or center idle line.

#### Scenario: Hold after Marbles step

- **WHEN** audio is playing and the user presses **Marbles** then waits without further steps
- **THEN** the Marbles 1 and/or Marbles 2 scope shows a horizontal indication at the held CV level
- **AND** the indication is distinguishable from the idle midline grid

#### Scenario: Step edge visible

- **WHEN** Marbles CV steps to a new value during Play
- **THEN** the scope trace shows a vertical or step edge between old and new levels within one UI refresh cycle

### Requirement: Idle scope reflects last CV when stopped

When audio stops, Marbles scopes MAY enter idle state but SHALL display the last known CV level (dimmed) rather than always drawing the 0.5 midline as if no signal existed.

#### Scenario: Stop after activity

- **WHEN** the user stops audio after Marbles produced non-zero CV
- **THEN** the scope idle state reflects the last CV level at reduced opacity

## MODIFIED Requirements

### Requirement: Mod rack CV scopes are wide trace displays

Each mod rack box (MIDI, VCO Envelope, Marbles 1, Marbles 2) SHALL show a **horizontal CV trace** (`CvScopeDisplay`), not a vertical level slider. The trace area SHALL be rectangular with height at least **40px**. Boxes SHALL be evenly spaced with visible gaps (**≥16px**). **Preferred box width SHALL be ~96px** (centered group); traces SHALL NOT expand to fill entire window width.

#### Scenario: Scope aspect ratio

- **WHEN** the user views the mod rack at default window size
- **THEN** each scope is wider than tall at the box width (≥2:1 at 96×44 px)
- **AND** scopes are evenly spaced with gaps between boxes

#### Scenario: Trace vs slider

- **WHEN** Marbles or MIDI mod CV changes over time during Play
- **THEN** the display shows a scrolling or step-hold trace line
- **AND** held CV shows a visible level indication, not an empty slider groove
