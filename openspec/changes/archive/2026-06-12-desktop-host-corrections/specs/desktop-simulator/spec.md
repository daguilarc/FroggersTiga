## MODIFIED Requirements

### Requirement: External ring-mod input

The desktop app SHALL route audio input from the configured input device to `ProcessBlock` whenever Play is active. External ring-mod vs VCO-only mixing SHALL be decided **only** by the engine envelope gate (`m_extGate`) and `OLVL`, not by a host **Off** switch.

#### Scenario: Input device configured

- **WHEN** the user configures an input device in Audio Settings and clicks Play
- **THEN** `ProcessBlock` receives input samples from that device

#### Scenario: Silent input

- **WHEN** input is disconnected or below gate threshold
- **THEN** output follows VCO-only path per MANUAL
- **AND** no host Off toggle is required

## REMOVED Requirements

### Requirement: External Off host gating

**Reason**: Superseded by engine Schmidt gate; was web Mic model mispapplied to desktop.

**Migration**: Remove `External: Off | L | R` and `ExternalInputMode` from desktop; keep web External/Mic toggle.
