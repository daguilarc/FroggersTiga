# vcv-cc-mod-gating Specification

## Purpose
**Historical — superseded by `froggers-host-master`.** Pre-omni VCV CC enable toggles and MIDI ingest for mod indices 0 and 1. Current VCV contract: CV-only module; no Froggers-owned MIDI.
## Requirements
### Requirement: VCV CC ingest through host CvMidiBridge

The VCV module SHALL enqueue inbound MIDI CC via `host.m_midiBridge.PushMidiCc` and SHALL NOT assign CC values directly to `mods[]`. Latching SHALL occur only through `PagedHostIO::ProcessBlock` → `tickControls` → `drainMidiIn`.

#### Scenario: Enabled pair latches CV

- **WHEN** MIDI CC 1 is enabled and a message matches pair 1 channel/CC config
- **THEN** `mods[0]` reflects the normalized CC value after block drain

#### Scenario: Disabled pair ignored

- **WHEN** MIDI CC 2 is disabled and a matching CC message arrives
- **THEN** `mods[1]` is 0.0 after block drain

#### Scenario: Non-pair CC discarded

- **WHEN** a CC message does not match either configured pair (including CC 3 and CC 4)
- **THEN** `mods[2]` and `mods[3]` are unchanged by MIDI ingest

### Requirement: Single CvMidiBridge on PagedHostIO

The VCV primary module SHALL use `host.m_midiBridge` for MIDI in latch, enable flags, and MIDI out export. The module SHALL NOT maintain a separate `CvMidiBridge` instance.

#### Scenario: MIDI out uses host bridge config

- **WHEN** VCO envelope MIDI out is active
- **THEN** outbound channel and CC number come from `host.m_midiBridge` out fields

### Requirement: VCV CC enable toggles

The VCV primary panel SHALL provide independent enable toggles for MIDI CC 1 and MIDI CC 2 (MIDI CC 1 default On, MIDI CC 2 default Off), wired to `PagedHostIO::SetMidiCcPairEnabled`.

#### Scenario: CC 2 default off

- **WHEN** a new VCV module is placed with factory defaults
- **THEN** the MIDI CC 2 enable control is Off and mod index 1 is gated off

#### Scenario: Disable clears routes

- **WHEN** the user disables MIDI CC 1 on the VCV panel while a knob uses mod index 0
- **THEN** that route becomes None with zero depth

#### Scenario: Toggle dimmed when off

- **WHEN** MIDI CC 2 is disabled
- **THEN** the CC 2 enable control renders at reduced brightness

### Requirement: VCV assignment respects core availability

When a MIDI CC pair is disabled, mod assignment to the corresponding mod index SHALL be rejected by existing `PagedHostIO` / `DelayState` guards using `IsModSourceAvailable`.

#### Scenario: Unavailable mod index rejected

- **WHEN** MIDI CC 1 is disabled
- **THEN** `SetPageModSource` with mod index 0 is a no-op

