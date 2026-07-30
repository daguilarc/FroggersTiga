## ADDED Requirements

### Requirement: Mobile knob grid uses three columns

On viewports ≤720px wide, the `.knobs` container SHALL lay out knob cells in exactly three grid columns.

#### Scenario: Three-column grid at phone width

- **WHEN** the viewport width is 390px and the sim field is visible
- **THEN** computed `grid-template-columns` on `.knobs` resolves to three tracks (not four)

#### Scenario: Four-column grid on desktop

- **WHEN** the viewport width is ≥1280px
- **THEN** computed `grid-template-columns` on `.knobs` resolves to four tracks

### Requirement: Knob labels visible on every page without Play

Each `.knob-label-main` on the active host page SHALL display the parameter name from the static page table (`paramDisplayNames.ts` / `ParamDisplayNames.hpp` parity) immediately on load and on every UI page change, without requiring Play or WASM screen updates.

#### Scenario: Labels on initial load

- **WHEN** mobile emulation is active and the sim loads on the Audio page
- **THEN** labels include `VCO1`, `Cross-coupler`, and `Att. 1+2` with visible bounding boxes

#### Scenario: Labels on page pill navigation

- **WHEN** the user selects a different page pill
- **THEN** knob labels update to that page's static names (e.g. Drive page shows `Drive`, not Audio names)

#### Scenario: WASM override when engine synced

- **WHEN** audio is playing and WASM sends screen rows for the current host page
- **THEN** knob labels reflect WASM row names (matching static table when in sync)

#### Scenario: UI page change while playing

- **WHEN** audio is playing and the user selects a different page pill
- **THEN** labels immediately show the new page's static names (not stale rows from the previous page)

### Requirement: Mobile parameter labels remain readable

On viewports ≤720px wide, each `.knob-label-main` element SHALL display its parameter name text without collapsing to zero visible height.

#### Scenario: Representative labels have visible layout boxes

- **WHEN** mobile emulation is active on the Audio page
- **THEN** labels for `VCO1` and `Cross-coupler` each have a bounding box height greater than zero

### Requirement: Playwright regression for mobile knob labels

The web Playwright suite SHALL include tests under mobile emulation that assert the three-column grid and label visibility on load and page navigation without starting audio.

#### Scenario: CI runs mobile label tests

- **WHEN** `npm run test:e2e` executes in CI
- **THEN** the mobile knob label spec passes alongside existing e2e tests
