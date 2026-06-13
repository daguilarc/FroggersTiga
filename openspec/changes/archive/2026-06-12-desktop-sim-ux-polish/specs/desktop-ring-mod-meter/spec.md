## SUPERSEDED

**Status:** Superseded by `desktop-host-corrections` — desktop uses engine `m_extGate` only; optional passive **In** envelope indicator; no host ring-mod On/Off toggle. Do not implement this spec.

---

## ADDED Requirements (historical — do not implement)

### Requirement: Ring mod input meter is separate from mod rack

The desktop UI SHALL provide a dedicated **ring mod input level meter** adjacent to the **Ring mod in: On/Off** control. This meter SHALL display external audio envelope level (`GetEnvelopeLevel()`), NOT mod bus CV (`GetCvOut`).

#### Scenario: Ring mod off

- **WHEN** ring mod input is Off
- **THEN** the ring mod meter shows zero or a dimmed idle state
- **AND** mod rack meters are unchanged

#### Scenario: Ring mod on with signal

- **WHEN** ring mod input is On, audio is playing, and channel 0 receives non-silent input
- **THEN** the ring mod meter rises with input amplitude
- **AND** mod rack MIDI meter only moves if MIDI CC is present

### Requirement: Mod rack meters labeled as mod bus output

Each mod rack box (MIDI, VCO feat, Marbles 1, Marbles 2) SHALL be labeled to indicate it shows **mod bus CV output**, not external audio input. Patching SHALL originate from the jack below the meter, not the meter bar.

#### Scenario: User identifies mod source

- **WHEN** the user views the mod rack
- **THEN** each box title or subtitle identifies it as a mod CV source
- **AND** the ring mod meter is clearly labeled as external audio input

#### Scenario: Meters idle without audio

- **WHEN** audio is not playing
- **THEN** mod rack meters read zero
- **AND** this is expected behavior, not a fault indicator
