## ADDED Requirements

### Requirement: Field docs are separate but mirrored copies are governed

Field operator documentation MAY remain separate from sim operator documentation. Any copied Field-adjacent quick-dict or public mirror content SHALL have a declared root authority and SHALL be freshness-checked or generated before publication.

#### Scenario: Field manual remains independent

- **WHEN** `MANUAL.md` changes for firmware-facing operator behavior
- **THEN** the change does not require sim UI docs to become identical
- **THEN** any shared quick-dict mirror text that copies the Field authority is updated or freshness checks fail

#### Scenario: Mirror drift is rejected

- **WHEN** a public mirror contains Field quick-dict text that differs from its declared authority
- **THEN** the docs parity check fails until the mirror is regenerated or the authority is updated
