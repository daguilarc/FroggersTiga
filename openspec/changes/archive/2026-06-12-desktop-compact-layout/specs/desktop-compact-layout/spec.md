## ADDED Requirements

### Requirement: Default window fits common single-monitor widths

The desktop application SHALL open at **1680×720** pixels by default. Six submodule panels SHALL divide the content area equally (`width / 6`).

#### Scenario: Launch on 1920×1080 display

- **WHEN** the user launches the desktop app on a 1920×1080 display at 100% scale
- **THEN** the full window width fits within the screen without horizontal clipping

#### Scenario: Panel width at default size

- **WHEN** the app opens at default size
- **THEN** each of the six panels receives approximately 280 px width (± margins)

### Requirement: Per-panel randomize buttons use compact labels

Each `SubModulePanel` SHALL display **Randomize** and **Randmod** (not “Randomize mod”). Buttons SHALL be laid out at intrinsic text width left-aligned in the button row, not an equal 50/50 split.

#### Scenario: Randmod label

- **WHEN** the user views any submodule panel header
- **THEN** the mod-randomize button text is **Randmod**

#### Scenario: Randomize button padding

- **WHEN** the user views the Randomize button beside Randmod
- **THEN** the button width is no wider than text fit plus minimal padding (≤ 6 px total horizontal inset beyond LookAndFeel text width)

### Requirement: Global strip uses shortened labels

The global strip SHALL label controls **Rand all**, **Randmod all**, **Rand waves**, and **Marbles**.

#### Scenario: Strip text

- **WHEN** the user views the bottom global strip
- **THEN** no strip button reads “Randomize mod (all)” or “Randomize VCO Waveform”
