# desktop-midi-cc-display Specification

## Purpose
Ensure desktop standalone mod-rack MIDI CC labels fit their module boxes without clipping at default window sizes.
## Requirements
### Requirement: CC values display without truncation

The desktop MIDI Settings dialog SHALL display the full numeric value (0–127) for both **In CC** and **Out CC** controls without ellipsis or clipping at the default dialog size (480×420 pixels).

#### Scenario: Two-digit In CC value visible

- **WHEN** the user opens MIDI Settings and sets In CC to **74**
- **THEN** the In CC control shows **74** (not `…` or a partial digit)

#### Scenario: Three-digit In CC value visible

- **WHEN** the user sets In CC to **127**
- **THEN** the In CC control shows **127** without truncation

#### Scenario: Out CC matches In CC display rules

- **WHEN** the user sets Out CC to **10**, **74**, or **127**
- **THEN** each value is fully visible in the Out CC control

#### Scenario: CC edit persists to engine state

- **WHEN** the user changes In CC to **42** and closes the dialog
- **THEN** `CvMidiBridge.m_inCc` equals **42** and subsequent MIDI CC events on that CC number affect the mod rack

