## ADDED Requirements

### Requirement: Manual opens with operator context, not implementation detail
The sim operator manual SHALL introduce itself as the guide for desktop, web, and plugin hosts. It MUST NOT lead with code identifiers (`ParamDisplayNames`, mod indices) in the opening paragraphs.

#### Scenario: Reader opens manual from web help modal
- **WHEN** the operator opens the Manual from the in-app help menu
- **THEN** the first screen explains what the sim is, which hosts share the guide, and that on-screen knob names match the manual

### Requirement: Quick start follows play-first cadence
The Quick start section SHALL be five steps or fewer and MUST follow this order: wait for engine → Play → navigate pages → use knobs 1–7 / Crispy on 8 → optional mod assignment pointer.

#### Scenario: New user reads quick start only
- **WHEN** the operator reads Quick start before any other section
- **THEN** they can hear sound and switch pages without reading Mod bay or page tables

### Requirement: Mod concepts are defined once per topic
The manual SHALL contain exactly one authoritative subsection for each of: mod routing (web dropdown vs desktop patch cable), mod indicators (scope vs LED, 55% threshold), Random S&H behavior (step, slew, no internal clock), and MIDI CC enable/disable effects.

#### Scenario: Operator learns Random S&H
- **WHEN** the operator reads about Random on Page 2 — Random
- **THEN** they are referred to the single Mod indicators / Random S&H subsection for LED and stepping rules instead of repeating those paragraphs inline

#### Scenario: Operator learns CC gating
- **WHEN** the operator reads about MIDI CC 1 or MIDI CC 2
- **THEN** enable/disable behavior (grey column, blocked routes, cleared assignments, QWERTY respects CC 1 flag) appears once in the Mod bay section, not split across a broken bullet list

### Requirement: Page reference tables include inline gloss
Each page section (Audio through Delay) SHALL present a table with columns **Row**, **Parameter**, and **What it does**. Gloss text MUST match semantic content already in `QUICK_DICT.md` without copying six identical Crispy lines.

#### Scenario: Operator looks up Filter row 5
- **WHEN** the operator opens Page 4 — Filter
- **THEN** row 5 shows **Comb delay** with a one-line gloss (comb pitch) in the same table, without requiring Quick Dict

### Requirement: Crispy is defined globally once
The manual SHALL define **Crispy** once (scramble knobs 1–7; external-input topology on sim) and MUST reference that definition from each page section instead of restating full Crispy prose per page.

#### Scenario: Operator reads Delay page
- **WHEN** the operator reaches the Crispy row on Page 6 — Delay
- **THEN** the gloss reads as a short back-reference (e.g., "Crispy — see Global controls") or a ≤8-word reminder, not a duplicated paragraph

### Requirement: Host differences appear after shared concepts
The Desktop vs web vs plugin section SHALL come after Mod bay and page reference. Host-specific bullets MUST only cover differences (layout, permissions, MIDI settings visibility), not re-teach shared knob semantics.

#### Scenario: Web user reads External MIDI
- **WHEN** the operator reads the Web subsection
- **THEN** External MIDI permission timing, CC 1/CC 2 toggle behavior, and default-off state are documented without repeating the full mod-source catalog

### Requirement: Canonical manual stays synced across mirrors
`SIM_MANUAL.md` at the repository root SHALL be the single authoring source. `docs/sim-manual.md` and `web/public/sim-manual.md` MUST be byte-identical copies after each update.

#### Scenario: CI or sync script runs after doc edit
- **WHEN** `SIM_MANUAL.md` changes on a branch
- **THEN** a check fails if `docs/sim-manual.md` or `web/public/sim-manual.md` differ from the canonical file

### Requirement: Operator prose excludes changelog negatives
User-facing manual sections (everything before Version history) SHALL describe current behavior only. They MUST NOT include migration phrasing such as "no longer controls PM3 on sim hosts."

#### Scenario: Operator reads Audio page notes
- **WHEN** the operator reads Phase mod 3 and Crispy notes on the Audio page
- **THEN** the text states what each control does now, with no reference to retired firmware mappings
