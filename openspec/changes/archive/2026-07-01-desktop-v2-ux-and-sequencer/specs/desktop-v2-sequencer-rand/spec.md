## ADDED Requirements

**Audit 2026-06-30:** `SequencerStepSnapshot` holds six global scene floats (`sim/SequencerState.hpp` L7–16). `applySequencerStepSnapshot` morphs those across all pages/rows (`FroggersV2ControlCore.cpp` L226–245). No edit-step UI (`SequencerPanelComponent.hpp` L27–28). This capability replaces that model.

### Requirement: v2-sequencer-full-step-snapshot

Each sequencer step SHALL store a full scene-slot snapshot: `sceneCenter[page][row][scene]` for `page < kNumHostPages`, `row < rowsForPage(page)`, and `scene < 3`, plus `crunchySceneCenter[3]`, `gestureWeight[0..1]`, `gate`, and `hasData`.

A step with `hasData == false` is **blank**. Pattern-mode Rand-seq fills blank steps only.

#### Scenario: Snapshot size matches control core scene storage

- **WHEN** a step is captured or randomized
- **THEN** stored scene values cover all musical rows on all host pages up to each page's `rowsForPage`
- **THEN** Crispy row values are stored but Rand-seq does not rewrite them (same skip as Rand All)
- **THEN** `crunchySceneCenter[0..2]` are stored and Rand-seq randomizes them with other scene slots

#### Scenario: Playback recalls per-row scene slots

- **WHEN** the playhead enters step N during **Start Sequence** playback
- **THEN** `applySequencerStepSnapshot` copies step N's stored `sceneCenter` into `m_params[page][row].sceneCenter[*]` for every page/row
- **THEN** Crunchy `sceneCenter[0..2]` copy from `crunchySceneCenter[0..2]` in the step snapshot
- **THEN** gesture weights copy from the step snapshot
- **THEN** live scene blend and S1/S2/S3 endpoint ordinals are unchanged

#### Scenario: Blank step definition

- **WHEN** a step has never been written by Rand-seq, record capture, or explicit authoring
- **THEN** `hasData` is false
- **THEN** Pattern-mode dice randomizes and writes that step
- **WHEN** a step already has `hasData == true`
- **THEN** Pattern-mode dice leaves it unchanged

### Requirement: v2-sequencer-edit-step-toolbar

The sequencer panel SHALL expose an edit-step toolbar above the step grid:

- **Previous step** button with left-arrow icon — decrements edit step within pattern length (wrap)
- **Next step** button with right-arrow icon — increments edit step within pattern length (wrap)
- **Rand-seq** button with dice icon — runs scene-slot randomization into step buffer(s) per scope toggle
- **Step / Pattern** scope toggle — **Step** targets edit step only; **Pattern** targets all blank steps in `0 .. patternLength-1`

Edit step (`m_editStep`) is distinct from playback playhead (`m_playhead`).

#### Scenario: Arrow navigation wraps

- **WHEN** pattern length is 16 and edit step is 0
- **THEN** previous step sets edit step to 15
- **WHEN** edit step is 15
- **THEN** next step sets edit step to 0

#### Scenario: Step grid single click selects edit step

- **WHEN** the operator **single-clicks** step button K in the grid
- **THEN** edit step becomes K
- **THEN** step K gate does **not** change

#### Scenario: Step grid double click toggles gate

- **WHEN** the operator **double-clicks** step button K in the grid
- **THEN** `m_steps[K].gate` toggles (lit ↔ rest)
- **THEN** edit step becomes K (same as single-click on the first click of the double-click)

#### Scenario: Step grid right-click opens context menu

- **WHEN** the operator **right-clicks** step button K in the grid
- **THEN** a context menu appears with **Reset** and **Randomize**
- **THEN** edit step becomes K
- **THEN** step K gate does **not** toggle

#### Scenario: Dual highlight

- **WHEN** edit step and playhead differ
- **THEN** both steps are visually distinct (playhead vs edit selection)
- **WHEN** they coincide
- **THEN** a single combined highlight is acceptable

### Requirement: v2-sequencer-gate-cell-stopped-dim

When `SequencerState::m_playing == false`, gate cells with `gate == true` SHALL render **dimmed** (reduced opacity or muted fill) to distinguish stored pattern data from live pattern playback. When `m_playing == true`, gate cells SHALL render at full brightness. `SequencerPanelComponent` SHALL refresh gate cell appearance when `m_playing` changes.

#### Scenario: Stopped sequencer dims lit gates

- **WHEN** **Start Sequence** is off and step K has `gate == true`
- **THEN** step K gate cell appears dimmed
- **THEN** the envelope is unaffected (open gate per `v2-step-gates-require-start-sequence`)

#### Scenario: Start Sequence restores full gate brightness

- **WHEN** the operator starts **Start Sequence**
- **THEN** all lit gate cells render at full brightness
- **WHEN** **Start Sequence** stops
- **THEN** lit gate cells return to dimmed appearance

#### Scenario: Dice icon adjacent to arrows

- **WHEN** the sequencer panel is visible
- **THEN** prev-arrow, next-arrow, and dice buttons appear on one toolbar row (dice immediately after arrows)

### Requirement: v2-sequencer-step-context-menu

Each step cell in the sequencer grid SHALL support a **right-click** context menu with exactly two actions: **Reset** and **Randomize**. Actions apply to the clicked step index only.

#### Scenario: Reset writes factory cold-start snapshot

- **WHEN** the operator chooses **Reset** on step N
- **THEN** `m_steps[N]` receives the factory snapshot from `captureFactoryStepSnapshot()` (inventory `pageKnobDefault` per row, Crunchy slots **0.0**, gesture weights **0.0**, gate **false**)
- **THEN** `hasData` is **true**
- **THEN** live `m_params`, other steps, and global scene L/R/blend are unchanged

#### Scenario: Randomize writes full step snapshot only

- **WHEN** the operator chooses **Randomize** on step N
- **THEN** `randomizeFullStepSnapshot` rewrites all storable fields in `m_steps[N]` (scene slots, Crunchy slots, gesture weights, gate)
- **THEN** `hasData` is **true**
- **THEN** live knobs, other steps, mod depths, and global scene L/R/blend are unchanged

#### Scenario: Context menu Randomize differs from Rand-seq dice

- **WHEN** the operator uses step context menu **Randomize**
- **THEN** `randomizeSceneEndpointsAndBlend()` is **not** called
- **WHEN** the operator presses toolbar **Rand-seq** dice in **Step** or **Pattern** scope
- **THEN** scene slots are randomized **and** `randomizeSceneEndpointsAndBlend()` runs once per dice press

#### Scenario: VST step context menu parity

- **WHEN** FroggersTigaPluginV2 editor shows the sequencer grid
- **THEN** right-click **Reset** / **Randomize** behave the same as standalone desktop v2
- **THEN** neither action mutates live knobs until step playback recall

### Requirement: v2-sequencer-ui-shared-component

Desktop v2 and VST v2 SHALL use the **same** `SequencerPanelComponent` (and performance-band sequencer controls) inside `MainComponent` and `HostedMainComponentV2`. Sequencer UX requirements in this capability apply to **both** hosts without a VST-specific fork.

### Requirement: v2-sequencer-rand-seq-scope

Rand-seq SHALL randomize **scene slots only** — the same per-row `sceneCenter[0..2]` policy as carousel **Randomize** / **Rand All** scene loop (`onRandAll` `FroggersV2ControlCore.cpp` L482–488):

- All host pages and musical rows
- Skip `crispyRowForPage(page)` (Audio row 7, expanded/ADSR row 9)
- **Crunchy** `sceneCenter[0..2]` included
- Gestures zeroed in the written snapshot
- No mod-depth writes into the step buffer
- **L/R endpoint ordinals and scene blend** randomized once per dice press via `randomizeSceneEndpointsAndBlend()` (live globals; not stored per step in v1 of step snapshot)

Rand-seq writes step buffer(s) and updates live morph globals. Step playback recalls stored scene slots; heard morph uses **current** live L/R/blend unless a future change adds per-step morph storage.

#### Scenario: Step mode dice

- **WHEN** scope is **Step** and the operator presses dice with edit step = 4
- **THEN** step 4 receives a fresh randomized scene snapshot and `hasData = true`
- **THEN** steps 0–3 and 5–15 are unchanged

#### Scenario: Pattern mode dice fills blanks

- **WHEN** scope is **Pattern**, pattern length is 16, and steps 2, 5, 9 have `hasData == false`
- **THEN** steps 2, 5, and 9 each receive independent randomized scene snapshots
- **THEN** steps with `hasData == true` are unchanged

#### Scenario: Rand-seq dice updates L/R indicators

- **WHEN** Rand-seq dice completes
- **THEN** performance band S1/S2/S3 suffixes and blend slider position reflect the new randomized endpoints and blend

#### Scenario: Rand-seq shares randomize implementation

- **WHEN** scene slots are randomized for Rand-seq, RandPage, or Rand All
- **THEN** the same scene-slot loop implementation is used (OMNI: no duplicate rand loops)
- **WHEN** Rand All or Rand-seq runs
- **THEN** the same `randomizeSceneEndpointsAndBlend()` helper is used

### Requirement: v2-sequencer-record-capture

When record arm is on and the playhead advances to a new step, the control core SHALL capture the current `m_params` scene slots and gesture weights into that step and set `hasData = true`.

#### Scenario: Record writes full snapshot

- **WHEN** record arm is active and the playhead advances from step 3 to step 4
- **THEN** step 4 stores the current per-row scene snapshot before or as it becomes active per bridge clock ordering
