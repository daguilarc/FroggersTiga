## ADDED Requirements

### Requirement: Quick Dict uses consistent entry format
Every Quick Dict entry SHALL use the pattern `Label — gloss` (em dash separator). The legacy `Label : gloss` colon form MUST NOT appear in updated files.

#### Scenario: Operator scans Transport section
- **WHEN** the operator reads transport entries in Quick Dict
- **THEN** each line uses the same separator and comparable sentence length

### Requirement: Transport labels match on-screen manual spelling
Quick Dict transport entries SHALL use the same spellings as the sim UI and manual: **Rand mod** (two words), **Rand Mods**, **Rand All**, **Rand waveforms**.

#### Scenario: Operator cross-references Rand mod button
- **WHEN** the operator searches Quick Dict for the page-level random-mod button
- **THEN** the entry is titled **Rand mod**, not `Randmod`

### Requirement: Crispy appears once in Quick Dict
Quick Dict SHALL contain one **Crispy** entry under a **Global** or **Every page** heading. Per-page sections MUST omit repeated Crispy lines.

#### Scenario: Operator looks up Reverb Crispy
- **WHEN** the operator opens the Reverb section in Quick Dict
- **THEN** Crispy is not listed again; the Global entry covers all pages

### Requirement: Mod sources section includes CC enable summary
The Sim mod sources section SHALL document MIDI CC 1 and MIDI CC 2 with default channel/CC and a single line on disable behavior (grey mod column, routes cleared).

#### Scenario: Operator checks CC 2 in Quick Dict
- **WHEN** the operator reads the MIDI CC 2 entry
- **THEN** they see default routing (ch 1, CC 2) and that disabling clears assignments, matching the manual Mod bay section

### Requirement: Section order mirrors manual teach flow
Quick Dict sections SHALL appear in this order: Mod sources → Transport → Global (Crispy) → Audio → Random → Reverb → Filter → Drive → Delay → Field-only pointer.

#### Scenario: Operator opens Quick Dict from help menu
- **WHEN** the operator scrolls top to bottom
- **THEN** section order matches the manual's conceptual priority (mod + transport before page knobs)

### Requirement: Quick Dict mirrors sync to public and docs copies
`QUICK_DICT.md` at the repository root SHALL be canonical. `docs/quick-dict.md` and `web/public/quick-dict.md` MUST stay byte-identical to it after each update.

#### Scenario: CI or sync script runs after dict edit
- **WHEN** `QUICK_DICT.md` changes on a branch
- **THEN** a check fails if mirror copies differ from canonical
