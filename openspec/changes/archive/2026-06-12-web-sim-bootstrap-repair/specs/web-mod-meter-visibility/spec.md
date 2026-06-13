## MODIFIED Requirements

### Requirement: Mod bay documents meter semantics

The mod bay area SHALL include exactly one helper text node stating scopes show **CV trace while playing**. The hint SHALL NOT be duplicated in both `index.html` and dynamically inside `#mod-bay`.

#### Scenario: Mod bay visible

- **WHEN** the mod bay is expanded
- **THEN** helper text **CV trace while playing** is visible once, adjacent to the mod bay toggle
- **AND** no second hint appears inside the scope grid
