# desktop-v2-performance-band-chrome Specification

## Purpose
Performance band controls at the default standalone width remain readable without label truncation.

## Requirements
### Requirement: Performance band readable at default width
Performance band controls at 1280px standalone width SHALL display scene button labels, blend endpoint labels, and marbles S&H labels without ellipsis truncation.

#### Scenario: Marbles labels visible
- **WHEN** performance band lays out at default width
- **THEN** marbles label components have height at least `kPerfMarblesLabelH` equal to 2 grid units
- **THEN** "S&H 1" and "S&H 2" text is fully visible

#### Scenario: Scene buttons show ordinal suffix
- **WHEN** left or right scene ordinals are active
- **THEN** scene button labels such as `S1.L` fit within button bounds without ellipsis
