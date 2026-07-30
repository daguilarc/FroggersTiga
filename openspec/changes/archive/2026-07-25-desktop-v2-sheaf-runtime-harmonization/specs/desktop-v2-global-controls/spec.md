## ADDED Requirements

### Requirement: Global Crunchy stable under sixteen-slot bank
Global Crunchy SHALL remain available as a stable global control under the sixteen-slot bank map and SHALL NOT require MIDI remapping when the active module section changes (MIDI mapping redesign itself is out of scope for this change).

#### Scenario: Section change keeps Crunchy identity
- **WHEN** the operator switches from Audio to Filter on the Application surface
- **THEN** Global Crunchy remains the same global control identity

### Requirement: Global rand chrome uses Froggers arm gesture
Global rand chrome SHALL implement toggle-global and held-next-click-local per `desktop-v2-rand-arm-gesture`.

#### Scenario: Global strip exposes toggle and hold affordances
- **WHEN** the Application surface shows global commands
- **THEN** rand toggle and hold interactions are available without Shift-based held-gesture semantics
