## MODIFIED Requirements

### Requirement: Randomize parity

Per-panel and global randomize actions SHALL include Delay parameters with the same UX as core pages.

#### Scenario: Per-panel Randomize mod on Delay (web)

- **WHEN** the user clicks page **Randomize mod** in Delay page chrome
- **THEN** Delay mod sources and depths randomize with sim-valid indices only

#### Scenario: Web global randomize unchanged

- **WHEN** the user clicks global **Randomize mod (all)** from any host page
- **THEN** core pages 0–4 and Delay mod randomize both apply

### Requirement: Web mobile page arrows

The web simulator SHALL provide large previous/next controls flanking the knob area. Touch targets SHALL be at least 44×44 CSS pixels.

#### Scenario: Mobile layout with chrome and pills

- **WHEN** viewport width is at most 720 px
- **THEN** prev/next arrows, page chrome, and bottom pills coexist without horizontal knob scroll
