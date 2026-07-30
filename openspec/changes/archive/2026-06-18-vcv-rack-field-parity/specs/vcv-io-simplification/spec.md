## ADDED Requirements

### Requirement: No duplicate external CV or MIDI outs on VCV

The VCV main module SHALL NOT expose bottom-row CV outputs or MIDI Out that duplicate mod-rack signals. VCO Envelope, Random 1, and Random 2 are patchable only from the mod rack.

#### Scenario: CV outs removed

- **WHEN** the main module bottom I/O row is inspected
- **THEN** `CV_OUT1` and `CV_OUT2` jacks are absent
- **THEN** VCO Envelope and Random 1 CV are available only from mod rack outputs (mods 4 and 5)

#### Scenario: MIDI Out removed

- **WHEN** the main module bottom I/O row is inspected
- **THEN** no MIDI Out port or button is present
- **THEN** MIDI In remains for CC ingest

#### Scenario: Essential I/O retained

- **WHEN** the main module I/O row is inspected
- **THEN** audio input, audio output, CV1–CV4 inputs, gate input, and MIDI In remain
- **THEN** the synth produces audio on the main audio output jack

#### Scenario: Rack patching replaces hardware export

- **WHEN** a user wants VCO envelope CV in a Rack patch
- **THEN** they patch from mod rack VCO Envelope output
- **THEN** no separate bottom-row CV jack is required

#### Scenario: No duplicate voltage assignment

- **WHEN** `process()` assigns mod CV to outputs
- **THEN** each mod index is written to exactly one output jack
- **THEN** removed `CV_OUT1/2` params do not retain shadow voltage writes alongside mod rack outputs
