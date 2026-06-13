# stereo-delay Specification

## Purpose
TBD - created by archiving change stereo-delay-page. Update Purpose after archive.
## Requirements
### Requirement: Sim-only delay with pre-reverb insert

Stereo delay SHALL exist only in sim builds (web + desktop). `FroggersEngine` SHALL NOT register a Delay `PageManager` page. Delay DSP SHALL run after the resonant bump and before `ProcessReverb` via an optional sim FX insert. When the insert is not registered or delay send is zero, audio SHALL match the current firmware path through that point.

#### Scenario: Core page count unchanged

- **WHEN** WASM host initializes
- **THEN** `froggers_num_pages()` SHALL return 5

#### Scenario: Firmware hook inert

- **WHEN** firmware runs `ApplyOutputFx` on Daisy Field
- **THEN** no delay processing SHALL occur
- **AND** output SHALL be identical to the pre-change firmware behavior

#### Scenario: Pre-reverb ordering

- **WHEN** sim audio runs with **DSND** greater than zero and **RVMX** greater than zero
- **THEN** delayed material SHALL enter `ProcessReverb` (including **RPRE**) before the reverb mix
- **AND** delay SHALL NOT process the post-reverb mixed output

### Requirement: Delay parameter layout

Host Delay page SHALL expose **DTIM**, **DSND**, **DFBK**, **DWID**, **DTON**, **DMOD**, **DMIX**, and **FUEG** at host page index 5.

#### Scenario: Delay time range

- **WHEN** **DTIM** is maximum at 44100 Hz sample rate
- **THEN** delay time SHALL be within 5% of 3.0 seconds

#### Scenario: Send and mix independence

- **WHEN** **DSND** is zero
- **THEN** delay wet signal SHALL be silent regardless of **DMIX**

### Requirement: Fuegoization on Delay params

**FUEG** (position 7) SHALL apply the same bit-mangle/XOR semantics as core `Parameter` fuegoization to Delay parameters 0–6 via shared `Fuegoize.hpp`.

#### Scenario: FUEG scrambles delay params

- **WHEN** **FUEG** is increased from zero with **DTIM** fixed
- **THEN** the effective **DTIM** value used by the delay DSP SHALL change discontinuously per core fuegoization rules

#### Scenario: FUEG at zero is identity

- **WHEN** **FUEG** is zero
- **THEN** Delay parameters 0–6 SHALL pass through unchanged after mod application

### Requirement: Parameter smoothing

Delay host parameters SHALL be smoothed per block equivalent to core `RuntimeParam` behavior to avoid zipper noise.

#### Scenario: Knob step without zip

- **WHEN** **DTIM** changes abruptly during audio playback
- **THEN** pitch glide SHALL NOT produce block-boundary discontinuities audible as clicks from unsmoothed param jumps

### Requirement: Stereo delay and output

The delay SHALL maintain separate L/R delay lines. Sim hosts SHALL write stereo output when the audio device exposes at least two output channels. Single-channel output SHALL use mono downmix `(L + R) * 0.5`.

#### Scenario: Width at zero

- **WHEN** **DWID** is 0 and a single impulse is sent
- **THEN** left and right wet samples SHALL be equal

#### Scenario: Stereo device

- **WHEN** the host audio device provides two output channels and **DWID** is maximum
- **THEN** left and right output channels SHALL differ during ping-pong feedback

#### Scenario: Mono fallback

- **WHEN** only one output channel is available
- **THEN** the host SHALL output `(outL + outR) * 0.5` without clipping due to summing

### Requirement: Stereo output bus math

After `ProcessBlock`, sim hosts SHALL restore delay stereo width using last-sample wet L/R from `StereoDelay`:

```text
monoWet = (wetL + wetR) * 0.5
deltaL = wetL - monoWet
deltaR = wetR - monoWet
outL = coreMono + DMIX * deltaL
outR = coreMono + DMIX * deltaR
```

Pre-reverb insert SHALL feed `(1 - DMIX) * bump + DMIX * monoWet` into `ProcessReverb`.

#### Scenario: DMIX zero bypasses width restore

- **WHEN** **DMIX** is zero
- **THEN** `outL` and `outR` SHALL equal `coreMono` (within float tolerance)

#### Scenario: Width restore at full DMIX

- **WHEN** **DMIX** is 1, **DSND** is non-zero, and **DWID** is maximum
- **THEN** `outL` and `outR` SHALL differ during ping-pong feedback

### Requirement: Sim FX insert hook

`FroggersEngine` SHALL expose an optional `SimFxInsert` callback invoked after the resonant bump and before `ProcessReverb`. Default SHALL be null (identity). Firmware SHALL NOT register a callback.

#### Scenario: Null hook identity

- **WHEN** the hook is null and delay params are default
- **THEN** engine output SHALL match the pre-change baseline for the same input buffer (max sample diff = 0)

### Requirement: Fuegoize parity

`Fuegoize(value, fuegKnob, row)` SHALL match core `Parameter` fuegoization output for the same inputs across a golden test vector of at least 16 tuples covering rows 0–6 and **FUEG** at 0 and 1.

#### Scenario: Golden vector match

- **WHEN** the parity test runs
- **THEN** max absolute diff between `Fuegoize` and core path SHALL be 0

### Requirement: Delay buffer capacity

`StereoDelay` SHALL support up to 3.0 seconds at 48 kHz (144000 samples per channel) in sim builds only.

#### Scenario: Max delay at 48 kHz

- **WHEN** **DTIM** is maximum and sample rate is 48000 Hz
- **THEN** delay time SHALL be within 5% of 3.0 seconds without buffer overrun

### Requirement: Mod bus sharing

Delay modulation SHALL read mod source levels from the engine's shared `ModMgr::m_mods` array. Delay SHALL NOT maintain a separate mod level bus.

#### Scenario: Marbles mod on DTIM

- **WHEN** row 0 mod source is Marbles 2 with depth 0.5 and Marbles 2 outputs 1.0
- **THEN** effective **DTIM** after mod blend SHALL equal `knob * 0.5 + 1.0 * 0.5` (clamped 0–1) before Fuegoize

### Requirement: Sample rate awareness

Delay buffer sizes and **DTIM** mapping SHALL update when the host sample rate changes.

#### Scenario: Rate change preserves seconds

- **WHEN** sample rate changes with **DTIM** knob unchanged
- **THEN** mapped delay time in seconds SHALL remain approximately the same

