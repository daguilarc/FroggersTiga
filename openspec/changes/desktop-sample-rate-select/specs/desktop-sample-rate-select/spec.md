## ADDED Requirements

### Requirement: Supported sample rates authority

Standalone desktop SHALL read supported sample rates from `HostAudioConfig.hpp`. Only **44100 Hz** and **48000 Hz** SHALL be offered.

#### Scenario: Default rate

- **WHEN** app launches fresh
- **THEN** requested device sample rate is 44100 Hz unless user previously chose 48000 in the same session's device setup persistence

### Requirement: Sample rate selector in Audio Settings

Audio Settings SHALL expose a sample-rate control with labels **44100 Hz** and **48000 Hz**. Changing the rate SHALL re-open the audio device at the selected rate.

#### Scenario: Switch to 48 kHz

- **WHEN** user selects 48000 Hz and output device supports it
- **THEN** `getCurrentSampleRate()` reports 48000 and engine runs at 48000 Hz

### Requirement: Engine sync on device start

When the audio device starts, `AudioEngine` SHALL call `setHostSampleRate` with the device's current sample rate.

#### Scenario: Device opens at selected rate

- **WHEN** device starts at 48000 Hz
- **THEN** `FroggersEngine` and `DelayState` sample rates are 48000 Hz

### Requirement: Export matches capture rate

Exported recordings SHALL use the active engine sample rate in the file writer, not a hardcoded 44100 Hz.

#### Scenario: Export after 48 kHz session

- **WHEN** user records at 48000 Hz and exports WAV
- **THEN** file header reports 48000 Hz
