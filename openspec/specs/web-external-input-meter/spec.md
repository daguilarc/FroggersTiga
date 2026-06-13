# web-external-input-meter Specification

## Purpose
TBD - created by archiving change web-ext-in-meter. Update Purpose after archive.
## Requirements
### Requirement: Peak meter beside External toggle

The web sim SHALL display a horizontal peak level meter immediately right of the **External** button. The meter SHALL show an idle track (grey background, centre tick) when **External** is off or audio is not playing. When **External** is on and audio is playing, the meter SHALL show a fill width proportional to peak external input level (0–1).

#### Scenario: Idle meter

- **WHEN** **External** is off or **Stop** has been pressed
- **THEN** the meter shows the idle track with no fill
- **AND** the meter is not an empty dead region (centre tick visible)

#### Scenario: Signal present

- **WHEN** **External** is on, **Play** is running, and the capture stream carries signal
- **THEN** the meter fill width reflects peak level from the worklet `screen` message
- **AND** the fill updates without a separate AnalyserNode polling loop

### Requirement: Single peak source in worklet

External input peak SHALL be computed in `froggers-processor.ts` during `process()` from `inputs[0]` when `externalEnabled` is true. The value SHALL be included in the existing `screen` postMessage as `inputPeak`. The main thread SHALL update the meter once per `screen` handler invocation.

#### Scenario: No duplicate peak paths

- **WHEN** reviewing the implementation
- **THEN** peak is accumulated only in the AudioWorklet processor
- **AND** `main.ts` reads `inputPeak` from `screen` messages only

