## ADDED Requirements

### Requirement: Marbles mod sources use LED indicator

Marbles 1 and Marbles 2 mod sources SHALL display a green on/off LED instead of an oscilloscope trace on web and desktop. LED on state SHALL be `CV level > 0.55`; off state otherwise. VCO Envelope SHALL retain a continuous scope trace.

#### Scenario: Marbles LED on web while playing

- **WHEN** audio is running and Marbles 1 CV level exceeds the on threshold
- **THEN** the Marbles 1 mod bay indicator shows green (on state)
- **AND** no oscilloscope canvas is drawn for Marbles 1

#### Scenario: Marbles LED off web

- **WHEN** Marbles 2 CV level is below the on threshold
- **THEN** the Marbles 2 indicator shows dim/off state

#### Scenario: Marbles LED on desktop mod rack

- **WHEN** the user views the desktop mod rack while playing
- **THEN** Marbles 1 and Marbles 2 modules show LED indicators instead of scope traces
- **AND** VCO Envelope module still shows a continuous CV trace

#### Scenario: Idle mod bay

- **WHEN** audio is not running
- **THEN** Marbles LEDs show off/dim state
- **AND** VCO Envelope scope shows idle styling

### Requirement: Marbles mod sources labeled S&H

Every user-visible label for Marbles mod outputs (mod indices 5 and 6) SHALL include **S&H** to indicate sample-and-hold behavior.

#### Scenario: Web mod bay labels

- **WHEN** the user views the mod sources panel
- **THEN** the Marbles mod source labels read **Marbles 1 S&H** and **Marbles 2 S&H**

#### Scenario: Web mod dropdown labels

- **WHEN** the user opens a knob mod source dropdown on any page
- **THEN** Marbles options read **Marbles 1 S&H** and **Marbles 2 S&H**

#### Scenario: Desktop mod rack labels

- **WHEN** the user views the desktop mod rack
- **THEN** Marbles module titles read **Marbles 1 S&H** and **Marbles 2 S&H**
