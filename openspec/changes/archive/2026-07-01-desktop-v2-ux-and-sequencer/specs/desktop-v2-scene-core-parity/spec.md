# desktop-v2-scene-core-parity Specification

## Purpose

Desktop v2 knob values live in `FroggersV2ControlCore::ParamState::sceneCenter[3]` per page row. `FroggersV2HostBridge::syncToHost` pushes `effective.effective` to `DesktopHostIO::SetPageKnob` on the active page (`FroggersV2HostBridge.cpp` L29–41).

**Audit 2026-06-30:** Cold-start seeding and shift-revert are implemented (`FroggersV2ControlCore.cpp` L50–52, L599–615). Quick Dict scene semantics fixed (`QUICK_DICT.md` L32–44). Remaining gaps: per-page Randomize still enqueues host only (`MainComponent.cpp` L86, `HostedMainComponentV2.cpp` L37), Rand All still double-writes via host (`GlobalStripV2.cpp` L109–112).

## Requirements

### Requirement: scene-centers-seeded-from-inventory-defaults

On `FroggersV2ControlCore` construction, every host page row SHALL initialize all three scene slots from `HostParameterInventoryV2::pageKnobDefault(page, row)` (`HostParameterInventoryV2.hpp` L85–107).

#### Scenario: cold start audible baseline on active page

- **WHEN** the app constructs `FroggersV2ControlCore` and `FroggersV2HostBridge::syncToHost` runs for page 0
- **THEN** `effectiveRow(page, row).effective` equals `pageKnobDefault(page, row)` for each row on that page (S1/S2 morph at default blend 0.5 with identical seeded slots)
- **THEN** Pair-AR attack/release rows (page 6, rows 0–5) seed per inventory after Phase P (no sustain rows)

#### Scenario: Audio VCO rows seed to 30 Hz

- **WHEN** cold start completes for Audio page rows **0**, **1**, and **2** (VCO1–VCO3)
- **THEN** `pageKnobDefault(0, row)` equals `audioVcoFrequencyDefaultNorm()` — the normalized knob value that maps to **30 Hz** through engine `ExpParam::Compute(20, 20000, norm)` (`FroggersEngine.hpp` L372–374)
- **THEN** all three `sceneCenter[scene]` for each VCO row equal that norm
- **THEN** after first host sync, `GetPageParam(0, row)` for rows 0–2 equals the same norm

#### Scenario: Audio VCO morph defaults sine square saw

- **WHEN** a v2 host cold-starts and applies inventory defaults
- **THEN** `GetVcoMorph(0)` equals **0.0** (sine per `EvalWaveMorph`)
- **THEN** `GetVcoMorph(1)` equals **1.0** (square)
- **THEN** `GetVcoMorph(2)` equals **0.5** (saw)

#### Scenario: all three scene slots share factory value

- **WHEN** init completes for any musical row
- **THEN** `sceneCenter[0]`, `sceneCenter[1]`, and `sceneCenter[2]` each equal `pageKnobDefault(page, row)`
- **THEN** Crispy rows seed to **0.0** (inventory Crispy rule unchanged)

### Requirement: shift-revert-restores-inventory-defaults

Shift + encoder press SHALL call `resetParameter`, restoring scenes and mod depths to inventory defaults — not zero.

Authority: `SIM_MANUAL.md` interaction matrix — **Shift + press | Revert param + depths to default**.

#### Scenario: shift press after edit

- **WHEN** the operator holds Shift and presses an encoder row that was edited away from default
- **THEN** all `sceneCenter[scene]` for that row reset to `pageKnobDefault(page, row)`
- **THEN** all `modDepth[source]` for that row reset to `HostParameterInventoryV2::modDepthDefault()` (**0.5**)
- **THEN** `gestureDepth[lane]` reset to **0.0** (unchanged)

### Requirement: per-page-randomize-writes-all-scene-slots

Per-module **Randomize** SHALL mirror **Rand All** scene behavior scoped to **one carousel page**:

| Field | Per-page Randomize | Rand All (`onRandAll`) |
|-------|-------------------|------------------------|
| `sceneCenter[0..2]` per musical row | ✅ that page only | ✅ all pages |
| `modDepth[*]` | ❌ (use **Rand mod**) | ✅ |
| `gestureDepth` / gesture selection | ❌ unchanged | cleared |
| `m_globalCrunchy` / Crunchy `sceneCenter[0..2]` | ❌ unchanged | ✅ all three slots randomized |
| L/R endpoint ordinals + scene blend | ❌ unchanged | ✅ randomized |

Implementation: `MessageIn::RandPage` → `onRandPage(uint8_t page)` applying the same per-row scene loop as `onRandAll` (`FroggersV2ControlCore.cpp` L482–489) for rows `0 .. rowsForPage(page)-1` except `row == crispyRowForPage(page)` (`crispyRowForPage`: Audio row **7**, expanded modules row **9**, **Pair-AR row 6**).

Host `EnqueueRandomizePanel` is not knob authority on v2: `syncToHost` overwrites host knobs from scene-derived effective values.

#### Scenario: randomize current module updates all scene slots per row

- **WHEN** the operator clicks **Randomize** on module page P
- **THEN** `onRandPage(P)` assigns new values to `sceneCenter[0]`, `sceneCenter[1]`, and `sceneCenter[2]` for each musical row on page P (skip `crispyRowForPage(P)`)
- **THEN** rows on other pages are unchanged
- **THEN** after `syncToHost`, heard knob values on page P change without manual ring edits

#### Scenario: per-page randomize does not change endpoint metadata

- **WHEN** per-page Randomize completes
- **THEN** `m_sceneLeftOrdinal`, `m_sceneRightOrdinal`, and `m_sceneBlend` are unchanged (global morph assignment — only **Rand All** and **Rand-seq** dice reassign)

### Requirement: randomize-scene-endpoints-and-blend

**Rand All** and **Rand-seq (dice)** SHALL randomize global scene morph assignment before or with scene-slot writes:

- `m_sceneLeftOrdinal` and `m_sceneRightOrdinal` SHALL be two **distinct** values in `{0, 1, 2}`
- `m_sceneBlend` SHALL be uniform random in **`[0.0, 1.0]`**
- `m_sceneSelectFlip` SHALL reset to **0** (next manual scene click assigns left)

Implementation: shared helper `randomizeSceneEndpointsAndBlend()` called from `onRandAll` and `RandSequencerStep` handlers. Uses the same PRNG stream as scene-slot randomization.

Per-page carousel **Randomize** SHALL NOT call this helper (module-scoped scene slots only).

#### Scenario: Rand All picks new L/R pair

- **WHEN** Rand All completes
- **THEN** `m_sceneLeftOrdinal ≠ m_sceneRightOrdinal`
- **THEN** `m_sceneBlend` is a new value in `[0, 1]` (not preserved from pre-rand state unless PRNG collision)
- **THEN** performance band **S{n}·L** / **S{n}·R** indicators reflect the new ordinals

#### Scenario: Rand All endpoint pair is never Sx/Sx

- **WHEN** Rand All randomizes endpoints
- **THEN** left and right ordinals are never equal

#### Scenario: Rand-seq dice randomizes endpoints once per press

- **WHEN** the operator presses Rand-seq dice (Step or Pattern scope)
- **THEN** `randomizeSceneEndpointsAndBlend()` runs **once** for that press
- **THEN** step buffer scene data uses the new L/R/blend for heard morph on subsequent playback (live globals updated immediately)

#### Scenario: host-only randomize is not v2 authority

- **WHEN** `SimHostKind::DesktopV2` or `VstV2` handles carousel **Randomize** on standalone or hosted editor
- **THEN** `onRandPage` runs in the control core
- **THEN** `EnqueueRandomizePanel` is not called

#### Scenario: inactive page unchanged

- **WHEN** Randomize runs on Filter while Audio is not active
- **THEN** all `sceneCenter[*]` on Audio rows remain at their pre-randomize values

### Requirement: rand-all-randomizes-scenes-without-redundant-host-knobs

**Rand All** SHALL randomize all pages' `sceneCenter[0..2]` and `modDepth[*]` in `onRandAll`, skip `crispyRowForPage(page)` on every page, randomize **all three Crunchy scene slots**, **randomize L/R endpoint ordinals and scene blend** per `randomize-scene-endpoints-and-blend`, and clear gesture selection. On v2 hosts it SHALL NOT rely on `EnqueueRandomizeAllPages` as the knob authority.

**Terminology:** Rand All rewrites stored scene positions **and** global morph assignment (which two slots are L/R + blend slider). Cold start still uses defaults S1·L / S2·R / blend **0.5**.

#### Scenario: rand all changes inactive pages

- **WHEN** Rand All runs while page 0 is active
- **THEN** `sceneCenter[0..2]` on pages 1–6 are randomized in core
- **WHEN** the operator switches to another page and `syncToHost` runs
- **THEN** heard values reflect randomized scene data for that page

#### Scenario: rand all skips crispy on every page

- **WHEN** Rand All runs
- **THEN** row `crispyRowForPage(page)` is not randomized on any page (Audio row 7, expanded/ADSR row 9)

### Requirement: crunchy-scene-encoder-parity

Global **Crunchy** on desktop v2 SHALL use the same scene-slot storage and encoder-ring interaction model as module parameter rows.

Crunchy SHALL store `sceneCenter[0]`, `sceneCenter[1]`, and `sceneCenter[2]`. The heard/host value SHALL be the blend of those slots using live `m_sceneLeftOrdinal`, `m_sceneRightOrdinal`, and `m_sceneBlend` — identical math to `blendedSceneCenter()` on module rows.

The global strip SHALL render Crunchy as an `EncoderRingComponent` with left/right scene arcs (not a plain `juce::Slider` rotary).

Ring drag with no gesture selected SHALL edit the active Crunchy scene slot. Shift+press on the Crunchy ring SHALL reset all three slots to **0.0**.

Per-page carousel **Randomize** SHALL NOT change Crunchy scene slots. **Rand All** and **Rand-seq** SHALL randomize all three Crunchy scene slots.

Web/WASM hosts are out of scope (single Crunchy rotary unchanged).

#### Scenario: Crunchy ring shows scene arcs

- **WHEN** desktop v2 renders the global strip
- **THEN** Crunchy is an encoder ring with concentric left/right scene arcs like module rows
- **THEN** the ring effective matches `globalCrunchy()` sent to the host

#### Scenario: Crunchy ring edit uses scene blend

- **WHEN** scene blend is 0.0 (L endpoint) and the operator turns the Crunchy ring
- **THEN** `sceneCenter[m_sceneLeftOrdinal]` changes
- **THEN** `SetGlobalCrunchy` receives the blended effective after `syncToHost`

#### Scenario: Crunchy seeded to factory off

- **WHEN** `FroggersV2ControlCore` constructs
- **THEN** all three Crunchy scene slots equal **0.0**

#### Scenario: Rand All randomizes Crunchy scene slots

- **WHEN** Rand All completes
- **THEN** Crunchy `sceneCenter[0..2]` each receive new values
- **THEN** no separate scalar `m_globalCrunchy` assignment remains in `onRandAll`

#### Scenario: shift revert on Crunchy ring

- **WHEN** the operator holds Shift and presses the Crunchy encoder ring
- **THEN** all three Crunchy scene slots reset to **0.0**

### Requirement: operator-docs-scene-semantics

Quick Dict and SIM_MANUAL **desktop v2** scene entries SHALL match code (SIM_MANUAL excludes VST/VCV per `sim-manual-excludes-vst-and-vcv`):

- S1/S2/S3 buttons select L/R morph endpoints (`onSceneSelect`); they do not snapshot current knob positions on press
- Ring turns (no gesture selected) edit the scene slot selected by blend between L/R ordinals
- Scenes store per-knob positions (including **Crunchy** on desktop v2); gestures store per-knob offsets; sequencer steps are a separate timed layer

#### Scenario: quick dict scene row accuracy

- **WHEN** an operator reads **Scene S1 / S2 / S3** in `QUICK_DICT.md`
- **THEN** the entry describes endpoint selection and ring editing — not "store current knob positions on button press"
