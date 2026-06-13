## MODIFIED Requirements

### Requirement: Depth deferred to Manual

Quick Dict SHALL include a single upfront note that the in-app **Manual** is the full sim operator guide. Daisy Field hardware detail (OLED symbols, flash procedure, pickup workflow) SHALL be referenced via repository `MANUAL.md`, not the in-app Manual.

#### Scenario: User needs decay time explanation

- **WHEN** a user needs to know what decay does in depth
- **THEN** Quick Dict shows only `Decay : …` with a short gloss
- **AND** the document directs the user to in-app **Manual** for full sim behavior

#### Scenario: User needs Field hardware detail

- **WHEN** a user needs pickup badges, M1–M7 assignment, or full Field signal-flow detail
- **THEN** Quick Dict directs them to repository `MANUAL.md`
- **AND** does not imply the in-app Manual documents Field hardware
