## ADDED Requirements

### Requirement: Module columns follow unified surface without Random section
Desktop v2 module column / section layout SHALL derive geometry from shared layout helpers for the unified Application surface. Layout SHALL NOT reserve a module column or section for Random S&H page parameters.

#### Scenario: Layout helpers omit Random module section
- **WHEN** layout helpers compute module-section regions
- **THEN** no region is allocated for a Random S&H module page
- **THEN** Random S&H remains reachable only via mod-depth lanes
