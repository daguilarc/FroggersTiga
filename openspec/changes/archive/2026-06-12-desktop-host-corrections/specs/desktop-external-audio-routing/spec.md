## ADDED Requirements

### Requirement: Desktop always routes configured audio input to the engine

The desktop simulator SHALL pass samples from the active audio input device (channel 0, or user-selected channel from Audio Settings) to `FroggersEngine::ProcessBlock` whenever audio is playing. The host SHALL NOT zero the external input bus based on a separate On/Off UI control.

#### Scenario: Line input connected and Play pressed

- **WHEN** the user selects an input device in Audio Settings, connects a signal, and clicks Play
- **THEN** `ProcessBlock` receives non-zero input samples without an additional enable step

#### Scenario: No input cable

- **WHEN** no input is connected or input is silence
- **THEN** the engine Schmidt gate (`m_extGate`) remains low
- **AND** output uses the VCO-only path (`OLVL` × oscillator mix) per MANUAL

### Requirement: No desktop ring-mod On/Off toggle

The desktop transport bar SHALL NOT expose a **Ring mod in: On/Off** toggle or any control that gates external audio at the host layer.

#### Scenario: Transport bar controls

- **WHEN** the user views the desktop top bar
- **THEN** Play, Stop, Audio Settings, and MIDI Settings are present
- **AND** no ring-mod enable switch is present

### Requirement: Optional passive input level indicator

The desktop MAY show a read-only input envelope indicator (meter or LED) driven by `GetEnvelopeLevel()` for monitoring. It SHALL NOT gate audio and SHALL NOT require user interaction.

#### Scenario: Passive meter with signal

- **WHEN** input signal is present and Play is active
- **THEN** the indicator reflects envelope level
- **AND** audio routing is unaffected by the indicator

## REMOVED Requirements

### Requirement: External ring-mod Off host switch

**Reason**: Duplicates engine `m_extGate`; copied from web Mic model inappropriately on desktop.

**Migration**: Remove `ExternalInputMode` enum and related UI; rely on Audio Settings input selection + engine gate.
