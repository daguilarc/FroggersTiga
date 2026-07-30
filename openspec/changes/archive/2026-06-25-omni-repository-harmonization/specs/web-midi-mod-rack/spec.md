## REMOVED Requirements

### Requirement: Web MIDI CC drives mod sources zero and one
**Reason**: The browser product supports one MIDI CC input; exposing a second configurable CC pair adds UI and routing complexity for an uncommon browser workflow.
**Migration**: Keep MIDI CC 1 on mod index 0 and remove browser ingestion, scope, assignment, and enablement for MIDI CC 2/mod index 1. Desktop standalone retains its dual-CC behavior; VST/AU and VCV use their host-native parameter/CV boundaries.

### Requirement: Mod bay shows MIDI CC 1 and MIDI CC 2 scopes
**Reason**: The five-entry dual-CC rack is a native-host contract, not the intended browser contract.
**Migration**: Replace it with the four-entry browser rack defined below.

## ADDED Requirements

### Requirement: Web MIDI CC drives only mod source zero
When External MIDI is on, matching Control Change messages SHALL update `mods[0]` for MIDI CC 1 using the shared `CvMidiBridge` rules. The browser SHALL NOT ingest or expose the MIDI CC 2 pair on `mods[1]`.

#### Scenario: CC 1 updates mod bay scope
- **WHEN** External MIDI is on and a matching MIDI CC 1 value 64 arrives
- **THEN** the MIDI CC 1 mod-bay scope reflects approximately 50% level

#### Scenario: CC 2 is unavailable in the browser
- **WHEN** External MIDI is on
- **THEN** the browser provides no MIDI CC 2 control, scope, ingestion route, or assignable mod source

### Requirement: Mod bay shows one MIDI CC scope
The web mod bay SHALL display four persistent entries in this order: MIDI CC 1, VCO Envelope, Random 1, Random 2. MIDI CC 1 and VCO Envelope SHALL use scope presentation; Random 1 and Random 2 SHALL use LED presentation. Labels SHALL come from WASM/shared display metadata. MIDI CC 1 SHALL remain visible with an unavailable state while External MIDI is Off.

#### Scenario: Four mod bay entries
- **WHEN** the mod bay is expanded before or after audio starts
- **THEN** four entries appear in order for mod indices `0, 4, 5, 6`

#### Scenario: External MIDI is off
- **WHEN** the web sim starts with External MIDI Off
- **THEN** the MIDI CC 1 scope is visible but unavailable and mod index 0 is absent from assignable options
