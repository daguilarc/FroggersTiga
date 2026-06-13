## ADDED Requirements

### Requirement: Non-finite output recovery during playback

When any output sample in an audio block is non-finite, the host SHALL call `SoftResetFxState()` on the engine and `softResetFx()` on `DelayState` on the audio thread before the next block processes.

#### Scenario: Randomize mod no longer causes permanent silence

- **WHEN** the user clicks **Randomize mod (all)** during playback and a block produces non-finite samples
- **THEN** FX state resets on the audio thread
- **AND** subsequent blocks produce finite audio without restarting the application

#### Scenario: Device stop clears poisoned FX

- **WHEN** the audio device stops unexpectedly after a non-finite block
- **THEN** `audioDeviceStopped` triggers FX soft-reset if the last block was non-finite

#### Scenario: Stop button recovery

- **WHEN** the user clicks **Stop** after non-finite output was detected
- **THEN** FX soft-reset runs before the next **Play**

### Requirement: Extended audio-thread safety for randomize

All desktop UI-initiated **Randomize** and **Randomize mod** actions (per-panel and global) SHALL be applied through the host mutation queue, not by direct `PageManager` or `DelayState` writes from the message thread.

#### Scenario: Global strip randomize all

- **WHEN** the user clicks **Randomize all** during playback
- **THEN** page parameter randomization runs on the audio thread via the mutation queue

#### Scenario: Direct engine morph write forbidden from UI

- **WHEN** desktop UI code initiates a morph change
- **THEN** it uses the mutation queue (existing morph commands)
