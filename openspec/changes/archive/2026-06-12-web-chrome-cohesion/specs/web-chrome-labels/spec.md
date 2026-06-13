## ADDED Requirements

### Requirement: Global strip uses Quick Dict labels

The web global strip SHALL label buttons **Rand All**, **Rand Mods**, **Rand waves**, and **Marbles** — matching `QUICK_DICT.md` transport section and desktop `GlobalStrip` post-`desktop-chrome-cohesion`.

#### Scenario: Global strip at mobile width

- **WHEN** the user views the global strip on a viewport ≤720 px
- **THEN** buttons read **Rand All**, **Rand Mods**, **Rand waves**, and **Marbles**
- **AND** labels match Quick Dict transport entries

### Requirement: Mod sources use VCO level naming

All web mod-source UI (mod bay cells, per-knob `<select>` options, route summary lines) SHALL display **VCO level** for mod index `4`, not “VCO Envelope”.

#### Scenario: Mod dropdown options

- **WHEN** the user opens a mod source select on any page
- **THEN** the option for index 4 reads **VCO level**

#### Scenario: Route summary line

- **WHEN** a knob on the current page is modulated by VCO level
- **THEN** the route summary shows `PARAM ← VCO level · N%`

### Requirement: Page chrome keeps per-page randomize labels

Per-page chrome buttons SHALL remain **Randomize** and **Randomize mod** (scoped to current page). This change SHALL NOT rename page chrome to Rand All / Rand Mods.

#### Scenario: Page chrome on Delay

- **WHEN** the user views the Delay page chrome
- **THEN** buttons read **Randomize** and **Randomize mod**
