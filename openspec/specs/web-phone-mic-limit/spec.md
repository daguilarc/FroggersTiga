# web-phone-mic-limit Specification

## Purpose
TBD - created by archiving change web-vco-morph-mobile. Update Purpose after archive.
## Requirements
### Requirement: Worklet external-input limiting

When external input is enabled, the AudioWorklet SHALL apply input pad and soft limiting before passing samples to the WASM engine.

#### Scenario: Hot mic samples are tamed

- **WHEN** external input is enabled and the capture device delivers samples with peak near 1.0
- **THEN** the samples written to the WASM input buffer are soft-limited to bounded amplitude (|sample| ≤ 1.0)
- **AND** the external input peak meter reflects the limited level sent to the engine

### Requirement: Mobile echo cancellation for phone mic

On mobile viewports, external audio capture SHALL enable browser echo cancellation.

#### Scenario: Mobile getUserMedia constraints

- **WHEN** the user enables External on a viewport ≤720px wide
- **THEN** `getUserMedia` requests `echoCancellation: true` and `autoGainControl: false`

#### Scenario: Desktop capture unchanged

- **WHEN** the user enables External on a viewport ≥721px wide
- **THEN** `getUserMedia` keeps `echoCancellation: false` for line-in / interface use

### Requirement: Stronger engine ext-input limiter

The WASM engine external-input limiter drive SHALL be increased to reduce ring-mod feedback from hot external sources.

#### Scenario: Engine limiter drive

- **WHEN** the WASM module is rebuilt after this change
- **THEN** `x_extInputLimiterDrive` is greater than 3.0 (target 5.0)

