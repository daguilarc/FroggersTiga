## ADDED Requirements

### Requirement: Multi-row module stack geometry

The FroggersTiga VCV product SHALL use **two or three standard Rack rows** (primary module plus expander module(s)), each with widget height exactly `RACK_GRID_HEIGHT`. Total width target 60–84 HP per row (default 72 HP). **Preferred:** two rows (mod rack + I/O on row 1; all six submodule columns on row 2).

#### Scenario: Per-module height

- **WHEN** any module in the stack is placed in a Rack case
- **THEN** its widget `box.size.y` equals `RACK_GRID_HEIGHT` (one 3U row per module)

#### Scenario: Two-row stack

- **WHEN** user adds primary + one Expander linked via expander ports
- **THEN** the combined panel occupies two Eurorack rows with shared engine and DelayState

#### Scenario: All submodules visible

- **WHEN** all expanders are attached
- **THEN** all six submodule titles (Audio, Random, Reverb, Filter, Drive, Delay) are visible without a page-switch knob

### Requirement: Submodule controls and mod input jacks

Each submodule column SHALL expose eight parameter knobs (rows 0–7), labels from `ParamDisplayNames`, and **eight mod input jacks** patchable from mod rack outputs.

#### Scenario: Audio row 7 label

- **WHEN** VCV renders Audio column row 7
- **THEN** the label is **Phase mod 3** per `ParamDisplayNames::forHostPageRow(0, 6)`, not VCO Envelope or VCO level

#### Scenario: Knob drives engine param on correct page

- **WHEN** user turns Audio VCO1 knob on the VCV Audio column
- **THEN** `PagedHostIO::SetPageKnob(0, 0, value)` updates page-0 row-0 without requiring page switch

#### Scenario: Mod cable routes to row on correct page

- **WHEN** user patches Random 1 output to Audio row 2 mod input jack
- **THEN** `SetPageModSource(0, 2, 5)` routing matches desktop patchbay for the same connection

### Requirement: Delay column uses DelayState

The Delay submodule column (page index 5) SHALL drive `DelayState` knobs and mod routes, not engine `PageManager` params alone.

#### Scenario: Delay wet mix knob

- **WHEN** user turns Delay column wet mix (row 6)
- **THEN** `DelayState` wet mix changes and audio output matches desktop Delay panel at the same setting

### Requirement: Mod rack output jacks — LED indicators only

The mod rack SHALL expose output jacks for mod sources MIDI (index 0), VCO Envelope (4), Random 1 (5), and Random 2 (6). Mod sources 4–6 SHALL use **green LED indicators** (on when held CV > 55% while processing). The mod rack SHALL **NOT** include oscilloscope, waveform trace, or scope canvas widgets.

#### Scenario: VCO Envelope indicator

- **WHEN** VCO Envelope mod CV exceeds 55% while audio is running
- **THEN** the VCO Envelope LED is green; no scope trace is drawn

#### Scenario: VCO Envelope is not Audio row 7

- **WHEN** user views Audio column row 7 and mod rack VCO Envelope simultaneously
- **THEN** row 7 is labeled Phase mod 3 and mod rack box is labeled VCO Envelope as separate controls

#### Scenario: Random output voltage

- **WHEN** Random 1 mod CV is active
- **THEN** the Random 1 output jack carries CV proportional to mod level (0–10 V Rack convention)

### Requirement: Block audio processing

VCV `process()` SHALL call `PagedHostIO::ProcessBlock` once per Rack process block with `n > 1`, not once per sample.

#### Scenario: Process block size

- **WHEN** Rack calls `process` with a block of N samples
- **THEN** the engine receives one `ProcessBlock` call with `n = N`

### Requirement: Master I/O preserved

The primary module SHALL retain audio in/out, four CV inputs, gate, MIDI in/out, and CV outputs compatible with the existing process loop.

#### Scenario: Audio passthrough block

- **WHEN** audio input is connected and knobs are at default
- **THEN** audio output carries processed engine signal via `PagedHostIO::ProcessBlock`

### Requirement: GPL boundary

All Rack SDK includes and widget code SHALL remain under `vcv/` only.

#### Scenario: Core header inclusion

- **WHEN** `src/core/FroggersEngine.hpp` is compiled for the VCV plugin
- **THEN** no Rack SDK headers are included from MIT core paths
