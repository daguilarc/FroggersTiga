## ADDED Requirements

### Requirement: Knob labels visible before Play

The web sim SHALL display all eight knob column labels for the current page immediately on first paint, without waiting for WASM bootstrap or a `screen` message.

#### Scenario: First paint on Audio page

- **WHEN** the sim page loads on the Audio page
- **THEN** knob columns show **VCO1**, **VCO2**, **VCO3**, **Cross-coupler**, **Phase mod 1**, **Phase mod 2**, **VCO level**, **Crunch**
- **AND** no column shows `—` or an empty label

#### Scenario: Page change updates labels

- **WHEN** the user navigates to the Filter page before clicking Play
- **THEN** row 0 label reads **Comb offset**
- **AND** labels match `ParamDisplayNames.hpp` for that page

#### Scenario: WASM screen updates values not primary labels

- **WHEN** a `screen` message arrives after Play
- **THEN** knob values and OLED refresh from WASM
- **AND** base column labels remain sim display names unless mod routing switches the column to **Mod depth**

### Requirement: Single label table in main thread

Display names for web knob labels SHALL come from one `HOST_PAGE_LABELS` constant in `web/src/main.ts` that mirrors `sim/ParamDisplayNames.hpp`. Labels SHALL be applied via one loop over the eight columns on page change, not duplicated per-page blocks. WASM `screen` updates SHALL overlay **Mod depth** only when mod routing is active; static names remain the base layer.

#### Scenario: Grep parity with header

- **WHEN** verifying the implementation
- **THEN** every string in `HOST_PAGE_LABELS` matches the corresponding entry in `ParamDisplayNames.hpp`

### Requirement: Local page navigation before WASM bootstrap

Page navigation controls (pills, prev/next, swipe, keyboard) SHALL update local `hostPage`, page chrome, and static knob labels without requiring a worklet or WASM `screen` message. WASM page messages SHALL be sent only when a worklet exists.

#### Scenario: Pill tap before Play

- **WHEN** the user taps the **Filter** pill before clicking Play
- **THEN** page chrome reads **Filter (4/6)**
- **AND** knob row 0 label reads **Comb offset**
- **AND** no WASM fetch or `addModule` runs
