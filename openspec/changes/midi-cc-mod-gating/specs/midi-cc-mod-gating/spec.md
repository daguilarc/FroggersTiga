## ADDED Requirements

### Requirement: Per-pair MIDI CC enable flags

The sim host SHALL maintain independent enable flags for MIDI CC 1 (mod index 0) and MIDI CC 2 (mod index 1). Defaults: both enabled on desktop; on web both disabled until External MIDI is turned on.

#### Scenario: Desktop defaults

- **WHEN** the desktop app starts with no persisted override
- **THEN** MIDI CC 1 and MIDI CC 2 enable flags are true

#### Scenario: Web defaults before External MIDI

- **WHEN** the web sim loads and External MIDI is Off
- **THEN** MIDI CC 1 and MIDI CC 2 enable flags are false

### Requirement: Disabled CC does not ingest or drive CV

When a MIDI CC pair is disabled, the host SHALL ignore inbound CC for that pair and SHALL drive `mods[0]` or `mods[1]` to 0.0.

#### Scenario: Hardware CC ignored when disabled

- **WHEN** MIDI CC 1 is disabled and a matching hardware CC message arrives
- **THEN** `mods[0]` remains 0.0 and the latched level for pair 1 is cleared

#### Scenario: QWERTY respects CC 1 enable only

- **WHEN** Computer keyboard is the MIDI In device and MIDI CC 1 is disabled
- **THEN** QWERTY key input does not update `mods[0]`

### Requirement: Disabled mod sources are not assignable

Mod indices 0 and 1 SHALL be assignable only when their enable flag is true. New assignment attempts via patch cable, dropdown, or host API SHALL be rejected; existing routes remain unchanged until the pair is disabled.

#### Scenario: Patch cable blocked on desktop

- **WHEN** MIDI CC 2 is disabled and the user drags a cable from the MIDI CC 2 jack
- **THEN** no new mod route is created

#### Scenario: Dropdown excludes disabled source on web

- **WHEN** MIDI CC 1 is disabled
- **THEN** the knob mod `<select>` omits MIDI CC 1 and cannot select it

#### Scenario: Random mod None probability preserved

- **WHEN** the user triggers random mod and both CC sources are enabled
- **THEN** the None (255) outcome probability remains 50% as before gating

### Requirement: Random mod excludes disabled CC sources

Random mod operations (`Randomize mod`, `Rand Mods`, delay random mod) SHALL pick only from currently available assignable mod indices.

#### Scenario: Rand Mods with CC 2 off

- **WHEN** the user clicks Rand Mods and MIDI CC 2 is disabled
- **THEN** no parameter is assigned mod index 1

### Requirement: Disable clears existing routes

When a MIDI CC pair is turned off, the host SHALL clear all mod routes (all pages + Delay) that use the corresponding mod index.

#### Scenario: Clearing on disable

- **WHEN** MIDI CC 1 is toggled from enabled to disabled while a knob uses mod index 0
- **THEN** that knob mod route becomes None (255) with zero depth

### Requirement: Grey unavailable mod UI

Hosts SHALL visually distinguish disabled MIDI CC mod sources from active ones.

#### Scenario: Desktop mod rack grey state

- **WHEN** MIDI CC 1 is disabled
- **THEN** the MIDI CC 1 mod rack column renders greyed out and its output jack is not highlighted as patchable

#### Scenario: Web mod bay grey state

- **WHEN** MIDI CC 2 is disabled
- **THEN** the MIDI CC 2 scope in the mod bay renders greyed out (reduced opacity, idle trace)

### Requirement: Host scope

Desktop standalone, JUCE VST/AU, and web SHALL implement CC mod gating. VCV Rack is excluded from this change; a separate change wires VCV ingest through `CvMidiBridge` enable flags.

## MODIFIED Requirements

### Requirement: Dual CC to CV conversion

The host SHALL accept hardware MIDI CC on two independently configured `(channel, CC number)` pairs. Each pair SHALL latch the most recent matching message value as normalized CV 0–1 on `mods[0]` (MIDI CC 1) and `mods[1]` (MIDI CC 2). Non-matching messages SHALL be discarded. **Each pair SHALL only ingest and latch when its enable flag is true.**

#### Scenario: CC1 match updates mods[0]

- **WHEN** a message arrives on `(m_inChannel1, m_inCc1)` and MIDI CC 1 is enabled
- **THEN** `mods[0]` equals `value / 127`

#### Scenario: CC2 match updates mods[1]

- **WHEN** a message arrives on `(m_inChannel2, m_inCc2)` and MIDI CC 2 is enabled
- **THEN** `mods[1]` equals `value / 127`

#### Scenario: Disabled pair ignores traffic

- **WHEN** a message arrives on `(m_inChannel1, m_inCc1)` and MIDI CC 1 is disabled
- **THEN** `mods[0]` is 0.0
