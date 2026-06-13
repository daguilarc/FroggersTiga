## ADDED Requirements

### Requirement: Portable include graph

The `src/core/` tree SHALL contain all DSP and `PageManager` headers required by sim hosts and firmware engine code. No file under `src/core/` SHALL include `daisy_field.h`, `DaisyIO.hpp`, or `App.hpp`.

#### Scenario: Core has no Daisy dependency

- **WHEN** `grep -r daisy src/core` is run
- **THEN** the command returns no matches

### Requirement: FroggersEngine block API

`FroggersEngine` SHALL expose `ProcessSample(float)` and `ProcessBlock(const float* in, float* out, size_t n)` where `ProcessBlock` calls `ReadParamsBlock()` once then `ProcessSample` per sample.

#### Scenario: Block processes without NaN

- **WHEN** a 440 Hz sine at 44.1 kHz is passed through `ProcessBlock` for 1024 samples after `SetSampleRate(44100.f)`
- **THEN** every output sample is finite

### Requirement: Configurable sample rate

`FroggersEngine` SHALL provide `SetSampleRate(float)` and use that rate for all time-normalized DSP mappings in `FroggersEngine`, `RuntimeParam` smoothing alphas, `Marbles::UpdateParams` filter corners, `EQ.hpp`, and reverb pre-delay sample counts. No bare `48000` literals SHALL remain in `FroggersEngine.hpp`, `Marbles.hpp`, `RuntimeParam.hpp`, or `EQ.hpp` after migration. The default after construction SHALL be 44100 Hz until set otherwise. Sim hosts SHALL call `SetSampleRate` with the audio device or `AudioContext` rate on start.

#### Scenario: Firmware uses 48 kHz

- **WHEN** the firmware shim calls `SetSampleRate(48000.f)` before audio starts
- **THEN** reverb and filter mappings match pre-extraction 48 kHz behavior within float tolerance

#### Scenario: Sim default is 44.1 kHz

- **WHEN** a sim host constructs the engine without an explicit call
- **THEN** internal sample rate is 44100 Hz

### Requirement: Firmware shim compatibility

`src/FroggersTiga/FroggersTiga.hpp` SHALL inherit or delegate to `FroggersEngine` and implement Daisy `Process(AudioHandle::...)` by calling `ProcessBlock` on channel 0.

#### Scenario: Firmware builds

- **WHEN** `make` is run in `src/FroggersTiga`
- **THEN** `build/FroggersTiga.bin` is produced successfully

### Requirement: Page-aware randomize for desktop

`PageManager` SHALL provide `KnobUpdateOnPage(uint8_t page, uint8_t position, float value)`, `RandomizePage(uint8_t page)`, and `RandomizeAllPagesIndependent()`. `RandomizePage` SHALL pass each parameter's stored `m_knobValue` to `Parameter::Randomize` (FUEG skipped by existing `Randomize` logic). `RandomizeAllPagesIndependent` SHALL call `RandomizePage` for every page index. Paged hosts SHALL continue using `KnobUpdate` and `RandomizeAllPages` with shared `m_knobPositions`.

#### Scenario: Desktop B1 randomizes one page

- **WHEN** `RandomizePage(drivePage)` is called while Audio page knob 0 is 0.3
- **THEN** only Drive page parameters are randomized and Audio knob 0 remains 0.3

#### Scenario: Desktop B2 does not use stale knob bank

- **WHEN** `RandomizeAllPagesIndependent()` runs with different per-page knob values
- **THEN** each page's `Randomize` receives that page's own `m_knobValue`, not `m_knobPositions`

### Requirement: Sim-only VCO waveform morph DSP

For sim hosts, `FroggersEngine` SHALL provide three `VcoWaveMorph` targets (VCO1–3) with knob values in 0..1. `VcoWaveMorph::GetMorph` SHALL return the modulated knob clamped to 0..1 (linear). It SHALL NOT use `PhaseUtils::ExpParam::Compute(0, 1, knob)` — that produces NaN for knob > 0 and breaks morph audio/icons. `EvalWaveMorph(phase, morph)` SHALL smoothly blend sine → saw → square across morph ∈ [0,1]. Sim `StepOscillators` SHALL use `EvalWaveMorph` for all three VCOs when sim morph mode is active.

**Superseded by:** `desktop-vco-morph-fix` (implementation + queue/idle drain). Original “exponential knob mapping” requirement was incorrect and is withdrawn.

#### Scenario: Morph at endpoints

- **WHEN** VCO1 morph knob is 0.0
- **THEN** `ModulatedMorph(0)` is 0.0 and output matches sine; at knob 1.0 morph is 1.0 and output matches square; at knob 0.5 morph is 0.5 and output matches saw within float tolerance

#### Scenario: Randomize all morphs

- **WHEN** a sim host calls `RandomizeVcoMorphs()`
- **THEN** each VCO morph receives an independent uniform random value in [0,1] via `RGen`

### Requirement: Sim wave morph activation gate

`FroggersEngine` SHALL expose `SetSimWaveMorph(bool)`. The default after construction SHALL be **false**. Sim host adapters SHALL call `SetSimWaveMorph(true)` during `Init()`. The firmware shim SHALL NOT call `SetSimWaveMorph(true)`.

#### Scenario: Firmware never uses EvalWaveMorph

- **WHEN** FroggersTiga firmware runs with default engine construction and `SetSampleRate(48000.f)` only
- **THEN** `StepOscillators` uses discrete `EvalWave` for VCO1/VCO2 and sine for VCO3; `EvalWaveMorph` is not invoked

#### Scenario: Sim host enables morph path

- **WHEN** `PagedHostIO::Init()` or `DesktopHostIO::Init()` completes on a sim host
- **THEN** `SetSimWaveMorph(true)` was called and all three VCOs use `EvalWaveMorph`

#### Scenario: Firmware VCO waves unchanged

- **WHEN** FroggersTiga firmware runs on Daisy Field
- **THEN** VCO1/VCO2 use discrete `EvalWave` from A8/B8; VCO3 uses sine only; `DaisyIO` is unchanged

### Requirement: VCO morph CV modulation

Each `VcoWaveMorph` target SHALL participate in `ModMgr` modulation with the same attenuator semantics as `Parameter` (knob base + mod amount). Sim hosts SHALL expose morph targets for mod-assign; firmware SHALL NOT register morph targets.

#### Scenario: CV modulates VCO2 morph

- **WHEN** M1 is assigned to VCO2 morph with depth 0.8 and CV1 is present
- **THEN** effective morph follows `ModMgr::Modulate` on the morph knob value

#### Scenario: Audio-rate morph modulation

- **WHEN** CV1 modulates VCO2 morph with non-zero depth and CV varies sample-to-sample inside `ProcessBlock`
- **THEN** `EvalWaveMorph` receives per-sample modulated morph values (same read contract as `Parameter::Get`)

### Requirement: Envelope level accessor

`FroggersEngine` SHALL expose `GetEnvelopeLevel()` returning the external envelope follower value (post `m_extEnvFilter`, pre `m_extGate` Schmidt) from the **last** `ProcessSample` in the most recent block. `CvMidiBridge::tickMidiOut` SHALL read this value once per block after `ProcessBlock` completes.

#### Scenario: Silent input

- **WHEN** `ProcessSample(0)` runs repeatedly
- **THEN** `GetEnvelopeLevel()` decays toward zero
