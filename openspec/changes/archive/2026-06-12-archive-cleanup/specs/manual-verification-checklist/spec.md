## ADDED Requirements

### Requirement: Consolidated manual verification document

The repository SHALL contain `MANUAL_VERIFY.md` at the repo root aggregating open manual verification steps from archived changes.

#### Scenario: File exists after cleanup

- **WHEN** `archive-cleanup` apply completes
- **THEN** `MANUAL_VERIFY.md` exists at the repository root
- **AND** it includes sections for stereo delay, patch/randomize, MIDI pitch CV, help menu, and web chrome

### Requirement: Checklist preserves source step IDs

Each manual step in `MANUAL_VERIFY.md` SHALL retain the source change name and step ID (e.g. `stereo-delay-page C.5`, `host-mutation-safety 6.1`) so failures are traceable.

#### Scenario: Traceable failure

- **WHEN** a manual step fails during sign-off
- **THEN** the step ID in `MANUAL_VERIFY.md` identifies the originating change and task number

### Requirement: README references manual verification

`README.md` SHALL link to `MANUAL_VERIFY.md` for human sign-off after automated builds pass.

#### Scenario: Developer onboarding

- **WHEN** a developer reads README for sim verification
- **THEN** they find a pointer to `MANUAL_VERIFY.md`
