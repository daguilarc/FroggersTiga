## ADDED Requirements

### Requirement: performance-band-readable-labels

Performance band controls SHALL not truncate their primary labels at default window width.

Explicit text labels SHALL identify:
- **Engine** — starts/stops audio processing (top transport row)
- **Start Sequence** — starts sequencer pattern playback when idle (performance band)
- **Stop Sequence** — stops sequencer pattern playback when playing (same button, toggle state)
- **Record** — sequencer record arm toggle
- **BPM** — label adjacent to tempo slider
- **Steps** — label adjacent to pattern-length slider
- **S1**, **S2**, **S3** — scene endpoint buttons (minimum 32px width each); active L/R endpoints show suffix **·L** / **·R** (e.g. `S1·L`, `S3·R`)
- **Scene blend** — slider with **L** label at left end and **R** at right end (blue/orange matching encoder ring arc colors)
- **G1**, **G2** — gesture lane toggles

The sequencer transport button SHALL be at least **108px** wide so **Stop Sequence** renders without ellipsis.

#### Scenario: BPM label visible

- **WHEN** performance band renders at default width
- **THEN** the text "BPM" is visible beside the tempo value control
- **THEN** no control in the sequencer subsection shows ellipsis for a three-letter or shorter intended label

#### Scenario: Engine distinct from Start Sequence

- **WHEN** operator reads standalone top transport and performance band
- **THEN** audio engine control reads **Engine** (not Play)
- **THEN** sequencer playback control reads **Start Sequence** when idle (not Play)

#### Scenario: VST has Start Sequence only

- **WHEN** VST v2 performance band renders
- **THEN** sequencer playback control reads **Start Sequence** / **Stop Sequence**
- **THEN** no **Engine** control is present

#### Scenario: Start Sequence button width

- **WHEN** sequencer is playing and the button label reads **Stop Sequence**
- **THEN** the full label is visible without truncation at `kDefaultWidth`

### Requirement: scene-endpoint-lr-indicators

The performance band SHALL show which scene slots are the current L/R morph endpoints, and SHALL label the scene blend slider ends **L** (left / blend 0) and **R** (right / blend 1).

Data source: `FroggersV2UIState::leftSceneOrdinal` and `rightSceneOrdinal` (`FroggersV2ControlCore.cpp` L214–215). Today these are published but not read by `PerformanceBandV2::refresh()` — this requirement adds that wiring.

#### Scenario: Scene buttons show L and R suffix

- **WHEN** `leftSceneOrdinal = 0` and `rightSceneOrdinal = 2`
- **THEN** the S1 button displays **S1·L** (or equivalent compact form)
- **THEN** the S3 button displays **S3·R**
- **THEN** the S2 button displays **S2** without L/R suffix (not an endpoint)

#### Scenario: Default cold-start indicators

- **WHEN** desktop v2 launches without scene button presses
- **THEN** S1 shows **L** endpoint marking and S2 shows **R** (defaults: ordinals 0 and 1)

#### Scenario: Indicators update after scene button press

- **WHEN** the operator clicks S3 then S1 to assign endpoints
- **THEN** `refresh()` updates button suffixes to match new ordinals without restart

#### Scenario: Blend slider end labels

- **WHEN** the performance band renders
- **THEN** a **L** label appears at the left end of the scene blend slider
- **THEN** a **R** label appears at the right end
- **THEN** **L** uses the same hue as the encoder ring left scene arc (blue); **R** uses the ring right arc hue (orange)

#### Scenario: Rand does not change indicators until user reassigns

- **WHEN** per-page carousel **Randomize** completes
- **THEN** L/R button suffixes and blend slider position are unchanged

#### Scenario: Rand All and Rand-seq update indicators

- **WHEN** Rand All or Rand-seq dice completes
- **THEN** L/R button suffixes and blend slider reflect new `leftSceneOrdinal`, `rightSceneOrdinal`, and `sceneBlend`
