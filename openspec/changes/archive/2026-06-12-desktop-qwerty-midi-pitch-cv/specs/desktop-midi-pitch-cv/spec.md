## ADDED Requirements

### Requirement: MIDI mod level combines pitch and velocity

When notes are held on the configured MIDI in channel, `m_mods[0]` SHALL equal the normalized pitch step of the **highest held note** multiplied by the maximum held velocity (velocity normalized 0–1). When no notes are held, `m_mods[0]` from the note path SHALL be 0 before CC processing.

Pitch step SHALL be `clamp((highestHeldNote − 60 + 1) / 16, 0, 1)` for the QWERTY piano map (MIDI 60 = 1/16, MIDI 75 = 1.0). Notes outside 60–75 SHALL clamp into that range before the formula.

#### Scenario: Different QWERTY keys produce different mod levels

- **WHEN** audio is playing, **Computer keyboard** is selected, and the user holds **A** (MIDI 60) then releases and holds **P** (MIDI 75)
- **THEN** `m_mods[0]` is lower for **A** than for **P** while each key is held
- **AND** the MIDI mod scope shows distinct levels

#### Scenario: Lowest key is not silent

- **WHEN** audio is playing, **Computer keyboard** is selected, and the user holds only **A** (MIDI 60)
- **THEN** `m_mods[0]` is approximately 1/16 × (velocity / 127)
- **AND** `m_mods[0]` is greater than 0 and distinct from the no-keys-held level

#### Scenario: No keys held

- **WHEN** the user releases all held notes on the in channel
- **THEN** the note contribution to `m_mods[0]` is 0 before CC events apply

#### Scenario: Polyphonic highest note

- **WHEN** the user holds **A** (60) and **D** (64) together
- **THEN** `m_mods[0]` reflects pitch step for note 64 (highest held)

#### Scenario: Hardware velocity scales level

- **WHEN** a hardware MIDI keyboard is selected and the user plays the same pitch at velocity 64 vs 127
- **THEN** `m_mods[0]` is proportionally lower at velocity 64

### Requirement: Note events drain on the audio thread

Note on and note off events SHALL be enqueued from the message thread via a thread-safe SPSC queue and applied to held-note state only inside `drainMidiIn` on the audio thread. `PushMidiNote` SHALL NOT mutate held-note arrays directly.

#### Scenario: QWERTY key during Play

- **WHEN** the user presses a piano-layout key while audio is running
- **THEN** held-note state updates before the next `ProcessBlock` via `drainMidiIn`
- **AND** no data race occurs between UI key handling and the audio callback

### Requirement: CC precedence unchanged

Controller messages on the configured in channel and in CC SHALL continue to set `m_mods[0]` after note level is computed in the same `drainMidiIn` call.

#### Scenario: CC after notes

- **WHEN** a held note sets `m_mods[0]` and a matching CC event arrives in the same drain
- **THEN** `m_mods[0]` reflects the CC value
