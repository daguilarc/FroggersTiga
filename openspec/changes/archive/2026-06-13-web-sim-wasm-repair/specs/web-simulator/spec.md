## MODIFIED Requirements

### Requirement: External audio toggle

The web simulator SHALL expose **External: Off | On** (renamed from Mic in v2). Default SHALL be **Off**. When off, the AudioWorklet SHALL pass zero for external input samples. Turning **On** SHALL follow `web-external-audio-permission` (user-gesture `getUserMedia`, pessimistic UI, denial recovery).

#### Scenario: External off avoids permission prompt on Play

- **WHEN** the user clicks Play with External off
- **THEN** the browser does not request microphone permission

#### Scenario: External enables ring mod after grant

- **WHEN** the user turns **External** on, grants mic access, and audio is running
- **THEN** mic feeds the external input path and processed output reflects ring-mod character without NaN

#### Scenario: External denied stays off

- **WHEN** the user turns **External** on but microphone permission is denied
- **THEN** **External: Off** remains shown
- **AND** the status line explains how to re-enable microphone access for the site
