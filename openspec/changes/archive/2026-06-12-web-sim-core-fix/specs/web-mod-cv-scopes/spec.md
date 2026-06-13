## ADDED Requirements

### Requirement: Mod bay uses rectangular CV scopes

The mod bay SHALL display three rectangular CV trace areas for **VCO level**, **Marbles 1**, and **Marbles 2**. Each trace area SHALL be at least **40 px tall** and wider than tall (target ~3:1). The mod bay SHALL NOT use narrow horizontal level bars as the primary visualization.

#### Scenario: Layout

- **WHEN** the mod bay is expanded
- **THEN** three scope panels appear in a row (stacked on narrow viewports)
- **AND** each panel includes a label and a canvas or SVG trace rectangle

### Requirement: VCO level continuous trace

While audio is playing, the VCO level scope SHALL scroll a continuous CV trace driven by mod index 4 levels from the engine.

#### Scenario: Playing VCO modulation

- **WHEN** audio is playing and VCO level CV changes
- **THEN** the VCO level scope trace moves over time

### Requirement: Marbles scopes show step-and-hold

Marbles 1 and Marbles 2 scopes SHALL use step-hold rendering: level fill at held CV, visible step edges when Marbles advances, matching desktop `CvTraceMode::StepHold` intent.

#### Scenario: Held CV while playing

- **WHEN** audio is playing and Marbles CV holds constant
- **THEN** the Marbles scope shows a horizontal level indication at that voltage (not a blank trace)

#### Scenario: Marbles button step

- **WHEN** the user clicks **Marbles** during Play and CV steps
- **THEN** the Marbles scope shows a step edge within one UI refresh cycle

### Requirement: Idle scopes retain last level

When audio stops, mod scopes SHALL enter an idle (dimmed) state that retains the last known CV level rather than resetting to empty or a fixed midline.

#### Scenario: Stop after Play

- **WHEN** the user clicks **Stop** after Marbles held a CV level
- **THEN** Marbles scopes remain visible at the last level with reduced opacity
