## ADDED Requirements

### Requirement: Input peak meter beside Ext. In.

A level meter SHALL appear immediately right of the **Ext. In.** toggle. It SHALL display peak input level (0–1) from the last audio block when **Ext. In.** is on and Play is running. It SHALL NOT appear as an empty dead region when idle.

#### Scenario: Active signal visible

- **WHEN** **Ext. In.** is on, Play is running, and input channel 0 carries signal
- **THEN** the meter shows a blue fill proportional to peak level
- **AND** the fill updates at UI refresh rate (~15 Hz)

#### Scenario: Idle but not blank

- **WHEN** **Ext. In.** is off or Play is stopped
- **THEN** the meter shows a dim grey track with a minimal centre tick or equivalent idle chrome
- **AND** peak fill is not shown at full width falsely

#### Scenario: Low signal still visible

- **WHEN** peak level is above zero but below the Schmidt threshold
- **THEN** the meter still shows a non-zero fill width
- **AND** ring-mod mix may remain VCO-only per engine gate
