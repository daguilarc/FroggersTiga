## ADDED Requirements

### Requirement: Hosted UI mutations apply while transport is stopped
VST/AU UI mutations SHALL drain through one safe apply path even when the DAW transport is stopped and no audio callback is running.

#### Scenario: Randmod while stopped
- **WHEN** the operator invokes Randmod with DAW transport stopped
- **THEN** the parameter routes and visible controls update without requiring playback

### Requirement: Patch overlay is a synchronized derived view
The hosted patch-cable overlay SHALL derive routes from current sim state and resynchronize after preset restore, Randmod, Rand Mods, manual patching, and route clearing. Cable color metadata SHALL NOT be an independent routing authority.

#### Scenario: Preset restore
- **WHEN** a DAW restores plugin state containing mod routes
- **THEN** visible patch cables match the restored `modIndex` routes

### Requirement: Hosted chrome and keyboard policy are DAW-appropriate
Hosted mode SHALL hide standalone-only Record/Export and hardware Audio/MIDI controls and SHALL disable Froggers QWERTY MIDI capture so DAW typing cannot change sound.

#### Scenario: Plugin editor opens
- **WHEN** the editor is hosted by a DAW
- **THEN** standalone transport/export/device controls are absent and typing does not generate Froggers MIDI control

### Requirement: Hosted editor minimum preserves the full control surface
The VST/AU editor minimum size SHALL be large enough to keep its labels, internal three-cell mod rack, and routing overlay usable without clipping.

#### Scenario: Editor reaches minimum size
- **WHEN** the DAW resizes the editor to its declared minimum
- **THEN** labels, mod cells, and patch endpoints remain visible and operable

### Requirement: VST3 and AU verification is layered and format-correct
Hosted verification SHALL include processor-level parameter/state/render tests, Steinberg VST3 validation, Tracktion `pluginval`, Apple `auval`, a VST3 smoke in a VST3-capable DAW, an AU smoke in Logic Pro, and cross-format state/render parity. Logic Pro SHALL NOT be named or used as a VST3 host. Validator success SHALL NOT replace editor, automation, project-recall, or audio-behavior checks in real hosts.

#### Scenario: Required automated validation
- **WHEN** the hosted verification gate runs
- **THEN** processor tests, Steinberg VST3 validation, `pluginval` strictness 5 or higher, `auval`, and VST3/AU parity checks pass for the produced bundles

#### Scenario: Format-correct DAW smoke tests
- **WHEN** manual hosted verification is performed on macOS
- **THEN** the VST3 is exercised in a VST3-capable host such as REAPER and the AU is exercised in Logic Pro

#### Scenario: Host behavior matrix
- **WHEN** each real-host smoke runs
- **THEN** it covers three-or-more DAW MIDI-learn mappings, automation record/playback, stopped-transport mutations, state recall, editor reopen/resize, zero-input rendering, and optional mono input where exposed
