## MODIFIED Requirements

### Requirement: Performance band readable at default width

Performance band controls at 1280px standalone width SHALL display scene button labels, blend endpoint labels, and marbles (S&H) labels without ellipsis truncation.

#### Scenario: Marbles labels visible

- **WHEN** performance band lays out at default width
- **THEN** marbles label components have height at least `kPerfMarblesLabelH` (2u)
- **THEN** "S&H 1" and "S&H 2" text is fully visible

#### Scenario: Scene buttons show ordinal suffix

- **WHEN** left/right scene ordinals are active
- **THEN** scene button labels (e.g. S1·L) fit within button bounds without `...`
