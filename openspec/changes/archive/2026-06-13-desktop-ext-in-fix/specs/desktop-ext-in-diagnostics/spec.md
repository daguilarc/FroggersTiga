## ADDED Requirements

### Requirement: Input peak meter beside Ext. In.

A level meter SHALL appear immediately right of the **Ext. In.** toggle. It SHALL display peak input level (0–1) from the last audio block when **Ext. In.** is on and Play is running.

#### Scenario: Signal present

- **WHEN** **Ext. In.** is on, Play is running, and input channel 0 carries signal
- **THEN** the meter fill width reflects peak level
- **AND** the meter is not an empty dead region

#### Scenario: Idle

- **WHEN** **Ext. In.** is off or Play is stopped
- **THEN** the meter shows grey idle track with centre tick

### Requirement: Routing failure status

When **Ext. In.** is on and Play is running but input is not reaching the engine, the desktop UI SHALL show a short diagnostic message in a transport-area label (`m_routeHint`) beside the input meter.

#### Scenario: No active input channels

- **WHEN** **Ext. In.** is on, Play is running, and `numInputChannels == 0`
- **THEN** `m_routeHint` indicates no input channels are active
- **AND** instructs the user to check Audio Settings

#### Scenario: Silent capture

- **WHEN** **Ext. In.** is on, Play is running, input channels are active, but peak level stays near zero for at least one second
- **THEN** `m_routeHint` indicates input is silent
- **AND** mentions macOS Privacy settings and/or line level / cable check

#### Scenario: Routing OK

- **WHEN** **Ext. In.** is on, Play is running, and peak level exceeds the silence threshold
- **THEN** `m_routeHint` is empty or hidden
- **AND** the meter reflects peak level
