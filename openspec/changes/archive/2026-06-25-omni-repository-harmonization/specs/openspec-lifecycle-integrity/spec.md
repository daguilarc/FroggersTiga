## ADDED Requirements

### Requirement: Active artifacts describe one implementable contract
Each in-scope desktop, web/WASM, VCV, VST/AU, sim, or shared-core change's proposal, delta specs, design, and tasks SHALL agree on required behavior, deferred behavior, and verification ownership. Removed/deferred work SHALL be removed from normative delta specs and SHALL NOT remain as unchecked struck-through tasks.

#### Scenario: Deferred device automation
- **WHEN** real-device automation is deferred in tasks
- **THEN** the active proposal/design/specs describe manual device verification as the current contract and track automation as a future change

#### Scenario: Mobile grid contract
- **WHEN** the mobile knob-label change is inspected
- **THEN** proposal, design, spec, tasks, CSS, and Playwright tests all specify three columns

#### Scenario: VCV VCO-AR controls
- **WHEN** the VCO-AR left-expander change promises Randmod and per-parameter modulation
- **THEN** its proposal, specs, design, and tasks consistently include six visible mod inputs and a resolved signed-envelope DSP formula

### Requirement: Baseline specs have meaningful purposes
Every in-scope archived host baseline spec SHALL replace generated `TBD` purpose text with a concise statement of the capability's domain and boundary.

#### Scenario: Baseline inspection
- **WHEN** the OpenSpec hygiene check scans `openspec/specs/*/spec.md`
- **THEN** no placeholder purpose remains

### Requirement: Canonical OpenSpec artifacts are version-controlled source
The repository SHALL track OpenSpec configuration, baseline specs, active change artifacts, per-change schema metadata, and archive history governing the in-scope host surfaces. `.gitignore` SHALL NOT exclude the entire planning home; only named ephemeral cache/session outputs MAY be ignored. This requirement does not authorize semantic edits to firmware-only planning artifacts.

#### Scenario: Clean clone
- **WHEN** a collaborator clones the repository at the same commit
- **THEN** OpenSpec lists the same baseline specs, active changes, tasks, and archive history

#### Scenario: Planning artifact changes
- **WHEN** a proposal, design, delta spec, task list, or archive result changes
- **THEN** Git reports a reviewable source diff

### Requirement: Non-omni active plans are closed with truthful archive semantics
Every in-scope active change other than `omni-repository-harmonization` SHALL be archived during this change. Code-backed deltas SHALL be reconciled with source and merged into baseline before normal archive. Superseded, abandoned, or contradictory plans SHALL be archived with `--skip-specs` and a supersession note so they do not mutate baseline truth. Archival SHALL NOT mark unrun manual verification as passed.

#### Scenario: Fully checked change
- **WHEN** every required task is complete and validation passes
- **THEN** the change is archived and removed from the active change list

#### Scenario: Stale plan has manual gates pending
- **WHEN** an obsolete plan has unchecked manual verification but its work is no longer intended
- **THEN** the archive records the gate as unrun and closes the plan without claiming verification

#### Scenario: Checked tasks contradict implementation
- **WHEN** every checkbox is marked complete but a normative delta still describes absent behavior
- **THEN** the unsupported delta is corrected before normal archive or the plan is archived with `--skip-specs`

#### Scenario: Closure sweep completes
- **WHEN** all planned archive operations finish
- **THEN** `openspec list` contains only `omni-repository-harmonization`

### Requirement: Active capability ownership is unambiguous
Two in-scope active changes SHALL NOT independently own the same host capability requirements without an explicit handoff or supersession recorded in both changes. A handoff SHALL identify which change implements, verifies, and archives each remaining requirement.

#### Scenario: Duplicate active capability
- **WHEN** the hygiene check finds the same capability delta under two active changes
- **THEN** it fails unless both artifacts identify a consistent temporary handoff and one visible task resolves it

#### Scenario: Pair-AR blend follow-up
- **WHEN** pair-AR is added to the shared modulation contract
- **THEN** `mod-blend-semantics-docs` is completed/archived first and this change extends the resulting `mod-blend-semantics` baseline rather than creating a second formula authority

### Requirement: OpenSpec hygiene is locally verifiable
The repository SHALL provide a local host-scoped OpenSpec hygiene check that validates the in-scope active changes/specs, rejects placeholder purposes, removed-task markup, unresolved duplicate ownership, and any non-omni active change after the closure sweep. Firmware-only artifacts SHALL be left unmodified and outside semantic pass/fail results.

#### Scenario: Clean OpenSpec tree
- **WHEN** the hygiene command runs after harmonization
- **THEN** all in-scope items validate, no placeholders, removed-task checkboxes, unresolved duplicate owners, or non-omni active changes are found
