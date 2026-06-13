## ADDED Requirements

### Requirement: Desktop sim runs DSP at 44.1 kHz

The desktop JUCE simulator SHALL configure the audio device for **44100 Hz** sample rate and SHALL call `FroggersEngine::SetSampleRate(44100.f)` and `DelayState::setSampleRate(44100.f)` whenever audio starts. The engine sample rate SHALL NOT follow a 48000 Hz OS default without explicit user override.

#### Scenario: Play at default device rate

- **WHEN** the user clicks Play on a Mac whose system default output is 48000 Hz
- **THEN** the Froggers engine internal sample rate is 44100 Hz
- **AND** delay time mapping matches 44.1 kHz spec (e.g. max DTIM ≈ 3 s)

#### Scenario: Init before first audio callback

- **WHEN** `AudioEngine` constructs and calls `DesktopHostIO::Init()`
- **THEN** `SetSampleRate(44100.f)` has been applied to the host engine before any manual test without Play

### Requirement: Sample rate visible on mismatch

If the opened audio device reports a sample rate other than 44100 Hz after configuration, the desktop app SHALL log a warning and the Audio Settings dialog SHALL remain available for the user to select a 44100-capable device.

#### Scenario: Device rejects 44100

- **WHEN** the device manager opens at a non-44100 rate after requesting 44100
- **THEN** a warning is logged identifying requested vs actual rate
