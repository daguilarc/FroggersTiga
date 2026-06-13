## ADDED Requirements

### Requirement: Knob groups have visible module boundaries

Each host page SHALL render knob columns inside one or more bordered **knob-group** panels that reflect Field submodule structure. Group membership SHALL come from a single `HOST_PAGE_GROUPS` table in `main.ts`, applied in one loop on page change.

#### Scenario: Audio page groups

- **WHEN** the user views the Audio page
- **THEN** knobs 0–2 appear inside a **VCOs** group panel
- **AND** knobs 3–5 appear inside a **Coupling** group panel
- **AND** knobs 6–7 appear inside an **Output** group panel
- **AND** each group has a visible border and group title

#### Scenario: Each column is a bordered cell inside the group

- **WHEN** the user views any host page with knob groups
- **THEN** each `.knob-col` inside a `.knob-group` has a visible border/panel enclosing its title label, rotary knob, and mod-source control
- **AND** the three controls read as one vertical column box, not floating on the page background

#### Scenario: Page change updates groups

- **WHEN** the user navigates to the Filter page before or after Play
- **THEN** group panels and titles update to match Filter row groupings
- **AND** knob labels inside groups remain correct

### Requirement: Group table is single source

Group titles and row indices SHALL be defined once per page in `HOST_PAGE_GROUPS`, not duplicated in HTML or per-page CSS blocks.

#### Scenario: Filter page includes Comb offset group

- **WHEN** verifying the Audio/Filter pages
- **THEN** Filter row 0 (**Comb offset**) sits in the **Pre-comb** group for that page
- **AND** group definitions are grep-verifiable in one table

#### Scenario: All six pages defined

- **WHEN** grepping `HOST_PAGE_GROUPS` in `main.ts`
- **THEN** all six host pages have group entries matching `design.md` D4
- **AND** every knob row 0–7 appears in exactly one group per page
