## MODIFIED Requirements

### Requirement: Per-pair MIDI CC enable flags
Desktop standalone SHALL maintain independent enable flags for MIDI CC 1 (mod index 0) and MIDI CC 2 (mod index 1), defaulting to CC 1 enabled and CC 2 disabled. Web SHALL expose only MIDI CC 1 and keep it disabled until External MIDI is enabled. VST/AU SHALL route DAW MIDI through host parameters rather than fixed CC mod pairs. VCV SHALL expose no MIDI ingest or CC mod pairs.

#### Scenario: Desktop defaults
- **WHEN** the desktop app starts with no persisted override
- **THEN** MIDI CC 1 enable flag is true and MIDI CC 2 enable flag is false

#### Scenario: Web defaults before External MIDI
- **WHEN** the web sim loads and External MIDI is Off
- **THEN** MIDI CC 1 is disabled and MIDI CC 2 is not exposed as a browser input or mod source

#### Scenario: Web External MIDI on
- **WHEN** the operator turns External MIDI on
- **THEN** MIDI CC 1 becomes enabled while MIDI CC 2 remains unavailable to the browser

#### Scenario: Web External MIDI off resets CC 1
- **WHEN** the operator turns External MIDI Off or tears down the worklet
- **THEN** the web MIDI CC 1 enable flag returns to false and no MIDI CC 2 browser state is created

#### Scenario: VST and VCV do not instantiate fixed pairs
- **WHEN** a VST/AU processor or VCV module is initialized
- **THEN** no host-visible CC-pair enable controls or CC mod-source assignments are available

### Requirement: Grey unavailable mod UI
Hosts that support fixed MIDI CC mod sources SHALL visually distinguish disabled sources from active ones. Desktop standalone SHALL show disabled supported CC cells as unavailable. Web SHALL show MIDI CC 1 as unavailable while External MIDI is Off. VST/AU and VCV SHALL omit fixed CC cells entirely.

#### Scenario: Desktop mod rack grey state
- **WHEN** MIDI CC 1 is disabled in desktop standalone
- **THEN** the MIDI CC 1 mod rack column renders greyed out and its output jack is not highlighted as patchable

#### Scenario: Web mod bay grey state
- **WHEN** External MIDI is Off
- **THEN** the web MIDI CC 1 scope renders greyed out with an idle trace

#### Scenario: Unsupported host omits CC UI
- **WHEN** VST/AU or VCV displays its modulation surface
- **THEN** no unavailable or active fixed MIDI CC cell is rendered

### Requirement: Host scope
Desktop standalone and web SHALL implement fixed CC-to-mod gating according to their supported pair counts. VST/AU SHALL use DAW-to-parameter routing instead. VCV SHALL accept modulation as CV and SHALL expose no MIDI boundary.

#### Scenario: Fixed CC gating deployment
- **WHEN** this capability is applied
- **THEN** fixed CC pair flags affect desktop standalone and web only

### Requirement: Dual CC to CV conversion with enable gating
Desktop standalone SHALL accept two independently configured `(channel, CC number)` pairs and latch normalized values on `mods[0]` and `mods[1]` when enabled. Web SHALL accept only its configured CC 1 pair on `mods[0]` while External MIDI is On. VST/AU and VCV SHALL NOT use `mods[0]` or `mods[1]` as MIDI CC latches.

#### Scenario: Desktop CC 1 match updates mods zero
- **WHEN** desktop standalone receives a matching enabled CC 1 message
- **THEN** `mods[0]` equals `value / 127`

#### Scenario: Desktop CC 2 match updates mods one
- **WHEN** desktop standalone receives a matching enabled CC 2 message
- **THEN** `mods[1]` equals `value / 127`

#### Scenario: Web only accepts CC 1
- **WHEN** External MIDI is On and web receives matching CC 1 traffic
- **THEN** `mods[0]` updates and no browser MIDI path updates `mods[1]`

#### Scenario: VST and VCV have no MIDI latches
- **WHEN** VST/AU or VCV processes control input
- **THEN** neither host uses fixed MIDI CC latches at mod indices 0 or 1
