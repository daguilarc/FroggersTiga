## ADDED Requirements

### Requirement: Desktop host mutation queue contract

`DesktopHostIO` SHALL implement a single mutation queue drained in `tickControls()`. Burst mutations from desktop UI (randomize, mod assign, morph) SHALL use this queue.

#### Scenario: Drain before ProcessBlock

- **WHEN** an audio block begins
- **THEN** all pending mutations are applied before `FroggersEngine::ProcessBlock`

#### Scenario: Queue overflow coalescing

- **WHEN** multiple identical global randomize mutations are pending
- **THEN** drain applies at most one effective randomize of that type per block
