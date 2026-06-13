## ADDED Requirements

### Requirement: Per-page knob routing

`DesktopHostIO` SHALL provide `SetPageKnob(uint8_t page, uint8_t position, float value)` that calls `PageManager::KnobUpdateOnPage` without changing `m_currentPage`.

#### Scenario: Simultaneous pages

- **WHEN** Audio knob 0 is 0.3 and Drive knob 0 is 0.9 at the same time
- **THEN** both values are stored on their respective pages

### Requirement: No page switching in desktop v1

`DesktopHostIO` SHALL NOT expose SW1/SW2 page navigation in the desktop UI or require `PageNext`/`PagePrevious` for normal editing.

#### Scenario: All pages editable

- **WHEN** the desktop app is running
- **THEN** all five page panels accept knob input without switching current page

### Requirement: Direct knob writes

`DesktopHostIO::Init()` SHALL set every parameter on every page to `TrackingState::Tracking`. Desktop knob updates SHALL apply via `KnobUpdateOnPage` without hardware pickup gestures.

#### Scenario: Slider move updates DSP

- **WHEN** the user moves Filter panel knob 4
- **THEN** filter DSP reads the new value on the next `ReadParamsBlock`

### Requirement: Per-panel and global randomize

Each sub-module panel SHALL expose **B1** calling `PageManager::RandomizePage(page)` (knobs 1–7 / params 0–6 only; FUEG skipped by `Parameter::Randomize`). The shared strip SHALL expose **B2** calling `RandomizeAllPagesIndependent()` and **B4** calling `RandomizeAllPagesMod()`. **B3** (randomize mod on one page) SHALL NOT appear in desktop v1.

#### Scenario: Per-panel B1

- **WHEN** the user presses B1 on the Drive panel
- **THEN** `RandomizePage(drivePage)` runs and only Drive page knobs 1–7 are randomized

#### Scenario: Global B2

- **WHEN** the user presses B2 on the shared strip
- **THEN** `PageManager::RandomizeAllPagesIndependent()` runs

#### Scenario: B3 absent in v1

- **WHEN** inspecting the desktop v1 shared strip
- **THEN** no B3 control is present

### Requirement: Shared global buttons

`DesktopHostIO` SHALL route shared strip actions: B5 → Marbles increment; wave buttons → `CycleVcoMorph` per VCO; B2/B4 → global randomize per MANUAL semantics. B6/B7 XCPL randomize is **not** on the desktop strip — use the XCPL knob on the Audio panel (Field hardware only).

#### Scenario: B5 increments Marbles

- **WHEN** the user presses B5 on the shared strip
- **THEN** `FroggersEngine::ButtonCallback(0)` runs once

### Requirement: Inline VCO morph controls on Audio panel

On the Audio sub-module, `DesktopHostIO` SHALL render a wave **icon** beside each `V1VO`/`V2VO`/`V3VO` label (rows 0–2) reflecting current morph. Each icon row SHALL include a compact morph knob (not a separate OLED row or full-width bar). Morph knobs SHALL be CV-modulatable via `ModMgr`.

#### Scenario: Icon reflects morph

- **WHEN** VCO2 morph is near saw
- **THEN** the icon beside `V2VO` shows the saw glyph state

#### Scenario: Morph knob updates DSP

- **WHEN** the user drags the VCO3 morph control to maximum
- **THEN** VCO3 output approaches square waveform

### Requirement: Sim-only VCO shortcut column

`DesktopHostIO` SHALL expose an extra two-button column (not on Daisy Field): A′ nudges VCO3 morph; B′ calls `RandomizeVcoMorphs()`. This column SHALL NOT be implemented in `DaisyIO`.

#### Scenario: Randomize all VCO morphs

- **WHEN** the user presses B′ on the shared strip
- **THEN** all three VCO morph values are randomized independently

### Requirement: Per-panel FUEG

Each desktop sub-module panel SHALL expose knob 8 as FUEG for that page when the page has fuegoization enabled.

#### Scenario: Filter FUEG independent

- **WHEN** Filter FUEG is 0.7 and Drive FUEG is 0.1
- **THEN** each page's parameters 0–6 fuegoize according to their own FUEG knob
