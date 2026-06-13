## MODIFIED Requirements

### Requirement: Page chrome shows current page context

Each host page SHALL display a chrome block with: page name, one-line role blurb, **Randomize** (knobs 1–7 on current page), and **Randomize mod** (mod routes on current page). Delay page SHALL use the same chrome pattern with Delay-scoped randomize actions.

#### Scenario: Audio page chrome

- **WHEN** the user navigates to the Audio page
- **THEN** the chrome title reads **Audio** (or **Audio (1/6)**)
- **AND** **Randomize** and **Randomize mod** buttons are visible and scoped to Audio only

#### Scenario: Delay page chrome

- **WHEN** the user navigates to the Delay page
- **THEN** the chrome shows Delay-specific blurb and scoped randomize actions

## ADDED Requirements

### Requirement: Mod route summary follows page chrome

The mod route summary block SHALL appear in the DOM **immediately after** the page chrome section and **before** the knob field layout, so users read page context before per-route lines.

#### Scenario: Reading order

- **WHEN** the user scrolls the sim top to bottom on mobile
- **THEN** page chrome (title + blurb + randomize) appears before the mod route summary list
