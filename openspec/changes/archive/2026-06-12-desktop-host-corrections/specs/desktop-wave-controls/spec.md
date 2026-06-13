## ADDED Requirements

### Requirement: Wave control shows painted waveform icon not ellipsis

On the Audio panel rows 0–2, the wave cycle control SHALL be a **drawn waveform icon** (sine, saw, or square mini path) reflecting the current morph band. It SHALL NOT use `juce::TextButton` default text or display `...`.

#### Scenario: First paint before timer

- **WHEN** the Audio panel is constructed and shown
- **THEN** each wave control immediately shows the correct icon for current morph
- **AND** no `...` placeholder is visible

#### Scenario: Morph band sine

- **WHEN** VCO morph is below 0.33
- **THEN** the icon depicts a sine-like curve

#### Scenario: Morph band saw

- **WHEN** VCO morph is between 0.33 and 0.66
- **THEN** the icon depicts a sawtooth shape

#### Scenario: Morph band square

- **WHEN** VCO morph is 0.66 or above
- **THEN** the icon depicts a square wave shape

#### Scenario: After cycle click

- **WHEN** the user clicks the VCO2 wave control
- **THEN** the icon updates to match the new morph band on the next UI refresh

### Requirement: Wave control is large enough to read

The wave control SHALL be at least **28×28 px** hit target with at least **22×14 px** drawable area for the icon path.

#### Scenario: Six-panel layout

- **WHEN** the window is at default size with six columns (**1680×720** per `desktop-compact-layout`; was 2016×720)
- **THEN** the waveform icon is visually distinct from label text and not clipped to ellipsis

### Requirement: Desktop audio rows use VCO1 VCO2 VCO3 display names

The desktop Audio panel SHALL display **VCO1**, **VCO2**, **VCO3** as row labels (display alias). Core `Parameter` OLED names (`V1VO` etc.) remain unchanged for firmware parity.

#### Scenario: Label at six-panel width

- **WHEN** the window is at default 2016×720
- **THEN** VCO1, VCO2, VCO3 are fully visible beside wave controls

### Requirement: Wave control purpose is identifiable

Wave controls SHALL expose tooltip or accessible name: **Cycle waveform: sine → saw → square**.

#### Scenario: Discoverability

- **WHEN** the user hovers the wave control
- **THEN** they understand it changes oscillator waveform, not modulation routing
