## ADDED Requirements

### Requirement: v2-operator-center-cluster-docs

Operator documentation SHALL describe the center global cluster and updated Write Seq. workflow.

#### Scenario: Center cluster documented

- **WHEN** reader opens desktop v2 layout documentation
- **THEN** Rand All / Crunchy / Shift are documented in the module center column, not the bottom strip

#### Scenario: Write Seq. playing workflow documented

- **WHEN** reader opens Write Seq. documentation
- **THEN** text explains: arm Write Seq., Start Sequence, edit highlight follows playhead while playing, each beat saves the step being left, first beat after start does not duplicate step-0 capture, stopped navigation saves on step change
