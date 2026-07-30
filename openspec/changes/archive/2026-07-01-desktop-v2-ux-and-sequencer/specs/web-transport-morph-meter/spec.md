## ADDED Requirements

### Requirement: web-engine-action-readiness-unified

Global-strip engine actions (`#rand-morphs`, page randomize buttons, VCO morph cycle) SHALL use the same readiness predicate as `#rand-all` and `#marbles-btn`: `engineActionReady()` (`engineReady` or worklet present with `audioRunning` or `transportIntentPlaying`).

`#rand-morphs` SHALL NOT apply a stricter Play-only gate than other global-strip randomize actions.

#### Scenario: Rand waveforms before engine ready without Play

- **WHEN** the worklet is loaded, `transportIntentPlaying` is true, and `engineReady` is false
- **THEN** clicking Rand waveforms sends `randomizeMorphs` to the worklet (same contract as Rand All)

#### Scenario: Rand waveforms blocked without transport intent

- **WHEN** `engineActionReady()` is false and `audioRunning` is false
- **THEN** clicking Rand waveforms sets `#status` to `Click Play first`
- **THEN** no worklet message is sent

### Requirement: web-status-not-clobbered-when-playing

When `requireEngineForAction()` rejects an action and `audioRunning` is true, the web sim SHALL restore transport status via `applyPlayingStatus()` before returning.

#### Scenario: Playing status preserved after gated action

- **WHEN** audio is running and the user triggers a gated control while `engineActionReady()` is false
- **THEN** `#status` shows the playing transport line (sample rate, external/midi state)
- **THEN** `#status` does not remain stuck on `Click Play first`

### Requirement: web-vco-morph-svg-sync

The main thread SHALL update `lastMorphs` from every screen payload that includes morph data and refresh VCO morph button SVG from `lastMorphs` on each such update, including when the active host page is not Audio.

#### Scenario: Morph SVG updates after Rand waveforms on Audio page

- **WHEN** the user is on the Audio page, audio is running, and Rand waveforms succeeds
- **THEN** all three VCO morph buttons reflect the new morph values from the screen payload

#### Scenario: Morph state retained off Audio page

- **WHEN** the user triggers Rand waveforms from a non-Audio page and returns to Audio
- **THEN** VCO morph buttons show the morph values from the latest screen payload without requiring another randomize action

### Requirement: web-external-meter-labeled-states

The external input monitor SHALL expose an explicit meter label element separate from `#status` with three operator-visible states.

#### Scenario: External off

- **WHEN** External Audio is disabled
- **THEN** the meter label reads `Off`
- **THEN** the meter fill width is 0%

#### Scenario: External on before Play

- **WHEN** External Audio is enabled and the worklet is not pulling audio (`processorRunning` false)
- **THEN** the meter label reads `Waiting for Play`
- **THEN** the meter fill width is 0%

#### Scenario: External on while playing

- **WHEN** External Audio is enabled and the processor is running
- **THEN** the meter label indicates active monitoring
- **THEN** the meter fill reflects `inputPeak` from the screen payload

### Requirement: web-external-peak-requires-output-graph

Peak metering SHALL begin only after `connectWorkletOutput()` has run (Play path). Enabling External before Play connects the mic to the worklet input but SHALL NOT imply peaks until the output graph is connected and `setRunning(true)` is active.

#### Scenario: Play after External enables peaks

- **WHEN** the user enables External Audio then clicks Play
- **THEN** `connectWorkletOutput()` runs before peak display is expected
- **THEN** the meter transitions from `Waiting for Play` to active when `inputPeak` arrives
