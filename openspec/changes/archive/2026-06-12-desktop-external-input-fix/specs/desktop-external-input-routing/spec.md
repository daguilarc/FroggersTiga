## ADDED Requirements

### Requirement: Ext. In. enables line input routing

The desktop transport toggle SHALL be labeled **Ext. In.** When **Ext. In.** is on and audio is playing with at least one active input channel, the host SHALL copy device input channel 0 into the engine input buffer every audio block. When **Ext. In.** is off, the host SHALL feed zero input regardless of device signal.

#### Scenario: Ext. In. off is VCO-only

- **WHEN** **Ext. In.** is off and Play is running
- **THEN** the engine input buffer is all zeros
- **AND** mix uses OLVL × oscillators (no ring mod from line in)

#### Scenario: Ext. In. on routes device input

- **WHEN** **Ext. In.** is on, Play is running, and the device has an active input channel with signal
- **THEN** the engine receives non-zero input samples
- **AND** ring-mod mix is controlled only by the engine Schmidt gate (not a second host gate)

### Requirement: Default Ext. In. off at launch

On launch, **Ext. In.** SHALL default to **off** regardless of input device capabilities.

#### Scenario: Cold launch

- **WHEN** the app launches
- **THEN** **Ext. In.** is unchecked
- **AND** the engine receives zero external input until the user enables it
