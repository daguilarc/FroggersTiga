## ADDED Requirements

### Requirement: VCV Rack-facing controls use named sections
The VCV Rack implementation SHALL expose and consume Rack controls as named sections, not pages. VCV wrapper code SHALL NOT depend on `m_currentPage`, page navigation, or shared hardware/current-page knob latch state when applying Rack knob, CV, randomize, or mod-route changes.

The required section names are Audio, Random, Filter, Drive, Reverb, Delay, Global, and VCO AR. The implementation MAY map these sections to existing shared-engine banks internally, but Rack-facing APIs, tests, and documentation SHALL use section/extension language.

#### Scenario: Rack column writes section values
- **WHEN** the VCV Filter column row 1 knob changes
- **THEN** the VCV adapter records the change as `Filter` row 1 section state
- **THEN** no VCV code path updates `m_currentPage` or shared `m_knobPositions` to apply that Rack knob

#### Scenario: Engine bank mapping stays behind adapter
- **WHEN** the VCV adapter applies `Drive` row 2 to the shared engine
- **THEN** any legacy page-index mapping remains encapsulated behind the adapter
- **THEN** Rack module code does not call `SetPageKnob` or `KnobUpdateOnPage` directly

### Requirement: Main module owns engine and audio processing
The VCV main module SHALL own the shared engine, audio processing call, sample-rate handling, global modulation outputs, audio/CV/gate I/O, Random trigger, global Crunchy control, and global Crunchy CV input.

Left and right extensions SHALL be optional control contributors. Extensions SHALL publish bounded control snapshots to the main module and SHALL NOT independently mutate shared engine parameter state in a separate process-order-dependent callback.

#### Scenario: Main processes one coherent snapshot
- **WHEN** a main module has both left and right Froggers extensions linked
- **THEN** main collects their section snapshots before audio processing
- **THEN** main applies one coherent VCV control snapshot during its own `process`

#### Scenario: Extension absent
- **WHEN** the right FX extension is not linked
- **THEN** main processes audio using default Reverb/Delay section state
- **THEN** no null extension is required for audio output

### Requirement: Left VCO AR extension is page-free and optional
The VCV left extension SHALL contribute VCO AR controls when present. It SHALL expose Attack and Release controls for VCO1, VCO2, and VCO3, a local Crispy control, Randomize and Randmod controls, and optional per-row CV inputs. It SHALL link to the main module from the left side and SHALL NOT own audio or engine state.

If the left extension is absent, the main module SHALL use default VCO AR values.

#### Scenario: Left extension controls VCO AR
- **WHEN** `Froggers Tiga VCO AR` is immediately left of main
- **THEN** main receives VCO AR section values for six attack/release rows and local Crispy
- **THEN** VCO AR values are applied without selecting an engine page

#### Scenario: Left extension randomize is scoped
- **WHEN** the user presses Randomize on the VCO AR left extension
- **THEN** only VCO AR section base controls are randomized
- **THEN** Audio, Random, Filter, Drive, Reverb, and Delay section base controls are unchanged

### Requirement: Right extensions contribute section and FX controls
VCV right extensions SHALL contribute visible section controls and optional FX/stereo I/O to main through snapshots. The current right FX extension SHALL contribute Reverb and Delay section controls and stereo L/R inputs and outputs. A future right section extension MAY contribute Audio, Random, Filter, and Drive controls through the same snapshot contract.

#### Scenario: FX extension contributes Reverb and Delay
- **WHEN** `Froggers Tiga FX` is linked to the right of main
- **THEN** main consumes Reverb and Delay section values from the extension snapshot
- **THEN** the extension does not apply then restore Reverb/Delay internal routes in its own audio callback

#### Scenario: Stereo input source
- **WHEN** either FX extension L/R input is patched
- **THEN** main derives the external input from the FX stereo inputs according to the VCV audio routing contract
- **THEN** main remains the only module that calls the shared engine process function

### Requirement: VCV global Crunchy supports knob and CV input
The VCV main module SHALL expose a global Crunchy knob and a global Crunchy CV input. Effective global Crunchy SHALL be `clamp(knob + cvVolts / 10, 0, 1)` when the CV input is connected and the knob value when disconnected.

Global Crunchy SHALL apply as a global fuego pass across VCV section processing without replacing per-section Crispy controls.

#### Scenario: Global Crunchy CV raises intensity
- **WHEN** the global Crunchy knob is `0.25` and the global Crunchy CV input receives `5 V`
- **THEN** effective global Crunchy is `0.75`

#### Scenario: Global Crunchy CV clamps
- **WHEN** the global Crunchy knob is `0.8` and the global Crunchy CV input receives `5 V`
- **THEN** effective global Crunchy is `1.0`

#### Scenario: Per-section Crispy remains active
- **WHEN** global Crunchy is non-zero and the Drive section Crispy row is also non-zero
- **THEN** the Drive section applies the global Crunchy pass and its section Crispy behavior in the documented order
- **THEN** the section Crispy control is not removed or ignored

### Requirement: VCV CV inputs produce temporary effective values
Each VCV per-parameter CV input SHALL combine with the target's stored base value and stored internal route for the current processing snapshot. The implementation SHALL compute internal route blend once, add normalized Rack CV once when connected, clamp once, and pass the result as an effective value for the current block.

The CV path SHALL NOT persist physical CV as a base knob value, SHALL NOT clear or mutate stored internal route/depth as part of ordinary processing, and SHALL NOT apply stored internal routes twice.

#### Scenario: Disconnected CV preserves internal route
- **WHEN** a Filter row has base `0.25`, internal mod source `5`, depth `1.0`, and the mod source value is `0.4`
- **THEN** the effective value for the row is `0.4`
- **THEN** the stored base remains `0.25`
- **THEN** the stored route remains source `5` with depth `1.0`

#### Scenario: Connected CV combines once
- **WHEN** a Filter row has internal effective value `0.4` and its Rack CV input receives `5 V`
- **THEN** the final effective value is `0.9`
- **THEN** no later engine read applies source `5` a second time to `0.9`

### Requirement: VCV randomization operates on section state
VCV Randomize and Randmod actions SHALL operate on section state directly. VCV SHALL NOT use hardware/current-page randomization paths that read `m_knobPositions`.

Random All SHALL affect all VCV-visible section base controls, including linked optional extension sections. Randmod All SHALL use only VCV-available internal mod sources `4`, `5`, and `6`, or None.

#### Scenario: Random All ignores current-page latch
- **WHEN** Random All is triggered on VCV main
- **THEN** all VCV-visible sections randomize from their stored section values
- **THEN** the result does not depend on `m_currentPage` or `m_knobPositions`

#### Scenario: VCV Randmod excludes MIDI indices
- **WHEN** VCV Randmod All runs for 1000 assignments
- **THEN** each assignment is None, `4`, `5`, or `6`
- **THEN** no assignment is `0` or `1`

### Requirement: VCV local docs use section and extension language
VCV local documentation SHALL describe the Rack product as main plus optional left/right extensions with named sections. Documentation SHALL NOT describe VCV as a paged UI, SHALL NOT mention CC enables or Froggers-owned MIDI, and SHALL NOT describe obsolete expander sizes or modules as current behavior.

#### Scenario: Development guide patch order
- **WHEN** a developer reads `vcv/DEVELOPMENT.md`
- **THEN** the documented patch order is VCO AR left extension, main module, right section/FX extension
- **THEN** no current VCV setup instruction mentions CC enables
