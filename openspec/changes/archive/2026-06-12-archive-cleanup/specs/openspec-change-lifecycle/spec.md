## ADDED Requirements

### Requirement: Two active implementation changes after cleanup

After `archive-cleanup` apply completes, `openspec list` SHALL show exactly two non-archived changes with incomplete implementation work: `desktop-header-hit-test` and `web-chrome-cohesion`.

#### Scenario: List after cleanup

- **WHEN** `openspec list` runs after archive-cleanup apply
- **THEN** only `desktop-header-hit-test` and `web-chrome-cohesion` appear as in-progress implementation changes
- **AND** all other listed pre-cleanup changes are archived

### Requirement: Archive order preserves spec merge integrity

Archived changes SHALL be processed in the dependency order documented in `archive-cleanup/design.md` §2.

#### Scenario: Foundational before dependent

- **WHEN** archiving `desktop-host-mutation-safety`
- **THEN** `sim-hosts-multi-ui` and `desktop-host-corrections` are already archived

### Requirement: Chrome cohesion archived only after hit-test apply

`desktop-chrome-cohesion` SHALL NOT be archived until `desktop-header-hit-test` implementation is applied and verified.

#### Scenario: Hit-test before chrome archive

- **WHEN** `desktop-chrome-cohesion` is archived
- **THEN** `RecordExportCluster` uses union bounds (not full header)
- **AND** transport buttons receive clicks at cold launch

### Requirement: Open tails merged before parent archive

Manual verification and related tasks from changes slated for archive SHALL be copied into `MANUAL_VERIFY.md` or merged into an active change's `tasks.md` before `openspec archive` runs on the parent change.

#### Scenario: Web sim page UX verify

- **WHEN** `web-sim-page-ux` is archived
- **THEN** its §7 manual checks exist in `web-chrome-cohesion/tasks.md` or `MANUAL_VERIFY.md`

#### Scenario: Audio export verify

- **WHEN** `desktop-audio-export` is archived
- **THEN** export manual checks exist in `desktop-header-hit-test/tasks.md` or `MANUAL_VERIFY.md`

### Requirement: MIDI clarity spec superseded by pitch CV

On archive of `desktop-midi-input-clarity`, velocity-only QWERTY mod semantics SHALL be marked superseded by `desktop-qwerty-midi-pitch-cv` pitch × velocity formula.

#### Scenario: Archive midi-input-clarity

- **WHEN** `desktop-midi-input-clarity` is archived
- **THEN** main specs do not require max-velocity-only `m_mods[0]` from QWERTY keys
- **AND** pitch step formula from `desktop-qwerty-midi-pitch-cv` is the canonical requirement
