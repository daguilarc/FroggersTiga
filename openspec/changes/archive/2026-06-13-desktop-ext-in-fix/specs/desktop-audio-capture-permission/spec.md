## ADDED Requirements

### Requirement: macOS audio input capture permission

The desktop app bundle SHALL declare audio input capture permission via JUCE `MICROPHONE_PERMISSION_ENABLED` and a user-facing permission string that describes **external audio input** (mic, line-in, or interface), not microphone-only.

#### Scenario: Built app Info.plist

- **WHEN** the desktop app is built for macOS
- **THEN** `Info.plist` contains `NSMicrophoneUsageDescription`
- **AND** the string mentions external audio input / ring-mod routing

#### Scenario: First capture attempt

- **WHEN** the user enables **Ext. In.**, clicks Play, and macOS has not yet granted input access
- **THEN** macOS shows a permission prompt (or directs user to System Settings)
- **AND** after grant, input samples reach the engine

### Requirement: Permission string is source-agnostic

The permission text SHALL NOT imply microphone-only capture.

#### Scenario: USB interface user

- **WHEN** a user with a USB audio interface reads the permission prompt
- **THEN** the text describes audio input for the simulator
- **AND** does not say "microphone" alone without mentioning line/interface input
