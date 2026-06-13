## MODIFIED Requirements

### Requirement: Computer keyboard is default MIDI in on desktop

On desktop sim launch, the MIDI input mode SHALL be **Computer keyboard**. No hardware `juce::MidiInput` SHALL open until the user selects an OS MIDI device in MIDI Settings.

#### Scenario: Fresh launch without controllers

- **WHEN** the desktop app starts
- **THEN** MIDI In is **Computer keyboard**
- **AND** no hardware MIDI input device is open

### Requirement: QWERTY piano keys drive MIDI mod output

When **Computer keyboard** is selected and the main window has keyboard focus, piano-layout keys SHALL update the **MIDI** mod source (`m_mods[0]`) via note on/off on the configured **in channel**. The mod level SHALL be the maximum velocity among held piano-layout notes (1–127 → float 0–1). When no piano-layout keys are held, the mod level SHALL be 0. Keys SHALL NOT be collapsed into a single unrelated CC number.

#### Scenario: Key press moves MIDI mod jack

- **WHEN** audio is playing, **Computer keyboard** is selected, in channel 1 is configured, the user patches the **MIDI** mod jack to a knob, and presses **A** with the main window focused
- **THEN** `m_mods[0]` reflects key velocity before the next `ProcessBlock`
- **AND** the MIDI mod scope reflects activity

#### Scenario: Key release clears mod

- **WHEN** the user releases all piano-layout keys
- **THEN** `m_mods[0]` returns to 0 after drain

#### Scenario: Polyphonic hold

- **WHEN** the user holds **A** and **D** and releases **A**
- **THEN** `m_mods[0]` remains driven by **D** until it is released

#### Scenario: Piano layout

- **WHEN** the user presses **W** (C#) or **A** (C)
- **THEN** both keys affect the MIDI mod level per the documented QWERTY map

### Requirement: Hardware MIDI in accepts notes for mod

When a hardware device is selected as MIDI In, **Note On** and **Note Off** on the configured in channel SHALL drive `m_mods[0]` with the same velocity semantics as QWERTY. **Controller** messages on the configured in channel and in CC SHALL continue to drive `m_mods[0]` from CC value.

#### Scenario: External keyboard notes

- **WHEN** the user selects a hardware MIDI keyboard as MIDI In and plays notes on the configured channel
- **THEN** the **MIDI** mod jack reflects note velocity without requiring CC messages
