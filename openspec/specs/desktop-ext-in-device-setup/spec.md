# desktop-ext-in-device-setup Specification

## Purpose
TBD - created by archiving change desktop-ext-in-fix. Update Purpose after archive.
## Requirements
### Requirement: Active input channel when input device configured

When an input device is selected in Audio Settings, the desktop host SHALL enable at least input channel 0 in `AudioDeviceSetup.inputChannels` before audio starts. This SHALL apply to all input device types (built-in mic, line-in, USB audio interface).

#### Scenario: Launch with default input device

- **WHEN** the app launches and the OS provides a default input device
- **THEN** input channel 0 is active in the device setup
- **AND** `audioDeviceIOCallbackWithContext` receives `numInputChannels >= 1` when Play is running

#### Scenario: User changes input device in Audio Settings

- **WHEN** the user selects a different input device and closes Audio Settings
- **THEN** `syncInputChannelSetup()` runs
- **AND** input channel 0 is active for the new device

#### Scenario: Line-in on audio interface

- **WHEN** the user selects an audio interface with line input, enables **Ext. In.**, and clicks Play
- **THEN** non-zero samples from input channel 0 reach `ProcessBlock` when signal is present
- **AND** the peak meter reflects input level

### Requirement: Ext. In. enables line input routing

The desktop transport toggle SHALL be labeled **Ext. In.** When **Ext. In.** is on and audio is playing with at least one active input channel, the host SHALL copy device input channel 0 into the engine input buffer every audio block. When **Ext. In.** is off, the host SHALL feed zero input regardless of device signal.

#### Scenario: Ext. In. off is VCO-only

- **WHEN** **Ext. In.** is off and Play is running
- **THEN** the engine input buffer is all zeros
- **AND** mix uses OLVL × oscillators (no ring mod from line in)

#### Scenario: Ext. In. on routes device input

- **WHEN** **Ext. In.** is on, Play is running, and the device has an active input channel with signal
- **THEN** the engine receives non-zero input samples
- **AND** ring-mod mix is controlled by the engine Schmidt gate

#### Scenario: Default Ext. In. off at launch

- **WHEN** the app launches
- **THEN** **Ext. In.** is unchecked
- **AND** the engine receives zero external input until the user enables it

### Requirement: Audio Settings requires at least one input channel

The Audio Settings dialog SHALL NOT allow disabling all input channels (minimum 1, maximum 2).

#### Scenario: User opens Audio Settings

- **WHEN** the user opens Audio Settings
- **THEN** at least one input channel must remain enabled
- **AND** the user cannot apply a setup with zero active input channels

