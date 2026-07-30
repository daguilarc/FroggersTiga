## ADDED Requirements

### Requirement: audio-vco-factory-defaults

`HostParameterInventoryV2` SHALL define Audio page VCO cold-start defaults:

- Rows **0–2** (VCO1–VCO3): `pageKnobDefault(0, row)` = **30 Hz** normalized frequency (`audioVcoFrequencyDefaultNorm()`)
- VcoMorph indices **0–2**: `vcoMorphDefault(index)` = **0.0 / 1.0 / 0.5** (sine / square / saw)

`seedSceneCentersFromDefaults()` picks up frequency defaults automatically. V2 hosts SHALL push morph defaults to the engine on cold start (first `syncToHost` or `Init`).

#### Scenario: shift-revert restores VCO frequency default

- **WHEN** the operator shift-reverts VCO2 (Audio row 1)
- **THEN** all three scene slots reset to `audioVcoFrequencyDefaultNorm()` (**30 Hz**)

#### Scenario: Rand waveforms does not change factory default policy

- **WHEN** the operator presses **Rand waveforms**
- **THEN** morph values randomize per existing engine behavior
- **THEN** cold-start defaults remain defined by inventory (not overwritten until next full app construct)

### Requirement: v2-rand-page-message

`FroggersV2ControlCore` SHALL handle `MessageIn::Type::RandPage` with `message.page` set to the target module index.

`onRandPage(uint8_t page)` SHALL apply the per-row `sceneCenter[0..2]` randomization loop from `onRandAll` scoped to that page only (skip `crispyRowForPage(page)`). It SHALL NOT randomize mod depths, gestures, or global Crunchy.

#### Scenario: RandPage message randomizes one page

- **WHEN** `MessageIn::RandPage` arrives with `page = 2`
- **THEN** all three scene slots on rows 0–8 of page 2 are randomized
- **THEN** page 2 row 9 (Crispy) is unchanged
- **THEN** pages 0–1 and 3–6 are unchanged

#### Scenario: Carousel wires RandPage on v2

- **WHEN** the operator clicks carousel **Randomize** on desktop v2
- **THEN** `MainComponent` / `HostedMainComponentV2` pushes `RandPage` to the control core and calls `syncToHost`
- **THEN** `EnqueueRandomizePanel` is not invoked for `SimHostKind::DesktopV2` or `VstV2`

### Requirement: v2-reset-sequencer-step-message

`FroggersV2ControlCore` SHALL handle `MessageIn::Type::ResetSequencerStep` with `message.index` set to the target step.

The handler SHALL write `captureFactoryStepSnapshot()` into `m_steps[index]` and set `hasData = true`. It SHALL NOT call `syncToHost` for live knob mutation.

#### Scenario: ResetSequencerStep from context menu

- **WHEN** `ResetSequencerStep` arrives with `index = 3`
- **THEN** step 3 equals the factory snapshot
- **THEN** steps 0–2 and 4–63 are unchanged

### Requirement: v2-rand-sequencer-step-message

`FroggersV2ControlCore` SHALL handle `MessageIn::Type::RandSequencerStep` with:

- `message.index` — step index (edit step for toolbar dice; clicked step for context menu **Randomize**)
- `message.page` — scope: `0` = **Step** (toolbar dice, single step + global endpoint/blend rand); `1` = **Pattern** (blank steps only); `2` = **Full step** (context menu — all storable snapshot fields for one step, no global endpoint/blend change)

Scopes **Step** and **Pattern** SHALL call `randomizeSceneSlotsInto` (and `randomizeSceneEndpointsAndBlend()` once per dice press for scope **Step** and **Pattern** only). Scope **Full step** SHALL call `randomizeFullStepSnapshot` on `m_steps[index]` only.

The handler SHALL NOT call `syncToHost` for live knob mutation.

#### Scenario: RandSequencerStep Step scope (toolbar dice)

- **WHEN** `RandSequencerStep` arrives with Pattern scope flag Step and edit step 7
- **THEN** `m_sequencer.m_steps[7]` receives randomized scene slots and `hasData = true`

#### Scenario: RandSequencerStep Pattern scope

- **WHEN** `RandSequencerStep` arrives with Pattern scope and steps 1 and 4 are blank
- **THEN** steps 1 and 4 are randomized; other steps with `hasData == true` are unchanged

#### Scenario: RandSequencerStep Full step scope (context menu)

- **WHEN** `RandSequencerStep` arrives with Full step scope and `index = 7`
- **THEN** step 7 receives `randomizeFullStepSnapshot` (scene + Crunchy + gestures + gate)
- **THEN** live L/R/blend/endpoints are unchanged

### Requirement: v2-sequencer-snapshot-capture-apply

`captureSequencerStepSnapshot` and `applySequencerStepSnapshot` SHALL read/write the full per-row scene snapshot defined in `desktop-v2-sequencer-rand`. The obsolete six-float L/R morph path (`FroggersV2ControlCore.cpp` L226–245) SHALL be removed.

#### Scenario: Capture reads all pages

- **WHEN** `captureSequencerStepSnapshot` is called
- **THEN** output contains `sceneCenter[page][row][scene]` from current `m_params` for all pages and rows up to `rowsForPage`

#### Scenario: Apply writes all pages

- **WHEN** `applySequencerStepSnapshot` runs for a step with `hasData == true`
- **THEN** `m_params[page][row].sceneCenter[*]` match the snapshot for all musical rows
- **THEN** Crunchy `sceneCenter[*]` match `crunchySceneCenter[*]` from the snapshot
