## ADDED Requirements

### Requirement: Web page chrome

Each host page SHALL display a chrome block above the knob area with: page name, host index `(n/6)`, a one-line page description, and **Randomize** plus **Randomize mod** buttons scoped to the visible page only.

#### Scenario: Delay page chrome

- **WHEN** host page index is 5
- **THEN** chrome title reads **Delay (6/6)**
- **AND** **Randomize** affects Delay knobs 0–6 only
- **AND** **Randomize mod** affects Delay mod sources and depths only

#### Scenario: Filter page chrome

- **WHEN** host page index is 3
- **THEN** **Randomize mod** randomizes Filter page mod assignments only via WASM

#### Scenario: Core page Randomize

- **WHEN** host page index is 0–4 and the user clicks page **Randomize**
- **THEN** WASM randomizes that core page's knob parameters only

### Requirement: Delay parameter hints on web

On host page 5, the web UI SHALL show a short hint for each Delay row explaining its role (time, send, feedback, width, tone, mod rate, mix, fuegoizer).

#### Scenario: DTIM hint visible

- **WHEN** the user navigates to Delay on a 375 px wide viewport
- **THEN** **DTIM** is labeled with a hint indicating delay time
- **AND** layout does not require horizontal scroll of knobs

### Requirement: Bottom page navigator

The web UI SHALL provide a row of six page pills with touch targets at least 44×44 CSS pixels. Tapping a pill SHALL navigate to that host page using the same path as prev/next arrows.

#### Scenario: Mobile pill navigation

- **WHEN** viewport width is at most 720 px and the user taps the **Delay** pill
- **THEN** host page index becomes 5
- **AND** prev/next arrows remain visible beside knobs

#### Scenario: Active pill indicator

- **WHEN** host page index is 2
- **THEN** the **Reverb** pill is visually highlighted
