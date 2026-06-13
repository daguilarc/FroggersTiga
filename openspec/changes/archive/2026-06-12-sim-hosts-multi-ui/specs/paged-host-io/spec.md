## ADDED Requirements

### Requirement: Page switch contract

`PagedHostIO` SHALL map SW1 and SW2 rising edges to `PageManager::PagePrevious()` and `PageManager::PageNext()` only. It SHALL NOT duplicate `StopModTracking` logic.

#### Scenario: Page next during mod assign

- **WHEN** mod-assign is active and SW2 fires once
- **THEN** `m_modIndex` returns to 255 and `m_currentPage` increments with wrap

### Requirement: Shared eight-knob model

`PagedHostIO::SetKnob(i, v)` SHALL update `m_knobPositions[i]` and call `PageManager::KnobUpdate(i, v)` on the current page.

#### Scenario: Knob affects current page only

- **WHEN** the current page is Reverb and knob 0 is set to 0.8
- **THEN** `GetParam(reverbPage, 0)` reflects 0.8 and Audio page param 0 is unchanged

### Requirement: Screen query API

`PagedHostIO` SHALL expose row name, value, and tracking badge for the current page for OLED rendering.

#### Scenario: OLED row matches PageManager

- **WHEN** `GetRowName(0)` is called on Audio page
- **THEN** the returned string matches `PageManager::GetNameCurrentPage(0)`

### Requirement: CV and gate handling

`PagedHostIO::tickControls()` SHALL run the CV presence loop and gate Schmidt trigger consistent with `DaisyIO` behavior, then batch `KnobUpdate` for all eight knob positions.

#### Scenario: CV marks external active

- **WHEN** CV input 0 rises above threshold with sufficient delta
- **THEN** `m_modMgr.m_externalCvActive[0]` becomes true

### Requirement: Audio block integration

`PagedHostIO::ProcessBlock` SHALL call `tickControls()` once, then `FroggersEngine::ProcessBlock`.

#### Scenario: Controls tick once per block

- **WHEN** `ProcessBlock` runs with 128 samples
- **THEN** `tickControls` executes exactly once per call

### Requirement: Sim-only VCO morph routing

`PagedHostIO` SHALL expose `SetVcoMorph(vcoIndex, value)`, `GetVcoMorph(vcoIndex)`, and `RandomizeVcoMorphs()`. Morph targets SHALL be mod-assignable. `DaisyIO` SHALL NOT gain equivalent handlers.

#### Scenario: Web sets VCO1 morph

- **WHEN** the web UI sends morph 0.75 for VCO1
- **THEN** `PagedHostIO` updates VCO1 morph and `EvalWaveMorph` reflects it on next `ProcessBlock`

### Requirement: Audio page morph display

On the Audio page, OLED rows 0–2 SHALL show a wave icon beside the param name; morph is controlled inline — **no additional OLED row** for wave shape.

#### Scenario: No extra wave row

- **WHEN** the web sim displays the Audio page
- **THEN** there are exactly 8 OLED rows and wave icons appear only on rows 0–2 name cells
