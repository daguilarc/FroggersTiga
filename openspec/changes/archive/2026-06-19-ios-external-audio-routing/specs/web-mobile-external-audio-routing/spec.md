## ADDED Requirements

### Requirement: Mobile-only audio session management

The web sim SHALL apply `navigator.audioSession` lifecycle hooks only when `isMobileWeb()` is true. Desktop browsers (non-mobile viewport and UA) SHALL NOT receive audio session mutations.

#### Scenario: Desktop Play unchanged

- **WHEN** a desktop user starts Play on a wide viewport without a mobile user agent
- **THEN** the sim does not set `navigator.audioSession.type`

#### Scenario: Mobile Play asserts playback when External off

- **WHEN** a mobile user starts Play with External Audio off and the Audio Session API exists
- **THEN** the sim sets `navigator.audioSession.type` to `'playback'`

### Requirement: Mobile Audio Session resets before External capture

On mobile browsers, immediately before `getUserMedia` when the user enables External Audio, the web sim SHALL set `navigator.audioSession.type` to `'auto'` when the Audio Session API is available.

#### Scenario: Enable External on mobile

- **WHEN** a mobile user turns External Audio on and the Audio Session API exists
- **THEN** the sim sets `navigator.audioSession.type` to `'auto'` before requesting the microphone

#### Scenario: No Audio Session API

- **WHEN** a mobile user turns External Audio on and `navigator.audioSession` is unavailable
- **THEN** the sim proceeds with `getUserMedia` without throwing

### Requirement: Mobile Audio Session marks play-and-record after mic connects

On mobile browsers, after External Audio successfully acquires a mic stream and connects `MediaStreamAudioSourceNode` to the worklet, the web sim SHALL set `navigator.audioSession.type` to `'play-and-record'` when the API is available.

#### Scenario: Mic stream active on mobile

- **WHEN** External Audio is on and the mic source is connected on a mobile browser
- **THEN** `navigator.audioSession.type` is `'play-and-record'` (when supported)

### Requirement: Mobile Audio Session restores playback when External releases mic

On mobile browsers, when External Audio turns off or Stop clears the mic stream, after all media tracks are stopped the web sim SHALL set `navigator.audioSession.type` to `'playback'` and then `'auto'` when the API is available.

#### Scenario: Toggle External off on mobile

- **WHEN** a mobile user turns External Audio off after it was on
- **THEN** mic tracks are stopped and the audio session is reset via `'playback'` then `'auto'`

#### Scenario: Stop transport on mobile

- **WHEN** a mobile user clicks Stop while External Audio was on
- **THEN** mic tracks are stopped and the audio session is reset via `'playback'` then `'auto'`

### Requirement: Mobile status hint distinguishes built-in vs headset routing

On mobile browsers, while audio is playing and External Audio is on, the transport status line SHALL explain that without headphones iOS may use the built-in earpiece, and that headphones or turning External off avoids that limitation.

#### Scenario: Playing with External on mobile

- **WHEN** Play is active, External Audio is on, and the host is a mobile browser
- **THEN** the status line includes built-in earpiece vs headset guidance

#### Scenario: External off on mobile

- **WHEN** External Audio is off
- **THEN** the earpiece routing hint is not shown

### Requirement: Subtitle clarifies Play vs External

The web sim header subtitle SHALL state that Play produces sound without External Audio, and that External adds microphone ring-mod input.

#### Scenario: Page load

- **WHEN** the web sim loads
- **THEN** the subtitle distinguishes Play-for-sound from External-for-mic-input

### Requirement: Manual documents mobile External routing

The sim manual Web host guide SHALL document mobile browser External Audio routing: play-and-record session, earpiece risk on iPhone without headphones, normal headset output with headphones, External off/reload restores built-in speaker, and that desktop browsers are unaffected.

#### Scenario: Manual Web section

- **WHEN** a reader opens the Host guide → Web section
- **THEN** a mobile External Audio routing note is present in all synced manual copies
