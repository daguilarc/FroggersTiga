## ADDED Requirements

### Requirement: Pair-AR knobs display live modulated position

When a pair-AR parameter has an assigned mod source (mod index ≠ 255) and mod depth > 0, the sim host SHALL expose and render the **effective** knob value — base knob blended with the current mod bus sample — using the same blend rule as page-row parameters. The displayed value SHALL NOT remain frozen at the stored base knob while modulation is active.

#### Scenario: Desktop with audio running

- **WHEN** the operator assigns a moving mod source to Attack 1+2 on the Audio page, presses Play, and does not drag that knob
- **THEN** the pair-AR rotary control position updates over time to reflect the modulated effective value, matching the behavior of vertical page-row knobs on the same panel

#### Scenario: Desktop before first audio block

- **WHEN** the operator assigns a mod source to a pair-AR param while audio is stopped, then reads `GetAudioPairArEffective` from the host IO layer
- **THEN** the returned value reflects the live mod bus at read time (not the unmodulated base knob when depth > 0 and mod is active)

#### Scenario: Web with audio running

- **WHEN** the operator assigns a mod source to a pair-AR param on the web sim and audio is running
- **THEN** the third-row rotary knob position updates on screen ticks to the effective value from WASM (`froggers_get_audio_pair_ar_effective`), not the base knob

#### Scenario: Dragging mod depth vs base knob

- **WHEN** the operator drags a pair-AR knob while a mod source is assigned
- **THEN** the UI edits mod depth (not the base knob), and on drag end the control shows the effective modulated position again — same interaction model as page-row knobs

#### Scenario: Mod cleared

- **WHEN** the operator clears the mod source (None) on a pair-AR param
- **THEN** the knob displays the stored base knob value and stops tracking the mod bus
