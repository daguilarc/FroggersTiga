## ADDED Requirements

### Requirement: Audio IO state feeds runtime audio projection
Desktop v2 audio IO state SHALL feed the runtime Audio page projection without changing existing stereo default, mono downmix, optional mono input, or mono-core rendering behavior.

#### Scenario: Runtime page reflects stereo default
- **WHEN** desktop v2 initializes default audio output
- **THEN** the runtime Audio page reports two active output channels
- **THEN** stereo processing behavior matches the existing audio IO contract

#### Scenario: Mono downmix remains shared
- **WHEN** standalone or hosted output has one channel
- **THEN** the runtime Audio page reports mono output
- **THEN** audio downmix uses the existing shared mono path
