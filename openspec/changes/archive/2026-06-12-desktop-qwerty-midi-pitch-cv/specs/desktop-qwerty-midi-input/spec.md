## MODIFIED Requirements

### Requirement: QWERTY piano keys drive MIDI mod output

When **Computer keyboard** is selected and the main window has keyboard focus, piano-layout keys SHALL update the **MIDI** mod source (`m_mods[0]`) via note on/off on the configured **in channel**. The mod level SHALL be **pitch CV × velocity**: pitch step from the highest held piano-layout note times maximum held velocity (1–127 → float 0–1). Pitch step SHALL be `clamp((highestHeldNote − 60 + 1) / 16, 0, 1)` so MIDI 60 (**A**) = 1/16 and MIDI 75 (**P**) = 1.0. When no piano-layout keys are held, the note contribution SHALL be 0. Keys SHALL NOT be collapsed into a single unrelated CC number or a velocity-only gate.

#### Scenario: Key press moves MIDI mod jack

- **WHEN** audio is playing, **Computer keyboard** is selected, in channel 1 is configured, the user patches the **MIDI** mod jack to a knob, and presses **A** with the main window focused
- **THEN** `m_mods[0]` reflects pitch and velocity for that note before the next `ProcessBlock`
- **AND** `m_mods[0]` is greater than 0 (approximately 1/16 at full velocity)
- **AND** the MIDI mod scope shows a level distinct from other keys and from idle

#### Scenario: Key release clears mod

- **WHEN** the user releases all piano-layout keys
- **THEN** the note contribution to `m_mods[0]` returns to 0 after drain

#### Scenario: Polyphonic hold

- **WHEN** the user holds **A** and **D** and releases **A**
- **THEN** `m_mods[0]` remains driven by **D** (highest remaining note) until it is released

#### Scenario: Piano layout

- **WHEN** the user presses **W** (C#) or **A** (C)
- **THEN** both keys affect the MIDI mod level at different pitch step values per the documented QWERTY map
