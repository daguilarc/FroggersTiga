## ADDED Requirements

**Audit 2026-06-30:** Section headers "MIDI In" / "CV Assignments" exist (`MidiCvSettingsComponent.cpp` L46–55). Open: Unicode `→` in status L317, Ext. mod A/B labels L67–68, no help text, no Ch/Any/pitch-name/gate-help, CC A/B not in mod enum.

### Requirement: midi-cv-settings-operator-copy

The MIDI CV settings dialog SHALL use ASCII-only status and help text (no Unicode arrows).

The dialog SHALL include help labels:
- Under **MIDI In**: explain that the device dropdown selects the single input stream (computer keyboard, none, or hardware port).
- Under **CV Assignments**: explain that rows below map messages from that input to pitch, gate, CC modulators, and performance triggers.

#### Scenario: No stray Unicode in status

- **WHEN** computer keyboard MIDI is active
- **THEN** the status line contains no U+2192 RIGHTWARDS ARROW character

#### Scenario: Two-step flow documented in UI

- **WHEN** the user opens MIDI CV settings
- **THEN** help text distinguishes input device selection from CV assignment rows

### Requirement: midi-cc-ab-labels-and-ch-any

Rows currently labeled **Ext. mod A** / **Ext. mod B** SHALL read **MIDI CC A** / **MIDI CC B**.

Channel sliders SHALL display **Any** when the stored channel value is 0 (all channels).

#### Scenario: Ch zero reads Any

- **WHEN** MIDI CC A channel is 0
- **THEN** the channel control shows **Any**, not bare **0**

### Requirement: pitch-target-parameter-name

The Pitch assignment row SHALL show the resolved parameter name from `V2ParamDisplayNames::forHostPageRow(pitchPage, pitchRow)` when page or row changes.

#### Scenario: Audio row 0 shows VCO1

- **WHEN** pitch page is Audio and row is 0
- **THEN** a read-only label shows the authority string for VCO1

### Requirement: shift-scene-midi-channel-exposed

Shift and Scene S1–S3 binding rows SHALL expose a **Ch** control bound to `MidiCvButtonBinding::channel`.

#### Scenario: Scene S1 channel persisted

- **WHEN** the user sets Scene S1 to Note 60 on channel 3
- **THEN** only channel 3 note-ons trigger scene select

### Requirement: gate-assignment-help

The Gate toggle SHALL include help text stating that MIDI note on/off drives ADSR and sequencer gates.

### Requirement: external-midi-cc-mod-sources

MIDI CC A and MIDI CC B levels SHALL be assignable mod sources in module mod dropdowns and SHALL participate in `computeEffective` when assigned.

#### Scenario: MIDI CC A modulates assigned row

- **WHEN** MIDI CC A is enabled, CC 74 on channel 1 sends value 127, and a knob row assigns mod source **MIDI CC A**
- **THEN** that row's effective value reflects the CC level
