## ADDED Requirements

### Requirement: External MIDI button requests permission on enable

The web sim SHALL provide an **External MIDI** control placed directly under the **External** (audio) control. Web MIDI access SHALL be requested only when the user enables External MIDI. Default state SHALL be off. Play alone SHALL NOT trigger a MIDI permission prompt.

#### Scenario: Permission on enable

- **WHEN** the user clicks External MIDI to turn it on
- **THEN** the browser prompts for Web MIDI access (or shows an error if unavailable or denied)

#### Scenario: Off by default

- **WHEN** the page loads
- **THEN** External MIDI is off and no Web MIDI inputs are connected

### Requirement: Web MIDI CC drives mod sources zero and one

When External MIDI is on, matching Control Change messages SHALL update `mods[0]` (MIDI CC 1 pair) and `mods[1]` (MIDI CC 2 pair) using the same `CvMidiBridge` rules as desktop. Default pairs: channel 1 + CC 1 → mod 0; channel 1 + CC 2 → mod 1.

#### Scenario: CC1 updates mod bay scope

- **WHEN** External MIDI is on and CC 1 value 64 arrives on channel 1
- **THEN** the MIDI CC 1 mod-bay scope reflects approximately 50% level

#### Scenario: CC2 assignable like other mod sources

- **WHEN** the user selects MIDI CC 2 as a knob mod source and sends matching CC traffic
- **THEN** the knob mod depth blends against `mods[1]` the same way as VCO Envelope mod

### Requirement: Mod bay shows MIDI CC 1 and MIDI CC 2 scopes

The web mod bay SHALL display scope indicators for MIDI CC 1 and MIDI CC 2 to the left of VCO Envelope, labeled from wasm mod-source names.

#### Scenario: Five mod bay entries

- **WHEN** mod bay is expanded
- **THEN** scopes appear in order: MIDI CC 1, MIDI CC 2, VCO Envelope, Random 1, Random 2
